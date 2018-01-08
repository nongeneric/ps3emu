#include "ELFLoader.h"
#include "ppu/PPUThread.h"
#include "dasm_utils.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdexcept>
#include "log.h"
#include "state.h"
#include "Config.h"
#include "ps3emu/execmap/ExecutionMapCollection.h"
#include "ps3emu/execmap/InstrDb.h"
#include "ps3emu/exports/splicer.h"
#include "InternalMemoryManager.h"
#include "ps3emu/BBCallMap.h"
#include <boost/range/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <map>
#include <dlfcn.h>

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
    if (!f) {
        ERROR(libs) << ssnprintf("can't open file %s: %s", filePath, strerror(errno));
        exit(1);
    }
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

Elf64_be_Ehdr* ELFLoader::header() {
    return _header;
}

struct vdescr {
    big_uint32_t size;
    big_uint32_t toc;
};

struct Replacement {
    std::string lib;
    std::string name;
    std::string proxy;
};

std::vector<StolenFuncInfo> ELFLoader::map(make_segment_t makeSegment,
                                           ps3_uintptr_t imageBase,
                                           std::vector<std::string> x86paths,
                                           RewriterStore* store,
                                           bool rewriter) {
    uint32_t prxPh1Va = 0;
    for (auto ph = _pheaders; ph != _pheaders + _header->e_phnum; ++ph) {
        if (ph->p_type != PT_LOAD && ph->p_type != PT_TLS)
            continue;
        if (ph->p_vaddr % ph->p_align != 0) {
            WARNING(libs) << "complex alignment not supported";
        }
        if (ph->p_memsz == 0)
            continue;
        
        uint64_t va = imageBase + ph->p_vaddr;
        auto index = std::distance(_pheaders, ph);
        if (imageBase && index == 1) {
            assert(_header->e_phnum == 3);
            auto ph0 = _pheaders[0];
            prxPh1Va = va = ::align(imageBase + ph0.p_vaddr + ph0.p_memsz, 0x10000);
        }
        
        INFO(libs) << ssnprintf("mapping segment of size %08" PRIx64 " to %08" PRIx64 "-%08" PRIx64 " (image base: %08x)",
            (uint64_t)ph->p_filesz, va, va + ph->p_memsz, imageBase);
        
        g_state.mm->mark(va,
                         ph->p_memsz,
                         false, //!(ph->p_flags & PF_W),
                         ssnprintf("%s segment", shortName()));
        
        assert(ph->p_memsz >= ph->p_filesz);
        g_state.mm->writeMemory(va, ph->p_offset + &_file[0], ph->p_filesz);
        g_state.mm->setMemory(va + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
        
        if (ph->p_type != PT_TLS) {
            makeSegment(va, ph->p_memsz, index);
        }
    }
    
    InstrDb db;
    db.open();
    std::map<uint32_t, uint32_t> bbBytes;
    for (auto& x86path : x86paths) {
        if (!boost::filesystem::exists(x86path))
            throw std::runtime_error("x86 doesn't exist");
        auto handle = dlopen(x86path.c_str(), RTLD_NOW);
        if (handle) {
            auto info = (RewrittenSegmentsInfo*)dlsym(handle, "info");
            info->init();
            int segmentNumber = 0;
            for (auto& segment : info->segments) {
                int index = 0;
                if (segment.spuEntryPoint) {
                    index = store->addSPU(&segment);
                } else {
                    index = store->addPPU(&segment);
                }
                INFO(libs) << ssnprintf("loading %s:%s with %d blocks",
                                        x86path,
                                        segment.description,
                                        segment.blocks->size());
                for (auto i = 0u; i < segment.blocks->size(); ++i) {
                    auto instr = asm_bb_call(
                        segment.spuEntryPoint ? SPU_BB_CALL_OPCODE : BB_CALL_OPCODE, index, i);
                    auto va = (*segment.blocks)[i].va;
                    bbBytes[va] = g_state.mm->load32(va);
                    if (segment.spuEntryPoint) {
                        g_state.mm->store32(va, instr, g_state.granule);
                    } else {
                        g_state.bbcallMap->set(va, instr);
                    }
                }
                auto entry = db.findSpuEntry(_elfName, segmentNumber);
                if (entry && segment.spuEntryPoint) {
                    g_state.executionMaps->spu.emplace_back(*entry, ExecutionMap<LocalStorageSize>());
                    segmentNumber++;
                }
            }
        } else {
            WARNING(libs) << ssnprintf("could not load %s: %s", x86path, dlerror());
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
            g_state.mm->store32(offset, val, g_state.granule);
        } else if (type == R_PPC64_ADDR16_LO) {
            g_state.mm->store16(offset, lo(val), g_state.granule);
        } else if (type == R_PPC64_ADDR16_HI) {
            g_state.mm->store16(offset, hi(val), g_state.granule);
        } else if (type == R_PPC64_ADDR16_HA) {
            g_state.mm->store16(offset, ha(val), g_state.granule);
        } else if (type == R_PPC64_ADDR16_LO_DS) {
            g_state.mm->store16(offset, lo(val) >> 2, g_state.granule);
        } else {
            throw std::runtime_error("unimplemented reloc type");
        }
    }
    
    _module = (module_info_t*)g_state.mm->getMemoryPointer(
        _pheaders->p_paddr - _pheaders->p_offset + imageBase, sizeof(module_info_t));
    
    std::vector<Replacement> replacements = {
        { "cellSpurs", "cellSpursInitializeWithAttribute2" },
        { "cellSpurs", "cellSpursInitializeWithAttribute" },
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
        { "sysPrxForUser", "_sys_memcpy" },
        { "sysPrxForUser", "_sys_memset" },
    };
    
    std::vector<StolenFuncInfo> stolenInfos;
    for (auto&& repl : replacements) {
        auto fnid = calcFnid(repl.name.c_str());
        auto stub = findExportedSymbol({this}, fnid, repl.lib, prx_symbol_type_t::function);
        if (stub == 0)
            continue;
        
        auto codeVa = g_state.mm->load32(stub);
        auto it = bbBytes.find(codeVa);
        if (it != end(bbBytes)) {
            auto bytes = it->second;
            g_state.mm->store32(codeVa, bytes, g_state.granule);
        }
        
        uint32_t index;
        findNCallEntry(fnid, index, true);
        stolenInfos.push_back({codeVa, stub, g_state.mm->load32(codeVa), index});
        encodeNCall(g_state.mm, codeVa, index);
        g_state.bbcallMap->set(codeVa, 0);
    }
    
    // install logging proxies
    if (!rewriter) {
        auto [exports, nexports] = this->exports();
        for (auto i = 0; i < nexports; ++i) {
            auto fnids = (big_uint32_t*)g_state.mm->getMemoryPointer(exports[i].fnid_table, 4 * exports[i].functions);
            auto stubs = (big_uint32_t*)g_state.mm->getMemoryPointer(exports[i].stub_table, 4 * exports[i].functions);
            for (auto j = 0; j < exports[i].functions; ++j) {
                auto fnidName = fnidToName(fnids[j]);
                auto name = fnidName ? *fnidName : ssnprintf("fnid_%08X", fnids[j]);
                std::string libname;
                readString(g_state.mm, exports[i].name, libname);
                auto replacement = std::find_if(begin(replacements), end(replacements), [&](auto& r) {
                    return r.name == name && r.lib == libname;
                });
                if (replacement != end(replacements))
                    continue;
                auto codeVa = g_state.mm->load32(stubs[j]);
                auto isSync = name == "cellSyncMutexLock" || name == "cellSyncMutexUnlock" ||
                              name == "cellSyncMutexTryLock";
                spliceFunction(codeVa, [=] {
                    auto message = ssnprintf("proxy [%08x] %s.%s", codeVa, libname, name);
                    if (isSync) {
                        INFO(libs, sync) << message;
                    } else {
                        INFO(libs) << message;
                    }
                });
            }
        }
    }
    
    return stolenInfos;
}

std::tuple<prx_import_t*, int> ELFLoader::prxImports() {
    auto count = _module->imports_end - _module->imports_start;
    return std::make_tuple(
        (prx_import_t*)g_state.mm->getMemoryPointer(_module->imports_start, count),
        count / sizeof(prx_import_t));
}

std::tuple<prx_import_t*, int> ELFLoader::imports() {
    if (_module)
        return prxImports();
    return elfImports();
}

std::tuple<prx_export_t*, int> ELFLoader::exports() {
    if (!_module)
        return {nullptr, 0};
    
    auto count = _module->exports_end - _module->exports_start;
    return std::make_tuple(
        (prx_export_t*)g_state.mm->getMemoryPointer(_module->exports_start, count),
        count / sizeof(prx_export_t));
}

bool isSymbolWhitelisted(ELFLoader* prx, uint32_t id) {
    auto name = prx->shortName();
    if (name == "liblv2.prx" || name == "libsre.prx" || name == "libsync2.prx" ||
        name == "libfiber.prx") {
        return true;
    }
    if (name == "libgcm_sys.sprx.elf" || name == "libsysutil_game.sprx.elf" ||
        name == "libsysutil.sprx.elf" || name == "libio.sprx.elf" ||
        name == "libaudio.sprx.elf" || name == "libfs.sprx.elf" ||
        name == "libsysutil_np_trophy.sprx.elf" ||
        name == "libsysutil_np.sprx.elf" || name == "libnetctl.sprx.elf" ||
        name == "libnet.sprx.elf" || name == "libgem.sprx.elf" ||
        name == "libcamera.sprx.elf") {
        return false;
    }
    return true;
}

uint32_t findExportedEmuFunction(uint32_t id) {
    static auto curUnknownNcall = 20000;
    static uint32_t ncallDescrVa = 0;
    if (ncallDescrVa == 0) {
        g_state.memalloc->allocInternalMemory(&ncallDescrVa, 1u << 20u, 4);
    }
    uint32_t index;
    auto ncallEntry = findNCallEntry(id, index);
    std::string name;
    if (!ncallEntry) {
        auto resolved = fnidToName(id);
        index = curUnknownNcall--;
        name = ssnprintf("(!) fnid_%08X%s", id, resolved ? " " + *resolved : "");
    } else {
        name = ncallEntry->name;
    }
    INFO(libs) << ssnprintf("    %s (ncall %x)", name, index);
    
    g_state.mm->setMemory(ncallDescrVa, 0, 8);
    g_state.mm->store32(ncallDescrVa, ncallDescrVa + 4, g_state.granule);
    encodeNCall(g_state.mm, ncallDescrVa + 4, index);
    ncallDescrVa += 8;
    return ncallDescrVa - 8;
}

uint32_t findExportedSymbol(std::vector<ELFLoader*> const& prxs,
                            uint32_t id,
                            std::string library,
                            prx_symbol_type_t type) {
    for (auto prx : prxs) {
        auto [exports, nexports] = prx->exports();
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

std::tuple<prx_import_t*, int> ELFLoader::elfImports() {
    auto phprx = std::find_if(_pheaders, _pheaders + _header->e_phnum, [](auto& ph) {
        return ph.p_type == PT_TYPE_PRXINFO;
    });
    
    if (phprx == _pheaders + _header->e_phnum)
        throw std::runtime_error("PRXINFO segment not present");
    
    auto prxInfo = (sys_process_prx_info*)g_state.mm->getMemoryPointer(phprx->p_vaddr, sizeof(sys_process_prx_info));
    auto count = prxInfo->imports_end - prxInfo->imports_start;
    return std::make_tuple(
        (prx_import_t*)g_state.mm->getMemoryPointer(prxInfo->imports_start, count),
        count / sizeof(prx_import_t));
}

void ELFLoader::link(std::vector<std::shared_ptr<ELFLoader>> prxs) {
    INFO(libs) << ssnprintf("linking prx %s", elfName());
    auto [imports, count] = this->imports();
    for (auto i = 0; i < count; ++i) {
        std::string library;
        readString(g_state.mm, imports[i].name, library);
        auto vstubs = (big_uint32_t*)g_state.mm->getMemoryPointer(imports[i].vstubs, 4 * imports[i].variables);
        auto vnids = (big_uint32_t*)g_state.mm->getMemoryPointer(imports[i].vnids, 4 * imports[i].variables);
        for (auto j = 0; j < imports[i].variables; ++j) {
            auto descr = (vdescr*)g_state.mm->getMemoryPointer(vstubs[j], sizeof(vdescr));
            auto symbol = findExportedSymbol(prxs, vnids[j], library, prx_symbol_type_t::variable);
            g_state.mm->store32(descr->toc, symbol, g_state.granule);
        }
        auto fstubs = (big_uint32_t*)g_state.mm->getMemoryPointer(imports[i].fstubs, imports[i].functions);
        auto fnids = (big_uint32_t*)g_state.mm->getMemoryPointer(imports[i].fnids, imports[i].functions);
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

ThreadInitInfo ELFLoader::getThreadInitInfo() {
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

Elf64_be_Phdr* ELFLoader::pheaders() {
    return _pheaders;
}

ELFLoader::ELFLoader() {}
ELFLoader::~ELFLoader() {}
