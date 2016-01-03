#pragma once

#include "../../ps3emu/MainMemory.h"
#include "../sys_defs.h"

struct sys_lwcond_attribute_t {
    char name[SYS_SYNC_NAME_SIZE];
};

struct sys_lwcond_t {
    big_uint32_t lwmutex;
    _sys_lwcond_queue_t lwcond_queue;
};

static_assert(sizeof(sys_lwcond_t) == 8, "");

int32_t sys_lwcond_create(ps3_uintptr_t lwcond, 
                          ps3_uintptr_t lwmutex,
                          const sys_lwcond_attribute_t* attr,
                          MainMemory* mm);
int32_t sys_lwcond_destroy(ps3_uintptr_t lwcond);
int32_t sys_lwcond_wait(ps3_uintptr_t lwcond, usecond_t timeout);
int32_t sys_lwcond_signal(ps3_uintptr_t lwcond);
int32_t sys_lwcond_signal_all(ps3_uintptr_t lwcond);
