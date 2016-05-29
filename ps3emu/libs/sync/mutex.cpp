#include "mutex.h"

#include "../../utils.h"
#include "../../IDMap.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/log/trivial.hpp>
#include <memory>
#include <assert.h>

namespace {
    ThreadSafeIDMap<sys_mutex_t, std::shared_ptr<IMutex>> mutexes;
}

int sys_mutex_create(sys_mutex_t* mutex_id, sys_mutex_attribute_t* attr) {
    std::shared_ptr<IMutex> mutex;
    assert(attr->attr_recursive == SYS_SYNC_RECURSIVE ||
           attr->attr_recursive == SYS_SYNC_NOT_RECURSIVE);
    if (attr->attr_recursive == SYS_SYNC_NOT_RECURSIVE) {
        mutex.reset(new Mutex<boost::timed_mutex>());
    } else {
        mutex.reset(new Mutex<boost::recursive_timed_mutex>());
    }
    *mutex_id = mutexes.create(std::move(mutex));
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sys_mutex_create(%x, ...)", *mutex_id);
    return CELL_OK;
}

int sys_mutex_destroy(sys_mutex_t mutex_id) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sys_mutex_destroy(%x, ...)", mutex_id);
    mutexes.destroy(mutex_id);
    return CELL_OK;
}

int sys_mutex_lock(sys_mutex_t mutex_id, usecond_t timeout) {
    auto mutex = mutexes.try_get(mutex_id);
    // using a destroyed mutex is a noop
    if (!mutex)
        return 0;
    if (timeout == 0) {
        mutex.value()->lock();
        return CELL_OK;
    } else {
        return mutex.value()->lock(timeout) ? CELL_OK : CELL_ETIMEDOUT;
    }
}

int sys_mutex_trylock(sys_mutex_t mutex_id) {
    auto mutex =  mutexes.get(mutex_id);
    bool locked = mutex->try_lock(0);
    return locked ? CELL_OK : CELL_EBUSY;
}

int sys_mutex_unlock(sys_mutex_t mutex_id) {
    auto mutex = mutexes.try_get(mutex_id);
    if (!mutex) // noop
        return CELL_OK;
    mutex.value()->unlock();
    return CELL_OK;
}

std::shared_ptr<IMutex> find_mutex(sys_mutex_t id) {
    return mutexes.get(id);
}