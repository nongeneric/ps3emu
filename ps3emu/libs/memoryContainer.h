#pragma once

#include "sys.h"
#include "ps3emu/libs/spu/sysSpu.h"

int32_t sys_memory_container_create(sys_memory_container_t* cid, uint32_t yield_size);
int32_t sys_memory_container_get_size(sys_memory_info_t* mem_info, sys_memory_container_t cid);
