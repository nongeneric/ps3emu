#pragma once

#include "../sys_defs.h"

typedef big_uint32_t sys_rwlock_t;

struct sys_rwlock_attribute_t {
    sys_protocol_t attr_protocol;
    sys_process_shared_t attr_pshared;
    sys_ipc_key_t key;
    int flags;
    uint32_t pad;
    char name[SYS_SYNC_NAME_SIZE];
};

int32_t sys_rwlock_create(sys_rwlock_t* rw_lock_id, const sys_rwlock_attribute_t* attr);
int32_t sys_rwlock_destroy(sys_rwlock_t rw_lock_id);
int32_t sys_rwlock_rlock(sys_rwlock_t rw_lock_id, usecond_t timeout);
int32_t sys_rwlock_tryrlock(sys_rwlock_t rw_lock_id);
int32_t sys_rwlock_runlock(sys_rwlock_t rw_lock_id);
int32_t sys_rwlock_wlock(sys_rwlock_t rw_lock_id, usecond_t timeout);
int32_t sys_rwlock_trywlock(sys_rwlock_t rw_lock_id);
int32_t sys_rwlock_wunlock(sys_rwlock_t rw_lock_id);
