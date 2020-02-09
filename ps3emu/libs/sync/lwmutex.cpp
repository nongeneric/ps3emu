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
    std::map<ps3_uintptr_t, sys_mutex_t> mutexes;
    boost::mutex map_mutex;
}

int sys_lwmutex_create(ps3_uintptr_t mutex_id, sys_lwmutex_attribute_t* attr) {
    INFO(sync) << ssnprintf("sys_lwmutex_create(%x, %s)", mutex_id, attr->name);
    sys_lwmutex_t type = { 0 };
    type.sleep_queue = 0x11223344;
    type.attribute = attr->attr_protocol | attr->attr_recursive;
    type.pad = mutex_id;
    g_state.mm->writeMemory(mutex_id, &type, sizeof(type));
    
    sys_mutex_attribute_t mattr;
    mattr.attr_protocol = attr->attr_protocol;
    mattr.attr_recursive = attr->attr_recursive;
    strncpy(mattr.name, attr->name, SYS_SYNC_NAME_SIZE);
    
    sys_mutex_t m;
    auto ret = sys_mutex_create(&m, &mattr);
    if (ret)
        return ret;
    
    boost::unique_lock<boost::mutex> lock(map_mutex);
    mutexes[mutex_id] = m;
    return CELL_OK;
}

int sys_lwmutex_destroy(ps3_uintptr_t lwmutex_id) {
    INFO(sync) << ssnprintf("sys_lwmutex_destroy(%x)", lwmutex_id);
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto it = mutexes.find(lwmutex_id);
    if (it == end(mutexes))
        return CELL_EINVAL;
    
    auto ret = sys_mutex_destroy(it->second);
    if (ret)
        return ret;

    mutexes.erase(it);
    return CELL_OK;
}

int sys_lwmutex_lock(ps3_uintptr_t lwmutex_id, usecond_t timeout) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto it = mutexes.find(lwmutex_id);
    if (it == end(mutexes)) {
        return CELL_ESRCH;
    }
    auto m = it->second;
    lock.unlock();
    return sys_mutex_lock(m, timeout);
}

int sys_lwmutex_unlock(ps3_uintptr_t lwmutex_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto it = mutexes.find(lwmutex_id);
    if (it == end(mutexes)) {
        return CELL_ESRCH;
    }
    auto m = it->second;
    lock.unlock();
    return sys_mutex_unlock(m);
}

int sys_lwmutex_trylock(ps3_uintptr_t lwmutex_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto it = mutexes.find(lwmutex_id);
    if (it == end(mutexes)) {
        return CELL_ESRCH;
    }
    auto m = it->second;
    lock.unlock();
    return sys_mutex_trylock(m);
}

sys_mutex_t find_lwmutex(ps3_uintptr_t id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto it = mutexes.find(id);
    if (it == end(mutexes)) {
        return 0;
    }
    return it->second;
}

std::map<uint32_t, std::shared_ptr<PthreadMutexInfo>> dbgDumpLwMutexes() {
    std::map<uint32_t, std::shared_ptr<PthreadMutexInfo>> map;
    for (auto& p : mutexes) {
        map[p.first] = find_mutex(p.second);
    }
    return map;
}
