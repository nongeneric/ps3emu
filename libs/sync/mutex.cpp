#include "mutex.h"

#include "../../ps3emu/IDMap.h"
#include <boost/thread/mutex.hpp>
#include <memory>
#include <assert.h>

namespace {
    IDMap<sys_mutex_t, std::unique_ptr<boost::timed_mutex>> mutexes;
    boost::mutex map_mutex;
}

int sys_mutex_create(sys_mutex_t* mutex_id, sys_mutex_attribute_t* attr) {
    assert(attr->attr_recursive == SYS_SYNC_NOT_RECURSIVE);
    auto mutex = std::make_unique<boost::timed_mutex>();
    boost::unique_lock<boost::mutex> lock(map_mutex);
    *mutex_id = mutexes.create(std::move(mutex));
    return CELL_OK;
}

int sys_mutex_destroy(sys_mutex_t mutex_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    mutexes.destroy(mutex_id);
    return CELL_OK;
}

int sys_mutex_lock(sys_mutex_t mutex_id, usecond_t timeout) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto& mutex = mutexes.get(mutex_id);
    lock.unlock();    
    if (timeout == 0)
        mutex->lock();
    else
        mutex->try_lock_for( boost::chrono::microseconds(timeout) );
    return CELL_OK;
}

int sys_mutex_trylock(sys_mutex_t mutex_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto mutex =  mutexes.get(mutex_id).get();
    lock.unlock();
    bool locked = mutex->try_lock();
    return locked ? CELL_OK : CELL_EBUSY;
}

int sys_mutex_unlock(sys_mutex_t mutex_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    mutexes.get(mutex_id)->unlock();
    return CELL_OK;
}