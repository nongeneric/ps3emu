#pragma once

#include "mutex.h"
#include "../sys.h"
#include "../sys_defs.h"

struct sys_cond_attribute_t {
    sys_process_shared_t attr_pshared;
    int flags;
    sys_ipc_key_t key;
    char name[SYS_SYNC_NAME_SIZE];
};

using sys_cond_t = big_uint32_t;

int32_t sys_cond_create(sys_cond_t* cond_id, 
                        sys_mutex_t mutex,
                        const sys_cond_attribute_t* attr);
int32_t sys_cond_destroy(sys_cond_t cond);
int32_t sys_cond_wait(sys_cond_t cond, usecond_t timeout);
int32_t sys_cond_signal(sys_cond_t cond);
int32_t sys_cond_signal_all(sys_cond_t cond);
int32_t sys_cond_signal_to(sys_cond_t cond, sys_ppu_thread_t ppu_thread_id);
