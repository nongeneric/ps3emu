#pragma once

#include <stdint.h>
#include <vector>
#include "constants.h"

struct MemoryMapping {
    uint32_t offset;
    ps3_uintptr_t mainMemory;
    uint32_t size;
};

class PPU;
class LocalMemory {
    PPU* _ppu;
    std::vector<MemoryMapping> _ioMap;
public:
    LocalMemory(PPU* ppu);
    void mapIO(uint32_t offset, ps3_uintptr_t mainMemory, uint32_t size);
    uint32_t load(uint32_t offset);
    void store(uint32_t offset, uint32_t value);
};