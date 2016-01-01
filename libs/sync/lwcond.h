#pragma once

#include "../sys_defs.h"

struct sys_lwcond_attribute_t {
    char name[SYS_SYNC_NAME_SIZE];
};

int32_t sys_lwcond_create(ps3_uintptr_t lwcond, 
                          ps3_uintptr_t lwmutex,
                          const sys_lwcond_attribute_t* attr);
int32_t sys_lwcond_destroy(ps3_uintptr_t lwcond);
int32_t sys_lwcond_wait(ps3_uintptr_t lwcond, usecond_t timeout);
int32_t sys_lwcond_signal(ps3_uintptr_t lwcond);
int32_t sys_lwcond_signal_all(ps3_uintptr_t lwcond);
