#include "ELFLoader.h"
#include "ppu/PPUThread.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdexcept>
#include "log.h"
#include <boost/range/algorithm.hpp>
#include <boost/filesystem.hpp>

using namespace boost::endian;

static const uint32_t PT_TYPE_PARAMS = PT_LOOS + 1;
static const uint32_t PT_TYPE_PRXINFO = PT_LOOS + 2;

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

void ELFLoader::map(MainMemory* mm, make_segment_t makeSegment, ps3_uintptr_t imageBase) {
    uint32_t prxPh1Va = 0;
    for (auto ph = _pheaders; ph != _pheaders + _header->e_phnum; ++ph) {
        if (ph->p_type != PT_LOAD)
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
        
        makeSegment(va, ph->p_memsz, index);
    }
    
    if (!imageBase)
        return;
    
    assert(prxPh1Va);
    auto rebase = [=](auto& x) { x += imageBase; };
    auto rebase_aux = [=](auto& x) { x += prxPh1Va - _pheaders[1].p_vaddr; };
    
    _module = (module_info_t*)mm->getMemoryPointer(
        _pheaders->p_paddr - _pheaders->p_offset + imageBase, sizeof(module_info_t));
    rebase(_module->imports_start);
    rebase(_module->imports_end);
    rebase(_module->exports_start);
    rebase(_module->exports_end);
    rebase_aux(_module->toc);
    prx_export_t* exports;
    prx_import_t* imports;
    int nexports, nimports;
    std::tie(exports, nexports) = this->exports(mm);
    std::tie(imports, nimports) = this->imports(mm);
    
    for (auto i = 0; i < nexports; ++i) {
        if (exports[i].name)
            rebase(exports[i].name);
        rebase(exports[i].fnid_table);
        rebase(exports[i].stub_table);
        auto totalExports = exports[i].functions + exports[i].variables + exports[i].tls_variables;
        auto stubs = (big_uint32_t*)mm->getMemoryPointer(exports[i].stub_table, 4 * totalExports);
        for (auto j = 0; j < totalExports; ++j) {
            rebase_aux(stubs[j]);
        }
        for (auto j = 0; j < exports[i].functions; ++j) {
            auto descr = (fdescr*)mm->getMemoryPointer(stubs[j], sizeof(fdescr));
            rebase(descr->va);
            rebase_aux(descr->tocBase);
        }
        if (exports[i].name) {
            auto tocs = (big_uint32_t*)mm->getMemoryPointer(_module->toc - 0x8000, 4 * exports[i].variables);
            for (auto j = 0; j < exports[i].variables; ++j) {
                rebase_aux(tocs[j]);
            }
        }
    }
    
    for (auto i = 0; i < nimports; ++i) {
        rebase(imports[i].name);
        rebase(imports[i].fnids);
        rebase_aux(imports[i].fstubs);
        rebase(imports[i].vnids);
        rebase(imports[i].vstubs);
        auto vstubs = (big_uint32_t*)mm->getMemoryPointer(imports[i].vstubs, 4 * imports[i].variables);
        for (auto j = 0; j < imports[i].variables; ++j) {
            rebase(vstubs[j]);
            auto descr = (vdescr*)mm->getMemoryPointer(vstubs[j], sizeof(vdescr));
            rebase_aux(descr->toc);
        }
        auto fstubs = (big_uint32_t*)mm->getMemoryPointer(imports[i].fstubs, 4 * imports[i].functions);
        for (auto j = 0; j < imports[i].functions; ++j) {
            rebase(fstubs[j]);
            auto orisImm = (big_uint16_t*)mm->getMemoryPointer(fstubs[j] + 6, 2);
            auto lwzImm = (big_uint16_t*)mm->getMemoryPointer(fstubs[j] + 10, 2);
            *orisImm += prxPh1Va >> 16;
            *lwzImm -= _pheaders[1].p_vaddr;
        }
    }
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

uint32_t findExportedSymbol(MainMemory* mm,
                            std::vector<std::shared_ptr<ELFLoader>> const& prxs,
                            uint32_t id,
                            std::string library,
                            prx_symbol_type_t type) {
    static auto curUnknownNcall = 2000;
    static auto ncallDescrVa = FunctionDescriptorsVa;
    for (auto prx : prxs) {
        prx_export_t* exports;
        int nexports;
        std::tie(exports, nexports) = prx->exports(mm);
        for (auto i = 0; i < nexports; ++i) {
            if (!exports[i].name)
                continue;
            std::string name;
            readString(mm, exports[i].name, name);
            if (name != library)
                continue;
            auto fnids = (big_uint32_t*)mm->getMemoryPointer(exports[i].fnid_table, 4 * exports[i].functions);
            auto stubs = (big_uint32_t*)mm->getMemoryPointer(exports[i].stub_table, 4 * exports[i].functions);
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
                    return stubs[j];
                }
            }
        }
    }
    
    if (type == prx_symbol_type_t::function) {
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
        
        mm->setMemory(ncallDescrVa, 0, 8, true);
        mm->store<4>(ncallDescrVa, ncallDescrVa + 4);
        encodeNCall(mm, ncallDescrVa + 4, index);
        ncallDescrVa += 8;
        return ncallDescrVa - 8;
    }
    
    return 0;
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
            auto symbol = findExportedSymbol(mm, prxs, vnids[j], library, prx_symbol_type_t::variable);
            mm->store<4>(descr->toc, symbol);
        }
        auto fstubs = (big_uint32_t*)mm->getMemoryPointer(imports[i].fstubs, imports[i].functions);
        auto fnids = (big_uint32_t*)mm->getMemoryPointer(imports[i].fnids, imports[i].functions);
        for (auto j = 0; j < imports[i].functions; ++j) {
            fstubs[j] = findExportedSymbol(mm, prxs, fnids[j], library, prx_symbol_type_t::function);
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
