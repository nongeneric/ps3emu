#include "sys.h"

void sys_initialize_tls(uint64_t undef, uint32_t unk1, uint32_t unk2) {
    
}

int sys_lwmutex_create(sys_lwmutex_t* mutex_id, sys_lwmutex_attribute_t* attr) {
    return CELL_OK;
}

int sys_lwmutex_destroy(sys_lwmutex_t* lwmutex_id) {
    return CELL_OK;
}

int sys_lwmutex_lock(sys_lwmutex_t* lwmutex_id, usecond_t timeout) {
    return CELL_OK;
}

int sys_lwmutex_unlock(sys_lwmutex_t* lwmutex_id) {
    return CELL_OK;
}
