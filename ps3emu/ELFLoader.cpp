#include "ELFLoader.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdexcept>
#include <boost/log/trivial.hpp>

using namespace boost::endian;

static const auto vaTLS = StackBase + StackSize;
static const auto tlsSize = 64 * (1u << 10);
static const uint32_t PT_TYPE_PARAMS = PT_LOOS + 1;
static const uint32_t PT_TYPE_PRXINFO = PT_LOOS + 2;

#pragma pack(1)

struct prx_info {
    big_uint32_t lib_header;
    big_uint16_t lib_id;
    big_uint16_t count;
    big_uint32_t unk0;
    big_uint32_t unk1;
    big_uint32_t prx_name;
    big_uint32_t prx_fnids;
    big_uint32_t prx_stubs;
    big_uint32_t unk2;
    big_uint32_t unk3;
    big_uint32_t unk4;
    big_uint32_t unk5;
};

static_assert(sizeof(prx_info) == 44, "");

struct sys_process_prx_info {
    big_uint32_t header_size;
    big_uint32_t unk0;
    big_uint32_t unk1;
    big_uint32_t unk2;
    big_uint32_t unk3;
    big_uint32_t unk4;
    big_uint32_t prx_infos;
    big_uint32_t padding[9];
};

static_assert(sizeof(sys_process_prx_info) == 64, "");

#pragma pack()

void ELFLoader::load(std::string filePath) {
    _loadedFilePath = filePath;
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

struct fdescr {
    big_uint32_t va;
    big_uint32_t tocBase;
};
static_assert(sizeof(fdescr) == 8, "");

void ELFLoader::map(PPU* ppu, std::vector<std::string> args) {
    ppu->setELFLoader(this);
    for (auto ph = _pheaders; ph != _pheaders + _header->e_phnum; ++ph) {
        if (ph->p_type != PT_LOAD)
            continue;
        if (ph->p_vaddr % ph->p_align != 0)
            throw std::runtime_error("complex alignment not supported");
        if (ph->p_memsz == 0)
            continue;
        
        BOOST_LOG_TRIVIAL(trace) << ssnprintf("mapping segment of size %" PRIx64 " to %" PRIx64 "-%" PRIx64,
            (uint64_t)ph->p_filesz, (uint64_t)ph->p_vaddr, (ph->p_paddr + ph->p_memsz));
        
        assert(ph->p_memsz >= ph->p_filesz);
        ppu->writeMemory(ph->p_vaddr, ph->p_offset + &_file[0], ph->p_filesz, true);
        ppu->setMemory(ph->p_vaddr + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz, true);
    }
    
    auto procParamSectionName = ".sys_proc_param"; // ppu/include/sys/process.h
    auto procParamSection = findSectionByName(procParamSectionName);
    if (procParamSection) {
        BOOST_LOG_TRIVIAL(error) << "proc param section not implemented";
    }
    
    // PPU_ABI-Specifications_e
    ppu->setMemory(StackBase, 0, StackSize, true);
    ppu->setGPR(1, StackBase + StackSize - sizeof(uint64_t));
        
    fdescr entry;
    ppu->readMemory(_header->e_entry, &entry, sizeof(entry));
    ppu->setGPR(2, entry.tocBase);
    
    auto vaArgs = storeArgs(ppu, args);
    ppu->setGPR(3, args.size());
    ppu->setGPR(4, vaArgs);
    
    // undocumented:
    ppu->setGPR(5, StackBase);
    ppu->setGPR(6, 0);
    ppu->setGPR(12, DefaultMainMemoryPageSize);
    
    ppu->setMemory(vaTLS, 0, tlsSize, true);
    ppu->setGPR(13, vaTLS);
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

template <typename Elem>
std::vector<Elem> readSection(Elf64_be_Shdr* sections,
                              Elf64_be_Ehdr* header,
                              uint32_t va,
                              PPU* ppu) {
    auto section = std::find_if(sections, sections + header->e_shnum, [&](auto& sh) {
        return sh.sh_addr == va;
    });
    
    if (section == sections + header->e_shnum)
        throw std::runtime_error("section not present");
    
    if (section->sh_size % sizeof(Elem) != 0)
        throw std::runtime_error("section size inconsistency");
    
    std::vector<Elem> vec(section->sh_size / sizeof(Elem));
    ppu->readMemory(section->sh_addr, &vec[0], section->sh_size);
    return vec;
}

void ELFLoader::link(PPU* ppu) {
    auto phprx = std::find_if(_pheaders, _pheaders + _header->e_phnum, [](auto& ph) {
        return ph.p_type == PT_TYPE_PRXINFO;
    });
    
    if (phprx == _pheaders + _header->e_phnum)
        throw std::runtime_error("PRXINFO segment not present");
    
    auto procPrxInfoVec = readSection<sys_process_prx_info>(
        _sections, _header, phprx->p_vaddr, ppu);
    
    auto prxInfos = readSection<prx_info>(
        _sections, _header, procPrxInfoVec[0].prx_infos, ppu);
    
    auto firstPrxName = std::min_element(prxInfos.begin(), prxInfos.end(), [](auto& l, auto& r) {
        return l.prx_name < r.prx_name;
    });
    auto firstPrxFnid = std::min_element(prxInfos.begin(), prxInfos.end(), [](auto& l, auto& r) {
        return l.prx_fnids < r.prx_fnids;
    });
    auto firstPrxStub = std::min_element(prxInfos.begin(), prxInfos.end(), [](auto& l, auto& r) {
        return l.prx_stubs < r.prx_stubs;
    });
    
    auto prxNames = readSection<char>(
        _sections, _header, firstPrxName->prx_name - 4, ppu);
    
    auto prxFnids = readSection<big_uint32_t>(
        _sections, _header, firstPrxFnid->prx_fnids, ppu);
    
    auto prxStubs = readSection<big_uint32_t>(
        _sections, _header, firstPrxStub->prx_stubs, ppu);
    
    if (prxFnids.size() != prxStubs.size())
        throw std::runtime_error("prx infos/stubs/fnids inconsistency");
    
    uint64_t vaDescr = 0x7f000000;
    ppu->setMemory(vaDescr, 0, prxFnids.size() * 8, true);
    uint32_t curUnknownNcall = 2000;
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("resolving %d rsx modules", prxInfos.size());
    for (auto& info : prxInfos) {
        auto nameOffset = info.prx_name - firstPrxName->prx_name;
        std::string name(&prxNames[nameOffset + 4]);
        BOOST_LOG_TRIVIAL(trace) << "  " << name;
        
        auto firstFnid = (info.prx_fnids - firstPrxFnid->prx_fnids) / sizeof(uint32_t);
        auto firstStub = (info.prx_stubs - firstPrxStub->prx_stubs) / sizeof(uint32_t);
        for (auto i = 0u; i < info.count; ++i) {
            auto fnid = prxFnids.at(firstFnid + i);
            auto& stub = prxStubs.at(firstStub + i);
            
            uint32_t index = 0;
            auto ncallEntry = ppu->findNCallEntry(fnid, index);
            std::string name;
            if (!ncallEntry) {
                index = curUnknownNcall--;
                name = ssnprintf("(!) fnid_%08X", fnid);
            } else {
                name = ncallEntry->name;
            }
            uint32_t ncall = (1 << 26) | index;
            ppu->store<4>(vaDescr, vaDescr + 4);
            ppu->store<4>(vaDescr + 4, ncall);
            BOOST_LOG_TRIVIAL(trace) << ssnprintf("    %x -> %s (ncall %x)",
                stub, name, index);
            stub = vaDescr;
            vaDescr += 8;
        }
    }
    
    ppu->writeMemory(firstPrxStub->prx_stubs, &prxStubs[0], prxStubs.size() * sizeof(big_uint32_t));
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

ps3_uintptr_t ELFLoader::storeArgs(PPU* ppu, std::vector<std::string> const& args) {
    const auto vaArgs = vaTLS + tlsSize;
    std::vector<big_uint64_t> arr;
    ppu->setMemory(vaArgs, 0, (args.size() + 1) * 8, true);
    auto len = 0;
    for (auto arg : args) {
        auto vaPtr = vaArgs + (args.size() + 1) * 8 + len;
        ppu->writeMemory(vaPtr, arg.data(), arg.size() + 1, true);
        arr.push_back(vaPtr);
        len += arg.size() + 1;
    }
    arr.push_back(0);
    ppu->writeMemory(vaArgs, arr.data(), arr.size() * 8);
    return vaArgs;
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
        if (getString(sym->st_name) == name)
            value = sym->st_value;
    });
    return value;
}

std::string ELFLoader::loadedFilePath() {
    return _loadedFilePath;
}

