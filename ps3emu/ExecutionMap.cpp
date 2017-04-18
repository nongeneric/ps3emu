#include "ExecutionMap.h"

#include <sstream>
#include <algorithm>
#include <set>
#include "ps3emu/utils.h"
#include <assert.h>

ExecutionMap::ExecutionMap() {
    _map.reset(new std::array<std::atomic<bool>, size>());
    std::fill(begin(*_map), end(*_map), false);
}

void ExecutionMap::mark(uint32_t va) {
    if (va >= _map->size())
        return;
    (*_map)[va].store(true, std::memory_order_relaxed);
}

std::vector<uint32_t> ExecutionMap::dump(uint32_t start, uint32_t len) {
    assert(start < _map->size());
    auto end = std::min<uint32_t>(start + len, _map->size());
    std::vector<uint32_t> vec;
    for (auto i = start; i < end; ++i) {
        if ((*_map)[i]) {
            vec.push_back(i);
        }
    }
    return vec;
}

std::string serializeEntries(std::vector<uint32_t> const& vec) {
    std::string res;
    std::set<uint32_t> set(begin(vec), end(vec));
    for (auto va : set) {
        res += ssnprintf("%08x\n", va);
    }
    return res;
}

std::vector<uint32_t> deserializeEntries(std::string const& text) {
    std::vector<uint32_t> vec;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        vec.push_back(std::stoi(line, 0, 16));
    }
    return vec;
}
