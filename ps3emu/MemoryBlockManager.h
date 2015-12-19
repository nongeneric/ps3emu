#pragma once

#include <bitset>
#include <assert.h>

template <uint32_t Base, uint32_t Size, uint32_t BlockSize>
class MemoryBlockManager {
    std::bitset<Size / BlockSize> _blocks;
public:
    ps3_uintptr_t alloc(uint32_t size) {
        for (auto i = 0u; i < _blocks.size(); ++i) {
            if (!_blocks[i]) {
                return Base + i * BlockSize;
            }
        }
        throw std::runtime_error("no free space left");
    }
    
    void release(ps3_uintptr_t va, uint32_t size) {
        assert(Base <= va && va < Base + Size);
        auto firstBlock = (va - Base) / BlockSize;
        auto blockCount = ((va - Base) + BlockSize - 1) / BlockSize;
        for (auto i = 0u; i < blockCount; ++i) {
            _blocks[i] = false;
        }
    }
};
