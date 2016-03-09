#include "heap.h"
#include "../ps3emu/MainMemory.h"
#include "../ps3emu/IDMap.h"
#include <boost/log/trivial.hpp>

namespace {

class SysHeap {
    ps3_uintptr_t _cur;
    ps3_uintptr_t _end;
public:
    SysHeap(uint32_t size, MainMemory* mm) {
        assert(size < (1 << 20));
        _cur = mm->malloc(1 << 20);
        _end = _cur + size;
    }
    
    ps3_uintptr_t alloc(uint32_t size) {
        auto res = _cur;
        _cur += size;
        assert(_cur < _end);
        return res;
    }
};

ThreadSafeIDMap<uint32_t, SysHeap*> map;

}

uint32_t _sys_heap_create_heap(uint64_t unk1, uint32_t size, uint64_t unk2, uint64_t unk3, MainMemory* mm) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    auto heap = new SysHeap(size, mm);
    return map.create(heap);
}

uint32_t _sys_heap_malloc(uint32_t heap_id, uint32_t size, big_uint32_t* ptr) {
    auto heap = map.get(heap_id);
    return heap->alloc(size);
}