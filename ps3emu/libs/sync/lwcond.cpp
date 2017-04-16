#include "lwcond.h"

#include "lwmutex.h"
#include <boost/thread/condition_variable.hpp>
#include "ps3emu/log.h"
#include "ps3emu/libs/sync/cond.h"

namespace {
    using cv_map_t = std::map<ps3_uintptr_t, sys_cond_t>;
    cv_map_t cvs;
    boost::mutex map_mutex;
}

cv_map_t::const_iterator find_cv_iter(ps3_uintptr_t mutex_id) {
    auto it = cvs.find(mutex_id);
    if (it == end(cvs))
        throw std::runtime_error("destroying uninitialized condition variable");
    return it;
}

int32_t sys_lwcond_create(ps3_uintptr_t lwcond,
                          ps3_uintptr_t lwmutex,
                          const sys_lwcond_attribute_t* attr,
                          MainMemory* mm) {
    INFO(libs) << ssnprintf("sys_lwcond_create(%x, %x, ...)", lwcond, lwmutex);
    sys_lwcond_t type = { 0 };
    type.lwmutex = lwmutex;
    type.lwcond_queue = 0xaabbccdd;
    mm->writeMemory(lwcond, &type, sizeof(type));
    
    sys_cond_attribute_t cattr;
    strncpy(cattr.name, attr->name, SYS_SYNC_NAME_SIZE);
    
    sys_cond_t c;
    auto ret = sys_cond_create(&c, find_lwmutex(lwmutex), &cattr);
    if (ret)
        return ret;
    
    boost::unique_lock<boost::mutex> lock(map_mutex);
    cvs[lwcond] = c;
    
    return CELL_OK;
}

int32_t sys_lwcond_destroy(ps3_uintptr_t lwcond) {
    INFO(libs) << ssnprintf("sys_lwcond_destroy(%x)", lwcond);
    
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto it = find_cv_iter(lwcond);
    
    auto ret = sys_cond_destroy(it->second);
    if (ret)
        return ret;
    
    cvs.erase(it);
    return CELL_OK;
}

int32_t sys_lwcond_wait(ps3_uintptr_t lwcond, usecond_t timeout) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto cond = find_cv_iter(lwcond)->second;
    lock.unlock();
    return sys_cond_wait(cond, timeout);
}

int32_t sys_lwcond_signal(ps3_uintptr_t lwcond) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto cond = find_cv_iter(lwcond)->second;
    lock.unlock();
    return sys_cond_signal(cond);
}

int32_t sys_lwcond_signal_all(ps3_uintptr_t lwcond) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto cond = find_cv_iter(lwcond)->second;
    lock.unlock();
    return sys_cond_signal_all(cond);
}
