#include "ELFLoader.h"
#include "ImportResolver.h"
#include "ppu/PPUThread.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdexcept>
#include "log.h"
#include <boost/range/algorithm.hpp>

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

void ELFLoader::map(MainMemory* mm, make_segment_t makeSegment, ps3_uintptr_t imageBase) {
    _imageBase = imageBase;
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
            _prxPh1Va = va = ::align(imageBase + ph0.p_vaddr + ph0.p_memsz, 0x10000);
        }
        
        LOG << ssnprintf("mapping segment of size %08" PRIx64 " to %08" PRIx64 "-%08" PRIx64 " (image base: %08x)",
            (uint64_t)ph->p_filesz, va, va + ph->p_memsz, imageBase);
        
        assert(ph->p_memsz >= ph->p_filesz);
        mm->writeMemory(va, ph->p_offset + &_file[0], ph->p_filesz, true);
        mm->setMemory(va + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz, true);
        
        makeSegment(va, ph->p_memsz, index);
    }
}

Elf64_be_Shdr* ELFLoader::findSectionByName(std::string name) {
    for (auto s = _sections; s != _sections + _header->e_shnum; ++s) {
        if (getSectionName(s->sh_name) == name)
            return s;
    }
    return nullptr;
}

template <typename Elem>
std::vector<Elem> readSection(Elf64_be_Shdr* sections,
                              Elf64_be_Ehdr* header,
                              uint32_t va,
                              MainMemory* mm) {
    auto section = std::find_if(sections, sections + header->e_shnum, [&](auto& sh) {
        return sh.sh_addr == va;
    });
    
    if (section == sections + header->e_shnum)
        throw std::runtime_error("section not present");
    
    if (section->sh_size % sizeof(Elem) != 0)
        throw std::runtime_error("section size inconsistency");
    
    std::vector<Elem> vec(section->sh_size / sizeof(Elem));
    mm->readMemory(section->sh_addr, &vec[0], section->sh_size);
    return vec;
}

void ELFLoader::link(MainMemory* mm) {
    auto phprx = std::find_if(_pheaders, _pheaders + _header->e_phnum, [](auto& ph) {
        return ph.p_type == PT_TYPE_PRXINFO;
    });
    
    if (phprx == _pheaders + _header->e_phnum)
        throw std::runtime_error("PRXINFO segment not present");
    
    sys_process_prx_info prxInfo;
    mm->readMemory(phprx->p_vaddr, &prxInfo, sizeof(prxInfo));
    
    auto prxInfos = readSection<prx_info>(
        _sections, _header, prxInfo.prx_infos, mm);
    
    _resolver.reset(new ImportResolver(mm));
    _resolver->populate(this, &prxInfos[0], prxInfos.size());
    
    uint32_t curUnknownNcall = 2000;
    LOG << ssnprintf("resolving %d rsx modules", prxInfos.size());
    for (auto& lib : _resolver->libraries()) {
        LOG << ssnprintf("  %s", lib.name());
        for (auto& entry : lib.entries()) {
            uint32_t index = 0;
            auto ncallEntry = findNCallEntry(entry.fnid(), index);
            std::string name;
            if (!ncallEntry) {
                index = curUnknownNcall--;
                name = ssnprintf("(!) fnid_%08X", entry.fnid());
            } else {
                name = ncallEntry->name;
            }
            LOG << ssnprintf("    %x -> %s (ncall %x)", entry.stub(), name, index);
            entry.resolveNcall(index, !ncallEntry);
        }
    }
}

void ELFLoader::relink(MainMemory* mm, std::vector<ELFLoader*> prxs, ps3_uintptr_t prxImageBase) {
    auto newPrx = prxs.back();
    assert(newPrx->_prxPh1Va);
    INFO(libs) << ssnprintf("relinking prx %s", newPrx->elfName());
    
    auto module =
        reinterpret_cast<module_info_t*>(&newPrx->_file[newPrx->_pheaders->p_paddr]);
    auto imports = reinterpret_cast<prx_import_t*>(
        &newPrx->_file[newPrx->_pheaders->p_offset + module->imports_start]);
    auto nimports = (module->imports_end - module->imports_start) / sizeof(prx_import_t);
    auto importLibCount = _resolver->libraries().size();
    auto ph1va = newPrx->_pheaders[1].p_vaddr;
    _resolver->populate(
        newPrx, imports, nimports, prxImageBase, ph1va, newPrx->_prxPh1Va);
    
    auto& importLibs = _resolver->libraries();
    // patch imports of newPrx
    for (auto i = importLibCount; i < importLibs.size(); ++i) {
        for (auto& entry : importLibs[i].entries()) {
            if (entry.type() == prx_symbol_type_t::function) {
                auto code = mm->load<4>(entry.stub());
                auto orisImmVa = prxImageBase + code + 6;
                auto orisImm = mm->load<2>(orisImmVa);
                mm->store<2>(orisImmVa, orisImm + (newPrx->_prxPh1Va >> 16));
                auto lwzImmVa = prxImageBase + code + 10;
                auto lwzImm = mm->load<2>(lwzImmVa);
                mm->store<2>(lwzImmVa, lwzImm - ph1va);
            } else if (entry.type() == prx_symbol_type_t::variable) {
                
            }
        }
    }
    
    auto tocVa = module->toc - 0x8000 + newPrx->_prxPh1Va - newPrx->ph1rva();
    auto tocEntry = mm->load<4>(tocVa);
    tocEntry = tocEntry + newPrx->_prxPh1Va - newPrx->ph1rva();
    mm->store<4>(tocVa, tocEntry);
    
    for (auto prx : prxs) {
        for (auto& exportLib : prx->getExports()) {
            for (auto& importLib : importLibs) {
                auto& importEntries = importLib.entries();
                for (auto& exportEntry : exportLib.entries) {
                    auto importEntry = boost::find_if(importEntries, [=](auto& ie) {
                        return ie.fnid() == exportEntry.fnid;
                    });
                    if (importEntry == end(importEntries))
                        continue;
                    auto stubVa = exportEntry.stubVa + prx->_imageBase;
                    mm->store<4>(stubVa, exportEntry.stub.va + prx->_imageBase);
                    mm->store<4>(stubVa + 4, exportEntry.stub.tocBase + prx->_imageBase);
                    assert(importEntry->type() == exportEntry.type);
                    importEntry->resolve(stubVa);
                    INFO(libs) << ssnprintf("  %x -> %x (fnid_%08x)",
                                            importEntry->stub(),
                                            stubVa,
                                            importEntry->fnid());
                }
            }
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

std::vector<prx_export_lib_t> ELFLoader::getExports() {
    auto module = reinterpret_cast<module_info_t*>(&_file[_pheaders->p_paddr]);
    auto exports = reinterpret_cast<prx_export_t*>(&_file[_pheaders->p_offset + module->exports_start]);
    auto nexports = (module->exports_end - module->exports_start) / sizeof(prx_export_t);
    std::vector<prx_export_lib_t> vec;
    for (auto i = 0u; i < nexports; ++i) {
        auto& e = exports[i];
        auto fnids = reinterpret_cast<big_uint32_t*>(
            &_file[_pheaders->p_offset + e.fnid_table]);
        auto stubs = reinterpret_cast<big_uint32_t*>(
            &_file[_pheaders->p_offset + e.stub_table]);
        prx_export_lib_t lib;
        lib.name = e.name;
        for (auto i = 0; i < e.functions + e.variables + e.tls_variables; ++i) {
            prx_export_info_t info;
            info.stubVa = stubs[i];
            info.stub = *reinterpret_cast<fdescr*>(&_file[_pheaders->p_offset + stubs[i]]);
            info.fnid = fnids[i];
            info.type = i < e.functions ? prx_symbol_type_t::function
                      : i < e.functions + e.variables ? prx_symbol_type_t::variable
                      : prx_symbol_type_t::tls_variable;
            lib.entries.push_back(info);
        }
        vec.push_back(lib);
    }
    return vec;
}

std::string ELFLoader::elfName() {
    return _elfName;
}

ImportResolver* ELFLoader::resolver() {
    return _resolver.get();
}

ps3_uintptr_t ELFLoader::ph1rva() {
    assert(_header->e_phnum > 1);
    return _pheaders[1].p_vaddr;
}

ELFLoader::ELFLoader() {}
ELFLoader::~ELFLoader() {}
