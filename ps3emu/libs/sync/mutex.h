#pragma once

#include "../sys_defs.h"
#include "ps3emu/state.h"
#include <boost/chrono.hpp>
#include <memory>
#include <map>
#include <pthread.h>

typedef big_uint32_t sys_mutex_t;

struct PthreadMutexInfo {
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
    std::string name;
    
    std::string typestr();
    int type();
};

struct sys_mutex_attribute_t {
    sys_protocol_t attr_protocol;
    sys_recursive_t attr_recursive;
    sys_process_shared_t attr_pshared;
    sys_adaptive_t attr_adaptive;
    sys_ipc_key_t key;
    big_uint32_t flags;
    uint32_t pad;
    char name[SYS_SYNC_NAME_SIZE];
};
static_assert(sizeof(sys_mutex_attribute_t) == 40, "");

int sys_mutex_create(sys_mutex_t* mutex_id, sys_mutex_attribute_t* attr);
int sys_mutex_destroy(sys_mutex_t mutex_id);
int sys_mutex_lock(sys_mutex_t mutex_id, usecond_t timeout);
int sys_mutex_trylock(sys_mutex_t mutex_id);
int sys_mutex_unlock(sys_mutex_t mutex_id);
std::shared_ptr<PthreadMutexInfo> find_mutex(sys_mutex_t id);
std::map<uint32_t, std::shared_ptr<PthreadMutexInfo>> dbgDumpMutexes();
