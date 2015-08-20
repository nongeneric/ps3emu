#include "ELFLoader.h"
#include <assert.h>
#include <stdio.h>
#include <stdexcept>
#include <boost/format.hpp>

using namespace boost::endian;

ELFLoader::ELFLoader()
{

}

ELFLoader::~ELFLoader()
{

}

void ELFLoader::load(std::string filePath) {
    FILE* f = fopen(filePath.c_str(), "rb");
    assert(f);
    fseek(f, 0, SEEK_END);
    auto fileSize = static_cast<unsigned>(ftell(f));
    if (fileSize < sizeof(Elf64_Ehdr))
        throw std::runtime_error("File too small for an ELF64 file");
    
    fseek(f, 0, SEEK_SET);
    
    _file.resize(fileSize);
    auto read = fread(&_file[0], 1, fileSize, f);
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
    
    if (_header->e_ident[EI_DATA] != ELFDATA2MSB)
        throw std::runtime_error("Only big endian supported");
    
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

struct fdescr {
    big_uint32_t va;
    big_uint32_t tocBase;
};
static_assert(sizeof(fdescr) == 8, "bad function descriptor size");

void ELFLoader::map(PPU* ppu, std::function<void(std::string)> log) {
    for (auto ph = _pheaders; ph != _pheaders + _header->e_phnum; ++ph) {
        if (ph->p_type != PT_LOAD)
            continue;
        if (ph->p_vaddr % ph->p_align != 0)
            throw std::runtime_error("complex alignment not supported");
        if (ph->p_memsz == 0)
            continue;
        
        log(str(boost::format("mapping segment of size %x to %x-%x")
            % ph->p_filesz % ph->p_vaddr % (ph->p_paddr + ph->p_memsz)
        ));
        
        assert(ph->p_memsz >= ph->p_filesz);
        ppu->writeMemory(ph->p_vaddr, ph->p_offset + &_file[0], ph->p_filesz, true);
        ppu->setMemory(ph->p_vaddr + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz, true);
    }
    
    auto procParamSectionName = ".sys_proc_param"; // ppu/include/sys/process.h
    auto procParamSection = findSectionByName(procParamSectionName);
    if (procParamSection) {
        throw std::runtime_error("proc param section not implemented");
    }
    
    // PPU_ABI-Specifications_e
    const auto vaStackBase = 0x06000000;
    const auto vaStackSize = 0x10000;
    ppu->setMemory(vaStackBase, 0, vaStackSize, true);
    ppu->setGPR(1, vaStackBase + vaStackSize - sizeof(uint64_t));
        
    fdescr entry;
    ppu->readMemory(_header->e_entry, &entry, sizeof(entry));
    ppu->setGPR(2, entry.tocBase);
    
    ppu->setGPR(3, 0);
    ppu->setGPR(4, vaStackBase);
    
    // undocumented:
    ppu->setGPR(5, vaStackBase);
    ppu->setGPR(6, 0);
    
    ppu->setGPR(13, 0); //TODO: control block of tls
    ppu->setFPSCR(0);
    ppu->setNIP(entry.va);
}

Elf64_be_Shdr* ELFLoader::findSectionByName(std::string name) {
    for (auto s = _sections; s != _sections + _header->e_shnum; ++s) {
        if (getSectionName(s->sh_name) == name)
            return s;
    }
    return nullptr;
}

struct LGFEntry {
    big_uint32_t prxStub;
    big_uint32_t va;
    big_uint32_t fnid;
    big_uint32_t stub;
    big_uint32_t sym;
};

void ELFLoader::link(PPU* ppu, std::function<void(std::string)> log) {
    auto lgfSectionName = ".debug_libgenfunc";
    auto lgfSection = findSectionByName(lgfSectionName);
    if (!lgfSection) {
        log("no libgenfunc section found, nothing to do");
        return;
    }
    auto entries = reinterpret_cast<LGFEntry*>(&_file[0] + lgfSection->sh_offset);
    auto count = lgfSection->sh_size / sizeof(LGFEntry);
    log(str(boost::format("%d LGF entries found in %s")
        % count % getSectionName(lgfSection->sh_name)));
    uint64_t vaFDescrs = 0x7f000000;
    ppu->setMemory(vaFDescrs, 0, count * 2, true);
    for (auto i = 0u; i < count; ++i) {
        uint32_t lgfIdx = std::distance(_sections, lgfSection);
        auto symbol = getGlobalSymbolByValue(entries[i].sym, lgfIdx);
        if (symbol) {
            auto sname = getString(symbol->st_name);
            auto vaFDescr = vaFDescrs + i * 8;
            auto index = ppu->findNCallEntryIndex(sname);
            uint32_t ncall = (1 << 26) | index;
            ppu->store<4>(vaFDescr, vaFDescr + 4);
            ppu->store<4>(vaFDescr + 4, ncall); // use tocbase to place ncall instruction
            ppu->store<4>(entries[i].va, vaFDescr);
            log(str(boost::format("resolved %x to point to %s (ncall %d)") 
                % entries[i].va % sname % index));
        }
    }
}

uint64_t ELFLoader::entryPoint() {
    return _header->e_entry;
}

Elf64_be_Sym* ELFLoader::getGlobalSymbolByValue(uint32_t value, uint32_t section) {
    for (auto s = _sections; s != _sections + _header->e_shnum; ++s) {
        if (s->sh_type == SHT_SYMTAB) {
            if (s->sh_entsize != sizeof(Elf64_be_Sym))
                throw std::runtime_error("bad symbol table");
            int count = s->sh_size / s->sh_entsize;
            auto sym = reinterpret_cast<Elf64_be_Sym*>(&_file[0] + s->sh_offset);
            auto end = sym + count;
            for (sym += 1; sym != end; ++sym) {
                if (sym->st_value == value && 
                    ELF64_ST_BIND(sym->st_info) == STB_GLOBAL &&
                    (sym->st_shndx == section || section == ~0u))
                {
                    return sym;
                }
            }
            return nullptr;
        }
    }
    throw std::runtime_error("no symbol table section present");
}
