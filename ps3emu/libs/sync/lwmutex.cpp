#include "lwmutex.h"

#include <map>
#include <memory>
#include "../../utils.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "../../log.h"
#include <assert.h>

namespace {
    using mutex_map_t = std::map<ps3_uintptr_t, std::shared_ptr<IMutex>>;
    mutex_map_t mutexes;
    boost::mutex map_mutex;
}

mutex_map_t::const_iterator find_mutex_iter(ps3_uintptr_t mutex_id) {
    auto it = mutexes.find(mutex_id);
    if (it == end(mutexes))
        throw std::runtime_error("destroying uninitialized mutex");
    return it;
}

int sys_lwmutex_create(ps3_uintptr_t mutex_id, sys_lwmutex_attribute_t* attr, MainMemory* mm) {
    LOG << ssnprintf("sys_lwmutex_create(%x, ...)", mutex_id);
    sys_lwmutex_t type = { 0 };
    type.sleep_queue = 0x11223344;
    type.attribute = attr->attr_protocol | attr->attr_recursive;
    mm->writeMemory(mutex_id, &type, sizeof(type));
    std::shared_ptr<IMutex> mutex;
    assert(attr->attr_recursive == SYS_SYNC_RECURSIVE ||
           attr->attr_recursive == SYS_SYNC_NOT_RECURSIVE ||
           attr->attr_recursive == 0);
    if (attr->attr_recursive == SYS_SYNC_RECURSIVE) {
        mutex.reset(new Mutex<boost::recursive_timed_mutex>());
    } else {
        mutex.reset(new Mutex<boost::timed_mutex>());
    }
    boost::unique_lock<boost::mutex> lock(map_mutex);
    mutexes.emplace(mutex_id, std::move(mutex));
    return CELL_OK;
}

int sys_lwmutex_destroy(ps3_uintptr_t lwmutex_id) {
    LOG << ssnprintf("sys_lwmutex_destroy(%x)", lwmutex_id);
    boost::unique_lock<boost::mutex> lock(map_mutex);
    mutexes.erase(find_mutex_iter(lwmutex_id));
    return CELL_OK;
}

int sys_lwmutex_lock(ps3_uintptr_t lwmutex_id, usecond_t timeout) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto mutex = find_mutex_iter(lwmutex_id)->second;
    lock.unlock();
    if (timeout == 0) {
        mutex->lock();
        return CELL_OK;
    } else {
        return mutex->try_lock(timeout) ? CELL_OK : CELL_ETIMEDOUT;
    }
}

int sys_lwmutex_unlock(ps3_uintptr_t lwmutex_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    find_mutex_iter(lwmutex_id)->second->unlock();
    return CELL_OK;
}

int sys_lwmutex_trylock(ps3_uintptr_t lwmutex_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto mutex = find_mutex_iter(lwmutex_id)->second;
    lock.unlock();
    bool locked = mutex->try_lock(0);
    return locked ? CELL_OK : CELL_EBUSY;
    return CELL_OK;
}

std::shared_ptr<IMutex> find_lwmutex(ps3_uintptr_t id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    return find_mutex_iter(id)->second;
}
