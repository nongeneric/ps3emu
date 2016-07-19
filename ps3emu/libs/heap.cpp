#include "heap.h"
#include "../MainMemory.h"
#include "../IDMap.h"
#include "../utils.h"
#include "../log.h"

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
    
    ps3_uintptr_t alloc(uint32_t size, uint32_t alignment) {
        auto res = ::align(_cur, alignment);
        _cur += size;
        assert(_cur < _end);
        return res;
    }
};

ThreadSafeIDMap<uint32_t, SysHeap*> map;

}

uint32_t _sys_heap_create_heap(uint64_t unk1, uint32_t size, uint64_t unk2, uint64_t unk3, MainMemory* mm) {
    auto heap = new SysHeap(size, mm);
    return map.create(heap);
}

uint32_t _sys_heap_delete_heap(uint32_t heap_id, uint64_t unk) {
    map.destroy(heap_id);
    return CELL_OK;
}

uint32_t _sys_heap_malloc(uint32_t heap_id, uint32_t size, big_uint32_t* ptr) {
    auto heap = map.get(heap_id);
    return heap->alloc(size, 1);
}

uint32_t _sys_heap_memalign(uint32_t heap_id, uint32_t alignment, uint32_t size) {
    auto heap = map.get(heap_id);
    return heap->alloc(size, alignment);
}