#include "HeapMemoryAlloc.h"

#include "ps3emu/utils.h"
#include "ps3emu/state.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/constants.h"
#include <assert.h>
#include <boost/align.hpp>

static auto kb64 = 64u << 10u;

HeapMemoryAlloc::HeapMemoryAlloc() : _map(HeapAreaSize / kb64), _available(HeapAreaSize) {
    g_state.mm->mark(HeapArea, HeapAreaSize, false, "heap");
}

std::tuple<uint8_t*, uint32_t> HeapMemoryAlloc::alloc(uint32_t size, uint32_t alignment) {
    assert(alignment == (1u << 20u) || alignment == kb64);
    auto pages = boost::alignment::align_up(size, kb64) / kb64;
    auto lock = boost::unique_lock(_m);
    auto it = findGap(
        begin(_map), end(_map), pages, alignment / kb64, [&](auto it) { return !*it; });
    if (it == end(_map))
        return {nullptr, -1};
    std::fill(it, it + pages, true);
    auto va = HeapArea + std::distance(begin(_map), it) * kb64;
    auto ptr = g_state.mm->getMemoryPointer(va, 0);
    _known[va] = pages;
    assert((va & (alignment - 1)) == 0);
    _available -= pages * kb64;
    return {ptr, va};
}

void HeapMemoryAlloc::dealloc(uint32_t va) {
    auto lock = boost::unique_lock(_m);
    auto it = _known.find(va);
    if (it == end(_known))
        throw std::runtime_error("bad dealloc");
    auto page = (va - HeapArea) / kb64;
    auto first = begin(_map) + page;
    std::fill(first, first + it->second, false);
    _available += it->second * kb64;
    _known.erase(it);
}

uint32_t HeapMemoryAlloc::available() const {
    return _available;
}
