#include "lwmutex.h"

#include <map>
#include <memory>
#include "../../ps3emu/utils.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/log/trivial.hpp>
#include <assert.h>

namespace {
    template <typename M>
    class Mutex : public IMutex {
        M _m;
    public:
        void lock(usecond_t timeout) override {
            if (timeout == 0)
                _m.lock();
            else
                _m.try_lock_for( boost::chrono::microseconds(timeout) );
        }
        
        void unlock() override {
            _m.unlock();
        }
        
        bool try_lock() override {
            return _m.try_lock();
        }
    };
    
    using mutex_map_t = std::map<ps3_uintptr_t, std::unique_ptr<IMutex>>;
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sys_lwmutex_create(%x, ...)", mutex_id);
    sys_lwmutex_t type = { 0 };
    type.sleep_queue = 0x11223344;
    type.attribute = attr->attr_protocol | attr->attr_recursive;
    mm->writeMemory(mutex_id, &type, sizeof(type));
    std::unique_ptr<IMutex> mutex;
    assert(attr->attr_recursive == SYS_SYNC_RECURSIVE ||
           attr->attr_recursive == SYS_SYNC_NOT_RECURSIVE);
    if (attr->attr_recursive == SYS_SYNC_NOT_RECURSIVE) {
        mutex.reset(new Mutex<boost::timed_mutex>());
    } else {
        mutex.reset(new Mutex<boost::recursive_timed_mutex>());
    }
    boost::unique_lock<boost::mutex> lock(map_mutex);
    mutexes.emplace(mutex_id, std::move(mutex));
    return CELL_OK;
}

int sys_lwmutex_destroy(ps3_uintptr_t lwmutex_id) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sys_lwmutex_destroy(%x)", lwmutex_id);
    boost::unique_lock<boost::mutex> lock(map_mutex);
    mutexes.erase(find_mutex_iter(lwmutex_id));
    return CELL_OK;
}

int sys_lwmutex_lock(ps3_uintptr_t lwmutex_id, usecond_t timeout) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto mutex = find_mutex_iter(lwmutex_id)->second.get();
    lock.unlock();
    mutex->lock(timeout);
    return CELL_OK;
}

int sys_lwmutex_unlock(ps3_uintptr_t lwmutex_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    find_mutex_iter(lwmutex_id)->second->unlock();
    return CELL_OK;
}

int sys_lwmutex_trylock(ps3_uintptr_t lwmutex_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto mutex = find_mutex_iter(lwmutex_id)->second.get();
    lock.unlock();
    bool locked = mutex->try_lock();
    return locked ? CELL_OK : CELL_EBUSY;
    return CELL_OK;
}

IMutex* find_mutex(ps3_uintptr_t id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    return find_mutex_iter(id)->second.get();
}
