#pragma once

#include "../../MainMemory.h"
#include "../sys_defs.h"

struct sys_lwcond_t {
    big_uint32_t lwmutex;
    _sys_lwcond_queue_t lwcond_queue;
};

int32_t sys_lwcond_create(big_uint32_t* lwcond_queue_id,
                          big_uint32_t sleep_queue_id,
                          big_uint32_t mutex_var,
                          big_uint64_t name,
                          uint32_t flags);
int32_t sys_lwcond_destroy(ps3_uintptr_t lwcond);
int32_t sys_lwcond_queue_wait(ps3_uintptr_t lwcond, uint32_t sleep_queue_id, useconds_t timeout);
int32_t sys_lwcond_signal_to(ps3_uintptr_t lwcond,
                             uint32_t sleep_queue_id,
                             uint32_t thread_id,
                             uint32_t unk2);
int32_t sys_lwcond_signal_all(ps3_uintptr_t lwcond,
                              uint32_t sleep_queue_id,
                              uint32_t unk2);
