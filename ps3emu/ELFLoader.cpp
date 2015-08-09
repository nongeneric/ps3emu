#include "ELFLoader.h"
#include <assert.h>
#include <stdio.h>
#include <stdexcept>

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

std::string ELFLoader::getString(uint idx) {
    for (auto s = _sections; s != _sections + _header->e_shnum; ++s) {
        if (s->sh_type == SHT_STRTAB) {
            if (s->sh_size <= idx)
                throw std::runtime_error("out of bounds");
            auto ptr = &_file[0] + s->sh_offset + idx;
            return std::string(reinterpret_cast<char*>(ptr));
        }
    }
    throw std::runtime_error("no string table section present");
}

struct fdescr {
    big_uint32_t va;
    big_uint32_t tocBase;
};
static_assert(sizeof(fdescr) == 8, "bad function descriptor size");

void ELFLoader::map(PPU* ppu) {
    for (auto ph = _pheaders; ph != _pheaders + _header->e_phnum; ++ph) {
        if (ph->p_type != PT_LOAD)
            continue;
        if (ph->p_vaddr % ph->p_align != 0)
            throw std::runtime_error("complex alignment not supported");
        
        assert(ph->p_memsz >= ph->p_filesz);
        ppu->writeMemory(ph->p_vaddr, ph->p_offset + &_file[0], ph->p_filesz);
        ppu->setMemory(ph->p_vaddr + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
    }
    
    // 3.4.1. Registers
    const auto vaStackBase = 0x06000000;
    const auto vaStackSize = 0x4000;
    ppu->setMemory(vaStackBase, 0, vaStackSize);
    ppu->setGPR(1, vaStackBase + vaStackSize - sizeof(uint64_t));
    
    fdescr entry;
    ppu->readMemory(_header->e_entry, &entry, sizeof(entry));
    ppu->setGPR(2, entry.tocBase);
    
    ppu->setGPR(3, 0);
    ppu->setGPR(4, 0);
    ppu->setGPR(5, 0);
    ppu->setGPR(6, 0); // TODO: aux vector
    ppu->setGPR(7, 0);
    ppu->setFPSCR(0);
    
    ppu->setLR(entry.va);
}

void ELFLoader::link(PPU* ppu) {
    for (auto s = _sections; s != _sections + _header->e_shnum; ++s) {
        if (s->sh_type == SHT_SYMTAB) {
            
        }
    }
}

uint64_t ELFLoader::entryPoint() {
    return _header->e_entry;
}
