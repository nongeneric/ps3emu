#include "lwmutex.h"

#include <map>
#include <memory>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "ps3emu/log.h"
#include "ps3emu/utils.h"
#include "ps3emu/state.h"
#include "ps3emu/libs/sys.h"
#include <assert.h>

namespace {
    using mutex_map_t = std::map<ps3_uintptr_t, std::shared_ptr<IMutex>>;
    mutex_map_t mutexes;
    boost::mutex map_mutex;
}

int sys_lwmutex_create(ps3_uintptr_t mutex_id, sys_lwmutex_attribute_t* attr) {
    LOG << ssnprintf("sys_lwmutex_create(%x, %s)", mutex_id, attr->name);
    sys_lwmutex_t type = { 0 };
    type.sleep_queue = 0x11223344;
    type.attribute = attr->attr_protocol | attr->attr_recursive;
    type.pad = mutex_id;
    g_state.mm->writeMemory(mutex_id, &type, sizeof(type));
    std::shared_ptr<IMutex> mutex;
    assert(attr->attr_recursive == SYS_SYNC_RECURSIVE ||
           attr->attr_recursive == SYS_SYNC_NOT_RECURSIVE ||
           attr->attr_recursive == 0);
    if (attr->attr_recursive == SYS_SYNC_RECURSIVE) {
        mutex.reset(new Mutex<boost::recursive_timed_mutex>(attr->name));
    } else {
        mutex.reset(new Mutex<boost::timed_mutex>(attr->name));
    }
    boost::unique_lock<boost::mutex> lock(map_mutex);
    mutexes.emplace(mutex_id, std::move(mutex));
    return CELL_OK;
}

int sys_lwmutex_destroy(ps3_uintptr_t lwmutex_id) {
    LOG << ssnprintf("sys_lwmutex_destroy(%x)", lwmutex_id);
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto m = mutexes.find(lwmutex_id);
    assert(m != end(mutexes));
    m->second->destroy();
    mutexes.erase(m);
    return CELL_OK;
}

int sys_lwmutex_lock(ps3_uintptr_t lwmutex_id, usecond_t timeout) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto it = mutexes.find(lwmutex_id);
    if (it == end(mutexes)) {
        return CELL_ESRCH;
    }
    auto mutex = it->second;
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
    auto it = mutexes.find(lwmutex_id);
    if (it == end(mutexes)) {
        return CELL_ESRCH;
    }
    it->second->unlock();
    return CELL_OK;
}

int sys_lwmutex_trylock(ps3_uintptr_t lwmutex_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto it = mutexes.find(lwmutex_id);
    if (it == end(mutexes)) {
        return CELL_ESRCH;
    }
    lock.unlock();
    bool locked = it->second->try_lock(0);
    return locked ? CELL_OK : CELL_EBUSY;
    return CELL_OK;
}

std::shared_ptr<IMutex> find_lwmutex(ps3_uintptr_t id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto it = mutexes.find(id);
    if (it == end(mutexes)) {
        return nullptr;
    }
    return it->second;
}

std::map<uint32_t, std::shared_ptr<IMutex>> dbgDumpLwMutexes() {
    return mutexes;
}
