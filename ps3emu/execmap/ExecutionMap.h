#pragma once

#include "ps3emu/int.h"
#include <vector>
#include <string>
#include <array>
#include <memory>
#include <atomic>
#include <algorithm>
#include <assert.h>

template <int Size>
class ExecutionMap {
    std::unique_ptr<std::array<std::atomic<bool>, Size>> _map;

public:
    ExecutionMap() {
#ifdef EXECMAP_ENABLED
        _map.reset(new std::array<std::atomic<bool>, Size>());
        std::fill(begin(*_map), end(*_map), false);
#endif
    }

    void mark(uint32_t va) {
        if (va >= Size)
            return;
        (*_map)[va].store(true, std::memory_order_relaxed);
    }

    std::vector<uint32_t> dump(uint32_t start, uint32_t len) {
        assert(start < Size);
        auto end = std::min<uint32_t>(start + len, Size);
        std::vector<uint32_t> vec;
        for (auto i = start; i < end; ++i) {
            if ((*_map)[i]) {
                vec.push_back(i);
            }
        }
        return vec;
    }
};
