#include "PPU.h"
#include "../libs/sys.h"

struct NCallEntry {
    const char* name;
    void (*stub)(PPU*);
};

void nstub_sys_initialize_tls(PPU* ppu) {
    
}

void nstub_sys_lwmutex_create(PPU* ppu) {
    sys_lwmutex_t a1;
    uint64_t a1va = ppu->getGPR(3);
    ppu->readMemory(a1va, &a1, sizeof(sys_lwmutex_t));
    sys_lwmutex_attribute_t a2;
    uint64_t a2va = ppu->getGPR(4);
    ppu->readMemory(a2va, &a2, sizeof(sys_lwmutex_attribute_t));
    auto r = sys_lwmutex_create(&a1, &a2);
    ppu->writeMemory(a1va, &a1, sizeof(sys_lwmutex_t));
    ppu->writeMemory(a2va, &a2, sizeof(sys_lwmutex_attribute_t));
    ppu->setGPR(3, r);
}

void nstub_sys_lwmutex_destroy(PPU* ppu) {
    sys_lwmutex_t a1;
    uint64_t a1va = ppu->getGPR(3);
    ppu->readMemory(a1va, &a1, sizeof(sys_lwmutex_t));
    auto r = sys_lwmutex_destroy(&a1);
    ppu->writeMemory(a1va, &a1, sizeof(sys_lwmutex_t));
    ppu->setGPR(3, r);
}

void nstub_sys_lwmutex_lock(PPU* ppu) {
    sys_lwmutex_t a1;
    uint64_t a1va = ppu->getGPR(3);
    ppu->readMemory(a1va, &a1, sizeof(sys_lwmutex_t));
    auto a2 = (usecond_t)ppu->getGPR(4);
    auto r = sys_lwmutex_lock(&a1, a2);
    ppu->writeMemory(a1va, &a1, sizeof(sys_lwmutex_t));
    ppu->setGPR(3, r);
}

void nstub_sys_lwmutex_unlock(PPU* ppu) {
    sys_lwmutex_t a1;
    uint64_t a1va = ppu->getGPR(3);
    ppu->readMemory(a1va, &a1, sizeof(sys_lwmutex_t));
    auto r = sys_lwmutex_unlock(&a1);
    ppu->writeMemory(a1va, &a1, sizeof(sys_lwmutex_t));
    ppu->setGPR(3, r);
}

#define ENTRY(name) { #name, nstub_##name }

NCallEntry ncallTable[] {
    { "", nullptr },
    ENTRY(sys_initialize_tls),
    ENTRY(sys_lwmutex_create),
    ENTRY(sys_lwmutex_destroy),
    ENTRY(sys_lwmutex_lock),
    ENTRY(sys_lwmutex_unlock),
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