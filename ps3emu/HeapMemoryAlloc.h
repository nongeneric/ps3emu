#pragma once

#include "ps3emu/int.h"
#include <boost/thread/mutex.hpp>
#include <memory>
#include <array>
#include <tuple>
#include <map>

class HeapMemoryAlloc {
    boost::mutex _m;
    std::vector<bool> _map;
    std::map<uint32_t, uint32_t> _known;
    
public:
    HeapMemoryAlloc();
    std::tuple<uint8_t*, uint32_t> alloc(uint32_t size, uint32_t alignment);
    void dealloc(uint32_t va);
};
