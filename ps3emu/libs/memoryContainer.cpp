#include "memoryContainer.h"

int32_t sys_memory_container_create(sys_memory_container_t* cid, uint32_t yield_size) {
    *cid = 0x11223344;
    return CELL_OK;
}

int32_t sys_memory_container_get_size(sys_memory_info_t* mem_info, sys_memory_container_t cid) {
    mem_info->total_user_memory = 250u << 20u;
    mem_info->available_user_memory = 200u << 20u;
    return CELL_OK;
}
