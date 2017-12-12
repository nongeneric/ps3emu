#include "mutex.h"

#include "ps3emu/utils.h"
#include "ps3emu/IDMap.h"
#include "ps3emu/log.h"
#include "ps3emu/profiler.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <memory>
#include <assert.h>

namespace {
    ThreadSafeIDMap<sys_mutex_t, std::shared_ptr<PthreadMutexInfo>, MutexIdBase> mutexes;
}

std::string PthreadMutexInfo::typestr() {
    return type() == PTHREAD_MUTEX_RECURSIVE ? "recursive" : "regular";
}

int PthreadMutexInfo::type() {
    int type;
    auto ret = pthread_mutexattr_gettype(&attr, &type);
    assert(ret == 0); (void)ret;
    return type;
}

int sys_mutex_create(sys_mutex_t* mutex_id, sys_mutex_attribute_t* attr) {
    if (!mutex_id || !attr)
        return CELL_EFAULT;

    auto info = std::make_shared<PthreadMutexInfo>();
    info->name = attr->name;
    pthread_mutexattr_t ptattr;
    auto ret = pthread_mutexattr_init(&ptattr);
    assert(ret == 0); (void)ret;

    auto type = attr->attr_recursive == SYS_SYNC_RECURSIVE
                    ? PTHREAD_MUTEX_RECURSIVE
                    : PTHREAD_MUTEX_ERRORCHECK;
                    
    ret = pthread_mutexattr_settype(&ptattr, type);
    assert(ret == 0);
    
    ret = pthread_mutex_init(&info->mutex, &ptattr);
    assert(ret == 0);
    
    info->id = mutexes.create(info);
    *mutex_id = info->id;
    INFO(libs) << ssnprintf("sys_mutex_create(%x, %s, %s)", *mutex_id, attr->name, info->type());

    __itt_sync_create(info.get(), "mutex", attr->name, 0);

    return CELL_OK;
}

int sys_mutex_destroy(sys_mutex_t mutex_id) {
    INFO(libs) << ssnprintf("sys_mutex_destroy(%x, ...)", mutex_id);
    auto info = mutexes.try_get(mutex_id);
    if (!info) {
        return CELL_ESRCH;
    }
    
    auto ret = pthread_mutex_destroy(&(*info)->mutex);
    if (ret == EBUSY)
        return CELL_EBUSY;
    
    ret = pthread_mutexattr_destroy(&(*info)->attr);
    assert(ret == 0);
    
    __itt_sync_destroy(info->get());
    mutexes.destroy(mutex_id);
    
    return CELL_OK;
}

int sys_mutex_lock(sys_mutex_t mutex_id, usecond_t timeout) {
    auto info = mutexes.try_get(mutex_id);
    INFO(libs, sync) << ssnprintf("locking %x %d %s",
                            mutex_id,
                            timeout,
                            info ? (*info)->name : "NULL");
    if (!info)
        return CELL_ESRCH;
    
    auto mutex = &(*info)->mutex;

    __itt_sync_prepare(info->get());
    
    if (timeout == 0) {
        auto ret = pthread_mutex_lock(mutex);
        __itt_sync_acquired(info->get());
        assert(ret == EDEADLK || ret == 0);
        return ret == EDEADLK ? CELL_EDEADLK : CELL_OK;
    } else {
        timespec abs_time;
        clock_gettime(CLOCK_REALTIME, &abs_time);
        auto ns = abs_time.tv_sec * 1000000000 + abs_time.tv_nsec + timeout * 1000;
        abs_time.tv_sec = ns / 1000000000;
        abs_time.tv_nsec = ns % 1000000000;
        auto ret = pthread_mutex_timedlock(mutex, &abs_time);
        assert(ret != EINVAL);
        assert(ret == 0 || ret == ETIMEDOUT);
        if (ret == ETIMEDOUT) {
            __itt_sync_cancel(info->get());
        } else {
            __itt_sync_acquired(info->get());
        }
        return ret == ETIMEDOUT ? CELL_ETIMEDOUT : CELL_OK;
    }
}

int sys_mutex_trylock(sys_mutex_t mutex_id) {
    auto info = mutexes.try_get(mutex_id);
    INFO(libs, sync) << ssnprintf(
        "trying to lock %x %s", mutex_id, info ? (*info)->name : "NULL");

    if (!info)
        return CELL_ESRCH;

    __itt_sync_prepare(info->get());

    auto ret = pthread_mutex_trylock(&(*info)->mutex);
    assert(ret == EDEADLK || ret == EBUSY || ret == 0);
    if (ret == EDEADLK)
        return CELL_EDEADLK;
    if (ret == EBUSY) {
        __itt_sync_cancel(info->get());
        if ((*info)->type() == PTHREAD_MUTEX_RECURSIVE) {
            return CELL_EBUSY;
        }
        return CELL_EDEADLK;
    }
    __itt_sync_acquired(info->get());
    return CELL_OK;
}

int sys_mutex_unlock(sys_mutex_t mutex_id) {
    auto info = mutexes.try_get(mutex_id);
    INFO(libs, sync) << ssnprintf("unlocking %x %s",
                            mutex_id,
                            info ? (*info)->name : "NULL");
    if (!info)
        return CELL_ESRCH;
    
    __itt_sync_releasing(info->get());
    auto ret = pthread_mutex_unlock(&(*info)->mutex);
    assert(ret == EPERM || ret == 0);
    if (ret == EPERM)
        return CELL_EPERM;
    return CELL_OK;
}

std::shared_ptr<PthreadMutexInfo> find_mutex(sys_mutex_t id) {
    return mutexes.get(id);
}

std::map<uint32_t, std::shared_ptr<PthreadMutexInfo>> dbgDumpMutexes() {
    std::map<uint32_t, std::shared_ptr<PthreadMutexInfo>> map;
    for (auto& pair : mutexes.map()) {
        map[pair.first] = pair.second;
    }
    return map;
}
