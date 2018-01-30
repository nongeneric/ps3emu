#include "cond.h"

#include "ps3emu/utils.h"
#include "ps3emu/IDMap.h"
#include "ps3emu/log.h"
#include "ps3emu/ppu/PPUThread.h"
#include "ps3emu/profiler.h"
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <set>

namespace {
    struct cv_info_t {
        PthreadMutexInfo* m;
        std::string name;
        std::list<sys_ppu_thread_t> waiters;
        std::list<sys_ppu_thread_t> next;
        boost::mutex waitersMutex;
        boost::condition_variable cv;
    };
    
    ThreadSafeIDMap<sys_cond_t, std::shared_ptr<cv_info_t>, CondIdBase> cvs;
}

int32_t sys_cond_create(sys_cond_t* cond_id, 
                        sys_mutex_t mutex,
                        const sys_cond_attribute_t* attr)
{
    auto info = std::make_shared<cv_info_t>();
    __itt_sync_create(info.get(), "cond", attr->name, 0);
    info->m = find_mutex(mutex).get();
    info->name = attr->name;
    // TODO: error handling
    *cond_id = cvs.create(std::move(info));
    INFO(libs) << ssnprintf("sys_cond_create(%d, %x, %s)", *cond_id, mutex, attr->name);
    return CELL_OK;
}

int32_t sys_cond_destroy(sys_cond_t cond) {
    INFO(libs) << ssnprintf("sys_cond_destroy(%x)", cond);
    auto optinfo = cvs.try_get(cond);
    if (!optinfo)
        return CELL_ESRCH;
    auto& info = *optinfo;
    __itt_sync_destroy(info.get());
    cvs.destroy(cond);
    return CELL_OK;
}

class pthread_mutex_lock_on_exit {
    pthread_mutex_t* _mutex;

public:
    inline pthread_mutex_lock_on_exit(pthread_mutex_t* mutex) : _mutex(mutex) {
        pthread_mutex_unlock(_mutex);
    }
    inline ~pthread_mutex_lock_on_exit() {
        pthread_mutex_lock(_mutex);
    }
};

int32_t sys_cond_wait(sys_cond_t cond, usecond_t timeout) {
    auto optinfo = cvs.try_get(cond);
    if (!optinfo)
        return CELL_ESRCH;
    auto& info = *optinfo;
    INFO(libs, sync) << ssnprintf("sys_cond_wait(%x) c:%s m:%x(%s)",
                            cond,
                            info->name,
                            info->m->id,
                            info->m->name);

    __itt_sync_prepare(info.get());

    auto id = g_state.th->getId();

    pthread_mutex_lock_on_exit externalLock(&info->m->mutex);

    auto lock = boost::unique_lock(info->waitersMutex);
    info->waiters.push_back(id);

    auto check = [&] {
        auto it = std::find(begin(info->next), end(info->next), id);
        if (it == end(info->next))
            return false;
        info->next.erase(it);
        it = std::find(begin(info->waiters), end(info->waiters), id);
        assert(it != end(info->waiters));
        info->waiters.erase(it);
        return true;
    };

    if (timeout == 0) {
        for (;;) {
            info->cv.wait(lock);
            if (check()) {
                __itt_sync_acquired(info.get());
                return CELL_OK;
            }
        }
    } else {
        for (;;) {
            if (info->cv.wait_for(lock, boost::chrono::microseconds(timeout)) == boost::cv_status::timeout) {
                __itt_sync_cancel(info.get());
                return CELL_ETIMEDOUT;
            }
            if (check()) {
                __itt_sync_acquired(info.get());
                return CELL_OK;
            }
        }
    }
    return CELL_OK;
}

int32_t sys_cond_signal(sys_cond_t cond) {
    auto optinfo = cvs.try_get(cond);
    if (!optinfo)
        return CELL_ESRCH;
    auto& info = *optinfo;
    __itt_sync_releasing(info.get());
    INFO(libs, sync) << ssnprintf("sys_cond_signal(%x) c:%s m:%x(%s)",
                            cond,
                            info->name,
                            info->m->id,
                            info->m->name);
    boost::lock_guard<boost::mutex> lock(info->waitersMutex);
    if (info->waiters.empty())
        return CELL_OK;
    info->next.push_back(info->waiters.back());
    info->cv.notify_all();
    return CELL_OK;
}

int32_t sys_cond_signal_all(sys_cond_t cond) {
    auto optinfo = cvs.try_get(cond);
    if (!optinfo)
        return CELL_ESRCH;
    auto& info = *optinfo;
    __itt_sync_releasing(info.get());
    INFO(libs, sync) << ssnprintf("sys_cond_signal_all(%x) c:%s m:%x(%s)",
                            cond,
                            info->name,
                            info->m->id,
                            info->m->name);
    boost::lock_guard<boost::mutex> lock(info->waitersMutex);
    for (auto waiter : info->waiters)
        info->next.push_back(waiter);
    info->cv.notify_all();
    return CELL_OK;
}

int32_t sys_cond_signal_to(sys_cond_t cond, sys_ppu_thread_t ppu_thread_id) {
    auto optinfo = cvs.try_get(cond);
    if (!optinfo)
        return CELL_ESRCH;
    auto& info = *optinfo;
    __itt_sync_releasing(info.get());
    INFO(libs, sync) << ssnprintf("sys_cond_signal_to(%x, %x) c:%s m:%x(%s)",
                            cond,
                            ppu_thread_id,
                            info->name,
                            info->m->id,
                            info->m->name);
    boost::lock_guard<boost::mutex> lock(info->waitersMutex);
    auto it = std::find(begin(info->waiters), end(info->waiters), ppu_thread_id);
    if (it == end(info->waiters))
        return CELL_EPERM;
    info->next.push_back(ppu_thread_id);
    info->cv.notify_all();
    return CELL_OK;
}
