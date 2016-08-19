#pragma once

#include "../sys_defs.h"
#include "mutex.h"
#include "ps3emu/MainMemory.h"
#include <memory>
#include <pthread.h>

int sys_sleep_queue_create(big_uint32_t* queue,
                           uint32_t attr_protocol,
                           uint32_t channel,
                           uint32_t unk,
                           big_uint64_t name);
int sys_sleep_queue_destroy(ps3_uintptr_t lwmutex_id);
int sys_sleep_queue_lock(ps3_uintptr_t lwmutex_id, usecond_t timeout);
int sys_sleep_queue_trylock(ps3_uintptr_t lwmutex_id);
int sys_sleep_queue_unlock(ps3_uintptr_t lwmutex_id);
std::shared_ptr<pthread_mutex_t> find_lwmutex(ps3_uintptr_t id);
