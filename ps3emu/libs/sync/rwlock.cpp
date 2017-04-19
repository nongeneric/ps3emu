#include "rwlock.h"
#include "../../IDMap.h"
#include "ps3emu/log.h"
#include "ps3emu/utils.h"
#include <boost/thread/shared_mutex.hpp>
#include <memory>
#include <assert.h>

namespace {
    ThreadSafeIDMap<sys_rwlock_t, std::shared_ptr<boost::shared_timed_mutex>> mutexes;
}

int32_t sys_rwlock_create(sys_rwlock_t* rw_lock_id, const sys_rwlock_attribute_t* attr) {
    auto mutex = std::make_shared<boost::shared_timed_mutex>();
    *rw_lock_id = mutexes.create(std::move(mutex));
    return CELL_OK;
}

int32_t sys_rwlock_destroy(sys_rwlock_t rw_lock_id) {
    mutexes.destroy(rw_lock_id);
    return CELL_OK;
}

int32_t sys_rwlock_rlock(sys_rwlock_t rw_lock_id, usecond_t timeout) {
    auto mutex = mutexes.get(rw_lock_id);
    if (timeout == 0) {
        mutex->lock_shared();
    } else {
        if (!mutex->try_lock_shared_for( boost::chrono::microseconds(timeout) )) {
            return CELL_ETIMEDOUT;
        }
    }
    return CELL_OK;
}

int32_t sys_rwlock_tryrlock(sys_rwlock_t rw_lock_id) {
    auto mutex =  mutexes.get(rw_lock_id).get();
    bool locked = mutex->try_lock_shared();
    return locked ? CELL_OK : CELL_EBUSY;
}

int32_t sys_rwlock_runlock(sys_rwlock_t rw_lock_id) {
    mutexes.get(rw_lock_id)->unlock_shared();
    return CELL_OK;
}

int32_t sys_rwlock_wlock(sys_rwlock_t rw_lock_id, usecond_t timeout) {
    auto mutex = mutexes.get(rw_lock_id);
    if (timeout == 0) {
        mutex->lock();
    } else {
        if (!mutex->try_lock_for( boost::chrono::microseconds(timeout) )) {
            return CELL_ETIMEDOUT;
        }
    }
    return CELL_OK;
}

int32_t sys_rwlock_trywlock(sys_rwlock_t rw_lock_id) {
    auto mutex =  mutexes.get(rw_lock_id);
    bool locked = mutex->try_lock();
    return locked ? CELL_OK : CELL_EBUSY;
}

int32_t sys_rwlock_wunlock(sys_rwlock_t rw_lock_id) {
    mutexes.get(rw_lock_id)->unlock();
    return CELL_OK;
}
