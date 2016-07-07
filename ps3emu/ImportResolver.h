#pragma once

#include "ELFLoader.h"
#include "constants.h"
#include <string>
#include <vector>

class MainMemory;

enum class ImportResolution {
    ncall,
    prx,
    ncallStub,
    unresolved
};

class ImportEntry {
    MainMemory* _mm;
    uint32_t _fnid;
    ps3_uintptr_t _stub;
    ps3_uintptr_t _descrVa; // only used for ncalls, _stub points to it
    ImportResolution _resolution;
    prx_symbol_type_t _type;
    
public:
    ImportEntry(MainMemory* mm,
                uint32_t fnid,
                ps3_uintptr_t stub,
                ps3_uintptr_t descrVa,
                prx_symbol_type_t type);
    void resolveNcall(uint32_t index, bool stub);
    void resolve(ps3_uintptr_t exportStubVa);
    uint32_t fnid();
    ps3_uintptr_t stub();
    ImportResolution resolution();
    prx_symbol_type_t type();
};

class ImportedLibrary {
    MainMemory* _mm;
    ELFLoader* _elf;
    std::string _name;
    std::vector<ImportEntry> _entries;
    ps3_uintptr_t _descrVa;
    
public:
    ImportedLibrary(MainMemory* mm,
                    ELFLoader* elf,
                    prx_info* info,
                    ps3_uintptr_t descrVa);
    ImportedLibrary(MainMemory* mm,
                    ELFLoader* elf,
                    prx_import_t* info,
                    ps3_uintptr_t descrVa,
                    ps3_uintptr_t imageBase,
                    ps3_uintptr_t ph1rva,
                    ps3_uintptr_t ph1va);
    ELFLoader* elf();
    std::string name();
    std::vector<ImportEntry>& entries();
    ps3_uintptr_t lastDescrVa();
};

class ImportResolver {
    MainMemory* _mm;
    std::vector<ImportedLibrary> _libraries;
    ps3_uintptr_t _descrVa;
    
public:
    ImportResolver(MainMemory* mm);
    void populate(ELFLoader* elf, prx_info* infos, size_t count);
    void populate(ELFLoader* elf,
                  prx_import_t* infos,
                  size_t count,
                  ps3_uintptr_t imageBase,
                  ps3_uintptr_t ph1rva,
                  ps3_uintptr_t ph1va);
    std::vector<ImportedLibrary>& libraries();
};
