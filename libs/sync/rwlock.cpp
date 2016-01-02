#include "rwlock.h"
#include "../../ps3emu/IDMap.h"
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <memory>
#include <assert.h>

namespace {
    IDMap<sys_rwlock_t, std::unique_ptr<boost::shared_mutex>> mutexes;
    boost::mutex map_mutex;
}

int32_t sys_rwlock_create(sys_rwlock_t* rw_lock_id, const sys_rwlock_attribute_t* attr) {
    auto mutex = std::make_unique<boost::shared_mutex>();
    boost::unique_lock<boost::mutex> lock(map_mutex);
    *rw_lock_id = mutexes.create(std::move(mutex));
    return CELL_OK;
}

int32_t sys_rwlock_destroy(sys_rwlock_t rw_lock_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    mutexes.destroy(rw_lock_id);
    return CELL_OK;
}

int32_t sys_rwlock_rlock(sys_rwlock_t rw_lock_id, usecond_t timeout) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto& mutex = mutexes.get(rw_lock_id);
    lock.unlock();
    if (timeout == 0)
        mutex->lock_shared();
    else
        mutex->try_lock_shared_for( boost::chrono::microseconds(timeout) );
    return CELL_OK;
}

int32_t sys_rwlock_tryrlock(sys_rwlock_t rw_lock_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto mutex =  mutexes.get(rw_lock_id).get();
    lock.unlock();
    bool locked = mutex->try_lock_shared();
    return locked ? CELL_OK : CELL_EBUSY;
}

int32_t sys_rwlock_runlock(sys_rwlock_t rw_lock_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    mutexes.get(rw_lock_id)->unlock_shared();
    return CELL_OK;
}

int32_t sys_rwlock_wlock(sys_rwlock_t rw_lock_id, usecond_t timeout) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto& mutex = mutexes.get(rw_lock_id);
    lock.unlock();
    if (timeout == 0)
        mutex->lock();
    else
        mutex->try_lock_for( boost::chrono::microseconds(timeout) );
    return CELL_OK;
}

int32_t sys_rwlock_trywlock(sys_rwlock_t rw_lock_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto mutex =  mutexes.get(rw_lock_id).get();
    lock.unlock();
    bool locked = mutex->try_lock();
    return locked ? CELL_OK : CELL_EBUSY;
}

int32_t sys_rwlock_wunlock(sys_rwlock_t rw_lock_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    mutexes.get(rw_lock_id)->unlock();
    return CELL_OK;
}