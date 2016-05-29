#pragma once

#include "sys_defs.h"

class MainMemory;

uint32_t _sys_heap_create_heap(uint64_t unk1, uint32_t size, uint64_t unk2, uint64_t unk3, MainMemory* mm);
uint32_t _sys_heap_delete_heap(uint32_t heap_id, uint64_t unk);
uint32_t _sys_heap_malloc(uint32_t heap_id, uint32_t size, big_uint32_t* ptr);
uint32_t _sys_heap_memalign(uint32_t heap_id, uint32_t alignment, uint32_t size);