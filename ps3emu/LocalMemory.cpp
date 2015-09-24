#include "LocalMemory.h"

#include "constants.h"
#include "PPU.h"

LocalMemory::LocalMemory(PPU* ppu) : _ppu(ppu) {
    
}

uint32_t LocalMemory::load(uint32_t offset) {
    auto mainBase = GcmLocalMemoryBase;
    for (auto& m : _ioMap) {
        if (m.offset <= offset && offset < m.offset + m.size) {
            mainBase = m.mainMemory + m.offset;
        }
    }
    return _ppu->load<4>(mainBase + offset);
}

void LocalMemory::mapIO(uint32_t offset, ps3_uintptr_t mainMemory, uint32_t size) {
    _ioMap.push_back({ offset, mainMemory, size });
}

void LocalMemory::store(uint32_t offset, uint32_t value) {
    for (auto& m : _ioMap) {
        if (m.offset <= offset && offset < m.offset + m.size) {
            _ppu->store<4>(m.mainMemory - m.offset, value);
        }
    }
    _ppu->store<4>(GcmLocalMemoryBase + offset, value);
}
