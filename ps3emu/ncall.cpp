#include "PPU.h"
#include "../libs/sys.h"

struct NCallEntry {
    const char* name;
    void (*stub)(PPU*);
};

void nstub_sys_initialize_tls(PPU* ppu) {
    
}

#define ENTRY(name) { #name, nstub_##name }

NCallEntry ncallTable[] {
    { "", nullptr },
    ENTRY(sys_initialize_tls),
};

void PPU::ncall(uint32_t index) {
    if (index == 0)
        throw std::runtime_error("unknown index");
    ncallTable[index].stub(this);
    setNIP(getLR());
}

uint32_t PPU::findNCallEntryIndex(std::string name) {
    auto count = sizeof(ncallTable) / sizeof(NCallEntry);
    for (uint32_t i = 0; i < count; ++i) {
        if (ncallTable[i].name == name)
            return i;
    }
    return 0;
}