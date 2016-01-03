#pragma once

#include "../sys_defs.h"
#include "../../ps3emu/MainMemory.h"

struct sys_lwmutex_lock_info_t {
    volatile big_uint32_t owner;
    volatile big_uint32_t waiter;
};

union sys_lwmutex_variable_t {
    sys_lwmutex_lock_info_t info;
    volatile big_uint64_t all_info;
};

struct sys_lwmutex_t {
    sys_lwmutex_variable_t lock_var;
    big_uint32_t attribute;
    big_uint32_t recursive_count;
    _sys_sleep_queue_t sleep_queue;
    big_uint32_t pad;
};

static_assert(sizeof(sys_lwmutex_t) == 24, "");

struct sys_lwmutex_attribute_t {
    sys_protocol_t attr_protocol;
    sys_recursive_t attr_recursive;
    char name[SYS_SYNC_NAME_SIZE];
};

static_assert(sizeof(sys_lwmutex_attribute_t) == 16, "");

class IMutex {
public:
    virtual void lock(usecond_t timeout = 0) = 0;
    virtual void unlock() = 0;
    virtual bool try_lock() = 0;
};

int sys_lwmutex_create(ps3_uintptr_t mutex_id,
                       sys_lwmutex_attribute_t * attr,
                       MainMemory* mm);
int sys_lwmutex_destroy(ps3_uintptr_t lwmutex_id);
int sys_lwmutex_lock(ps3_uintptr_t lwmutex_id, usecond_t timeout);
int sys_lwmutex_trylock(ps3_uintptr_t lwmutex_id);
int sys_lwmutex_unlock(ps3_uintptr_t lwmutex_id);
IMutex* find_mutex(ps3_uintptr_t);
