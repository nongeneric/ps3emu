#pragma once

#include "ps3emu/int.h"
#include <memory>
#include <atomic>
#include <array>
#include <assert.h>

template <unsigned PageSize, unsigned MaxAddress>
class ModificationMap {
    static constexpr uint32_t PageCount = MaxAddress / PageSize;
    static constexpr uint32_t PageShift = log2(PageSize);
    std::unique_ptr<std::array<std::atomic<bool>, PageCount>> _pages;
    
public:
    inline ModificationMap() {
        _pages.reset(new std::array<std::atomic<bool>, PageCount>());
        for (auto& p : *_pages)
            p = 0;
    }
    
    template <int Size>
    inline void mark(uint32_t va) {
        static_assert(Size < PageSize);
        // better be on the safe side
        auto index = va >> PageShift;
        assert(index + 1 < _pages->size());
        (*_pages)[index].store(true, std::memory_order_relaxed);
        (*_pages)[index + 1].store(true, std::memory_order_relaxed);
    }
    
    inline void mark(uint32_t va, uint32_t size) {
        for (auto i = va; i < va + size; ++i) {
            mark<1>(i);
        }
    }
    
    inline bool marked(uint32_t va, uint32_t size) {
        auto page = va >> PageShift;
        auto lastPage = page + ((va + size) >> PageShift) + 1;
        assert(lastPage < _pages->size());
        for (; page < lastPage; page++) {
            if ((*_pages)[page].load(std::memory_order_relaxed))
                return true;
        }
        return false;
    }
    
    inline void reset(uint32_t va, uint32_t size) {
        auto page = va >> PageShift;
        auto lastPage = page + ((va + size) >> PageShift) + 1;
        assert(lastPage < _pages->size());
        for (; page < lastPage; page++) {
            (*_pages)[page].store(false, std::memory_order_relaxed);
        }
    }
};
