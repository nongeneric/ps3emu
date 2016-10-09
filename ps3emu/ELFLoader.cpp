#include "ELFLoader.h"
#include "ppu/PPUThread.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdexcept>
#include "log.h"
#include "state.h"
#include <boost/range/algorithm.hpp>
#include <boost/filesystem.hpp>

using namespace boost::endian;

static const uint32_t PT_TYPE_PARAMS = PT_LOOS + 1;
static const uint32_t PT_TYPE_PRXINFO = PT_LOOS + 2;

uint32_t findExportedSymbol(std::vector<ELFLoader*> const& prxs,
                            uint32_t id,
                            std::string library,
                            prx_symbol_type_t type);

void ELFLoader::load(std::string filePath) {
    _elfName = filePath;
    FILE* f = fopen(filePath.c_str(), "rb");
    assert(f);
    fseek(f, 0, SEEK_END);
    auto fileSize = static_cast<unsigned>(ftell(f));
    if (fileSize < sizeof(Elf64_Ehdr))
        throw std::runtime_error("File too small for an ELF64 file");
    
    fseek(f, 0, SEEK_SET);
    
    _file.resize(fileSize);
    auto read = fread(&_file[0], 1, fileSize, f);
    (void)read;
    assert(read == fileSize);
    fclose(f);
    
    _header = reinterpret_cast<Elf64_be_Ehdr*>(&_file[0]);
    bool hasElfIdent = 
        _header->e_ident[EI_MAG0] == ELFMAG0 &&
        _header->e_ident[EI_MAG1] == ELFMAG1 &&
        _header->e_ident[EI_MAG2] == ELFMAG2 &&
        _header->e_ident[EI_MAG3] == ELFMAG3;
    if (!hasElfIdent)
        throw std::runtime_error("Incorrect ELF magic");
    
    if (_header->e_ident[EI_CLASS] != ELFCLASS64)
        throw std::runtime_error("Only ELF64 supported");
    
    auto ELFOSABI_CELLOSLV2 = 0x66;
    
    if (_header->e_ident[EI_OSABI] != ELFOSABI_CELLOSLV2)
        throw std::runtime_error("Not CELLOSLV2 ABI");
    
    if (_header->e_ident[EI_DATA] != ELFDATA2MSB)
        throw std::runtime_error("Only big endian supported");
    
    if (_header->e_machine != EM_PPC64)
        throw std::runtime_error("Unkown machine value");
    
    _sections = reinterpret_cast<Elf64_be_Shdr*>(&_file[0] + _header->e_shoff);
    
    if (_header->e_phoff != sizeof(Elf64_Ehdr))
        throw std::runtime_error("Unknown data between header and program header");
    
    _pheaders = reinterpret_cast<Elf64_be_Phdr*>(&_file[0] + _header->e_phoff);
    
    for (auto ph = _pheaders; ph != _pheaders + _header->e_phnum; ++ph) {
        if (ph->p_type == PT_INTERP)
            throw std::runtime_error("program interpreter not supported");
    }
}

const char* ELFLoader::getString(uint32_t idx) {
    for (auto s = _sections; s != _sections + _header->e_shnum; ++s) {
        if (s->sh_type == SHT_STRTAB && std::distance(_sections, s) != _header->e_shstrndx) {
            if (s->sh_size < idx)
                throw std::runtime_error("string table out of bounds");
            auto ptr = &_file[0] + s->sh_offset + idx;
            return reinterpret_cast<const char*>(ptr);
        }
    }
    throw std::runtime_error("string table section not found");
}

const char* ELFLoader::getSectionName(uint32_t idx) {
    if (_header->e_shstrndx == 0)
        return "";
    auto s = _sections + _header->e_shstrndx;
    if (s->sh_size <= idx)
        throw std::runtime_error("section name string table out of bounds");
    auto ptr = &_file[0] + s->sh_offset + idx;
    return reinterpret_cast<const char*>(ptr);
}

module_info_t* ELFLoader::module() {
    return _module;
}

struct vdescr {
    big_uint32_t size;
    big_uint32_t toc;
};

struct Replacement {
    std::string lib;
    std::string name;
};

std::vector<StolenFuncInfo> ELFLoader::map(MainMemory* mm,
                                           make_segment_t makeSegment,
                                           ps3_uintptr_t imageBase) {
    uint32_t prxPh1Va = 0;
    for (auto ph = _pheaders; ph != _pheaders + _header->e_phnum; ++ph) {
        if (ph->p_type != PT_LOAD && ph->p_type != PT_TLS)
            continue;
        if (ph->p_vaddr % ph->p_align != 0)
            throw std::runtime_error("complex alignment not supported");
        if (ph->p_memsz == 0)
            continue;
        
        uint64_t va = imageBase + ph->p_vaddr;
        auto index = std::distance(_pheaders, ph);
        if (imageBase && index == 1) {
            assert(_header->e_phnum == 3);
            auto ph0 = _pheaders[0];
            prxPh1Va = va = ::align(imageBase + ph0.p_vaddr + ph0.p_memsz, 0x10000);
        }
        
        LOG << ssnprintf("mapping segment of size %08" PRIx64 " to %08" PRIx64 "-%08" PRIx64 " (image base: %08x)",
            (uint64_t)ph->p_filesz, va, va + ph->p_memsz, imageBase);
        
        assert(ph->p_memsz >= ph->p_filesz);
        mm->writeMemory(va, ph->p_offset + &_file[0], ph->p_filesz, true);
        mm->setMemory(va + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz, true);
        
        if (ph->p_type != PT_TLS) {
            makeSegment(va, ph->p_memsz, index);
        }
    }
    
    if (!imageBase)
        return {};
    
    assert(prxPh1Va);
    
    auto lo = [](uint32_t x) { return x & 0xffff; };
    auto hi = [](uint32_t x) { return (x >> 16) & 0xffff; };
    auto ha = [](uint32_t x) { return ((x >> 16) + ((x & 0x8000) ? 1 : 0)) & 0xffff; };
    
    auto rel = (Elf64_be_Rela*)&_file[_pheaders[2].p_offset];
    auto endRel = rel + _pheaders[2].p_filesz / sizeof(Elf64_be_Rela);
    for (; rel != endRel; ++rel) {
        assert(((uint64_t)rel->r_offset >> 32) == 0);
        assert(((uint64_t)rel->r_addend >> 32) == 0);
        uint64_t info = rel->r_info;
        uint32_t sym = ELF64_R_SYM(info);
        uint32_t type = ELF64_R_TYPE(info);
        auto offsetRelativeToSegment = sym & 0xff;
        assert(offsetRelativeToSegment <= 1);
        auto pointsToSegment = (sym >> 8) & 0xff;
        assert(offsetRelativeToSegment <= 1);
        auto offset = rel->r_offset + (offsetRelativeToSegment ? prxPh1Va : imageBase);
        auto base = pointsToSegment ? prxPh1Va : imageBase;
        auto val = rel->r_addend + base;
        if (type == R_PPC64_ADDR32) {
            mm->store<4>(offset, val);
        } else if (type == R_PPC64_ADDR16_LO) {
            mm->store<2>(offset, lo(val));
        } else if (type == R_PPC64_ADDR16_HI) {
            mm->store<2>(offset, hi(val));
        } else if (type == R_PPC64_ADDR16_HA) {
            mm->store<2>(offset, ha(val));
        } else {
            throw std::runtime_error("unimplemented reloc type");
        }
    }
    
    _module = (module_info_t*)mm->getMemoryPointer(
        _pheaders->p_paddr - _pheaders->p_offset + imageBase, sizeof(module_info_t));
    
    std::vector<Replacement> replacements = {
        { "cellSpurs", "cellSpursInitializeWithAttribute2" },
        //{ "cellSpurs", "cellSpursFinalize" },
        { "sysPrxForUser", "sys_lwmutex_create" },
        { "sysPrxForUser", "sys_lwmutex_lock" },
        { "sysPrxForUser", "sys_lwmutex_unlock" },
        { "sysPrxForUser", "sys_lwmutex_trylock" },
        { "sysPrxForUser", "sys_lwmutex_destroy" },
        { "sysPrxForUser", "sys_lwcond_create" },
        { "sysPrxForUser", "sys_lwcond_destroy" },
        { "sysPrxForUser", "sys_lwcond_wait" },
        { "sysPrxForUser", "sys_lwcond_signal_all" },
        { "sysPrxForUser", "sys_lwcond_signal" },
    };
    
    std::vector<StolenFuncInfo> stolenInfos;
    for (auto&& repl : replacements) {
        auto fnid = calcFnid(repl.name.c_str());
        auto stub = findExportedSymbol({this}, fnid, repl.lib, prx_symbol_type_t::function);
        if (stub == 0)
            continue;
        auto codeVa = mm->load<4>(stub);
        uint32_t index;
        auto entry = findNCallEntry(fnid, index);
        assert(entry); (void)entry;
        stolenInfos.push_back({codeVa, stub, mm->load<4>(codeVa), index});
        encodeNCall(mm, codeVa, index);
    }
    return stolenInfos;
}

std::tuple<prx_import_t*, int> ELFLoader::prxImports(MainMemory* mm) {
    auto count = _module->imports_end - _module->imports_start;
    return std::make_tuple(
        (prx_import_t*)mm->getMemoryPointer(_module->imports_start, count),
        count / sizeof(prx_import_t));
}

std::tuple<prx_import_t*, int> ELFLoader::imports(MainMemory* mm) {
    if (_module)
        return prxImports(mm);
    return elfImports(mm);
}

std::tuple<prx_export_t*, int> ELFLoader::exports(MainMemory* mm) {
    if (!_module)
        return {nullptr, 0};
    
    auto count = _module->exports_end - _module->exports_start;
    return std::make_tuple(
        (prx_export_t*)mm->getMemoryPointer(_module->exports_start, count),
        count / sizeof(prx_export_t));
}

bool isSymbolWhitelisted(ELFLoader* prx, uint32_t id) {
    auto name = prx->shortName();
    if (name == "liblv2.prx") {
        return true;
    }
    if (name == "libsre.prx") {
        return true;
    }
    if (name == "libsync2.prx") {
        return true;
    }
    if (name == "libfiber.prx") {
        return true;
    }
    if (name == "libgcm_sys.sprx.elf" || name == "libsysutil_game.sprx.elf" ||
        name == "libsysutil.sprx.elf" || name == "libaudio.sprx.elf" ||
        name == "libio.sprx.elf" || name == "libfs.sprx.elf" ||
        name == "libsysutil_np_trophy.sprx.elf") {
        return false;
    }
    return true;
}

uint32_t findExportedEmuFunction(uint32_t id) {
    static auto curUnknownNcall = 2000;
    static auto ncallDescrVa = FunctionDescriptorsVa;
    uint32_t index;
    auto ncallEntry = findNCallEntry(id, index);
    std::string name;
    if (!ncallEntry) {
        index = curUnknownNcall--;
        name = ssnprintf("(!) fnid_%08X", id);
    } else {
        name = ncallEntry->name;
    }
    LOG << ssnprintf("    %s (ncall %x)", name, index);
    
    g_state.mm->setMemory(ncallDescrVa, 0, 8, true);
    g_state.mm->store<4>(ncallDescrVa, ncallDescrVa + 4);
    encodeNCall(g_state.mm, ncallDescrVa + 4, index);
    ncallDescrVa += 8;
    return ncallDescrVa - 8;
}

uint32_t findExportedSymbol(std::vector<ELFLoader*> const& prxs,
                            uint32_t id,
                            std::string library,
                            prx_symbol_type_t type) {
    for (auto prx : prxs) {
        prx_export_t* exports;
        int nexports;
        std::tie(exports, nexports) = prx->exports(g_state.mm);
        for (auto i = 0; i < nexports; ++i) {
            if (!exports[i].name)
                continue;
            std::string name;
            readString(g_state.mm, exports[i].name, name);
            if (name != library)
                continue;
            auto fnids = (big_uint32_t*)g_state.mm->getMemoryPointer(exports[i].fnid_table, 4 * exports[i].functions);
            auto stubs = (big_uint32_t*)g_state.mm->getMemoryPointer(exports[i].stub_table, 4 * exports[i].functions);
            auto first = 0;
            auto last = 0;
            if (type == prx_symbol_type_t::function) {
                first = 0;
                last = exports[i].functions;
            } else if (type == prx_symbol_type_t::variable) {
                first = exports[i].functions;
                last = first + exports[i].variables;
            } else { assert(false); }
            
            for (auto j = first; j < last; ++j) {
                if (fnids[j] == id) {
                    if (!isSymbolWhitelisted(prx, fnids[j]))
                        continue;
                    return stubs[j];
                }
            }
        }
    }
    return 0;
}

uint32_t findExportedSymbol(std::vector<std::shared_ptr<ELFLoader>> const& prxs,
                            uint32_t id,
                            std::string library,
                            prx_symbol_type_t type) {
    std::vector<ELFLoader*> vec;
    for (auto& prx : prxs) {
        vec.push_back(prx.get());
    }
    return findExportedSymbol(vec, id, library, type);
}

Elf64_be_Shdr* ELFLoader::findSectionByName(std::string name) {
    for (auto s = _sections; s != _sections + _header->e_shnum; ++s) {
        if (getSectionName(s->sh_name) == name)
            return s;
    }
    return nullptr;
}

std::tuple<prx_import_t*, int> ELFLoader::elfImports(MainMemory* mm) {
    auto phprx = std::find_if(_pheaders, _pheaders + _header->e_phnum, [](auto& ph) {
        return ph.p_type == PT_TYPE_PRXINFO;
    });
    
    if (phprx == _pheaders + _header->e_phnum)
        throw std::runtime_error("PRXINFO segment not present");
    
    auto prxInfo = (sys_process_prx_info*)mm->getMemoryPointer(phprx->p_vaddr, sizeof(sys_process_prx_info));
    auto count = prxInfo->imports_end - prxInfo->imports_start;
    return std::make_tuple(
        (prx_import_t*)mm->getMemoryPointer(prxInfo->imports_start, count),
        count / sizeof(prx_import_t));
}

void ELFLoader::link(MainMemory* mm, std::vector<std::shared_ptr<ELFLoader>> prxs) {
    INFO(libs) << ssnprintf("linking prx %s", elfName());
    prx_import_t* imports;
    int count;
    std::tie(imports, count) = this->imports(mm);
    for (auto i = 0; i < count; ++i) {
        std::string library;
        readString(mm, imports[i].name, library);
        auto vstubs = (big_uint32_t*)mm->getMemoryPointer(imports[i].vstubs, 4 * imports[i].variables);
        auto vnids = (big_uint32_t*)mm->getMemoryPointer(imports[i].vnids, 4 * imports[i].variables);
        for (auto j = 0; j < imports[i].variables; ++j) {
            auto descr = (vdescr*)mm->getMemoryPointer(vstubs[j], sizeof(vdescr));
            auto symbol = findExportedSymbol(prxs, vnids[j], library, prx_symbol_type_t::variable);
            mm->store<4>(descr->toc, symbol);
        }
        auto fstubs = (big_uint32_t*)mm->getMemoryPointer(imports[i].fstubs, imports[i].functions);
        auto fnids = (big_uint32_t*)mm->getMemoryPointer(imports[i].fnids, imports[i].functions);
        for (auto j = 0; j < imports[i].functions; ++j) {
            auto symbol = findExportedSymbol(prxs, fnids[j], library, prx_symbol_type_t::function);
            if (!symbol) {
                symbol = findExportedEmuFunction(fnids[j]);
            }
            fstubs[j] = symbol;
            
        }
    }
}

uint64_t ELFLoader::entryPoint() {
    return _header->e_entry;
}

Elf64_be_Sym* ELFLoader::getGlobalSymbolByValue(uint32_t value, uint32_t section) {
    Elf64_be_Sym* res = nullptr;
    foreachGlobalSymbol([&](auto sym) {
        if (sym->st_value == value &&
            (sym->st_shndx == section || section == ~0u))
        {
            res = sym;
        }
    });
    return res;
}

void ELFLoader::foreachGlobalSymbol(std::function<void(Elf64_be_Sym*)> action) {
    for (auto s = _sections; s != _sections + _header->e_shnum; ++s) {
        if (s->sh_type == SHT_SYMTAB) {
            if (s->sh_entsize != sizeof(Elf64_be_Sym))
                throw std::runtime_error("bad symbol table");
            int count = s->sh_size / s->sh_entsize;
            auto sym = reinterpret_cast<Elf64_be_Sym*>(&_file[0] + s->sh_offset);
            auto end = sym + count;
            for (sym += 1; sym != end; ++sym) {
                if (ELF64_ST_BIND(sym->st_info) == STB_GLOBAL) {
                    action(sym);
                }
            }
            return;
        }
    }
}

uint32_t ELFLoader::getSymbolValue(std::string name) {
    auto bssfSection = findSectionByName(".bss");
    if (!bssfSection) {
        throw std::runtime_error("no bss section found");
    }
    uint32_t value = 0;
    foreachGlobalSymbol([&](auto sym) {
        if (sym->st_shndx != std::distance(_sections, bssfSection))
            return;
        if (this->getString(sym->st_name) == name)
            value = sym->st_value;
    });
    return value;
}

struct sys_process_param_t {
    big_uint32_t size;
    big_uint32_t magic;
    big_uint32_t version;
    big_uint32_t sdk_version;
    big_int32_t primary_prio;
    big_uint32_t primary_stacksize;
    big_uint32_t malloc_pagesize;
    big_uint32_t ppc_seg;
    big_uint32_t crash_dump_param_addr;
};

ThreadInitInfo ELFLoader::getThreadInitInfo(MainMemory* mm) {
    ThreadInitInfo info;
    info.primaryStackSize = DefaultStackSize;
    for (auto ph = _pheaders; ph != _pheaders + _header->e_phnum; ++ph) {
        if (ph->p_type == PT_TLS) {
            info.tlsBase = &_file[0] + ph->p_offset;
            info.tlsSegmentVa = ph->p_vaddr;
            info.tlsFileSize = ph->p_filesz;
            info.tlsMemSize = ph->p_memsz;
        } else if (ph->p_type == PT_TYPE_PARAMS) {
            if (ph->p_filesz != 0) {
                auto procParam = reinterpret_cast<sys_process_param_t*>(&_file[0] + ph->p_offset);
                info.primaryStackSize = procParam->primary_stacksize;
            }
        }
    }
    info.entryPointDescriptorVa = _header->e_entry;
    return info;
}

std::string ELFLoader::elfName() {
    return _elfName;
}

std::string ELFLoader::shortName() {
    return boost::filesystem::path(_elfName).filename().string();
}

ELFLoader::ELFLoader() {}
ELFLoader::~ELFLoader() {}
