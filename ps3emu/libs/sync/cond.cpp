#include "cond.h"

#include "ps3emu/utils.h"
#include "ps3emu/IDMap.h"
#include "ps3emu/log.h"
#include "ps3emu/ppu/PPUThread.h"
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <set>

namespace {
    struct cv_info_t {
        pthread_cond_t cv;
        PthreadMutexInfo* m;
        std::string name;
        std::list<sys_ppu_thread_t> waiters;
        std::list<sys_ppu_thread_t> next;
        boost::mutex waitersMutex;
    };
    
    ThreadSafeIDMap<sys_cond_t, std::shared_ptr<cv_info_t>, CondIdBase> cvs;
}

int32_t sys_cond_create(sys_cond_t* cond_id, 
                        sys_mutex_t mutex,
                        const sys_cond_attribute_t* attr)
{
    auto info = std::make_shared<cv_info_t>();
    info->m = find_mutex(mutex).get();
    info->name = attr->name;
    // TODO: error handling
    pthread_cond_init(&info->cv, NULL);
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
    pthread_cond_destroy(&info->cv);
    cvs.destroy(cond);
    return CELL_OK;
}

int pthread_cond_timedwait2(pthread_cond_t* cond,
                            pthread_mutex_t* mutex,
                            usecond_t timeout) {
    timespec abs_time;
    clock_gettime(CLOCK_REALTIME, &abs_time);
    auto ns = abs_time.tv_sec * 1000000 + abs_time.tv_nsec + timeout * 1000;
    abs_time.tv_sec = ns / 1000000;
    abs_time.tv_nsec = ns % 1000000;
    return pthread_cond_timedwait(cond, mutex, &abs_time);
}

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
    auto id = g_state.th->getId();
    {
        boost::lock_guard<boost::mutex> lock(info->waitersMutex);
        info->waiters.push_back(id);
    }
    auto check = [&] {
        boost::lock_guard<boost::mutex> lock(info->waitersMutex);
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
            // there is a race condition:
            //   this thread:  add to waiters
            //   other thread: change condition and notify (add to next)
            //   this thread:  wait
            // this sequence leads to this thread ignoring a notification
            // and waiting indefinitely
            // to mitigate this race, introduce a timed wait and verify that
            // no notification has been sent in between adding to waiters and wait
            auto res = pthread_cond_timedwait2(&info->cv, &info->m->mutex, 2);
            if (res == EPERM)
                return CELL_EPERM;
            if (res == ETIMEDOUT)
                return CELL_OK;
            assert(res == 0 || res == ETIMEDOUT);
            if (check())
                return CELL_OK;
        }
    } else {
        timespec abs_time;
        clock_gettime(CLOCK_REALTIME, &abs_time);
        auto ns = abs_time.tv_sec * 1000000000 + abs_time.tv_nsec + timeout * 1000;
        abs_time.tv_sec = ns / 1000000000;
        abs_time.tv_nsec = ns % 1000000000;
        for (;;) {
            auto res = pthread_cond_timedwait(&info->cv, &info->m->mutex, &abs_time);
            if (res == ETIMEDOUT)
                return CELL_ETIMEDOUT;
            if (res == EPERM)
                return CELL_EPERM;
            assert(res == 0);
            if (check())
                return CELL_OK;
        }
    }
    return CELL_OK;
}

int32_t sys_cond_signal(sys_cond_t cond) {
    auto optinfo = cvs.try_get(cond);
    if (!optinfo)
        return CELL_ESRCH;
    auto& info = *optinfo;
    INFO(libs, sync) << ssnprintf("sys_cond_signal(%x) c:%s m:%x(%s)",
                            cond,
                            info->name,
                            info->m->id,
                            info->m->name);
    boost::lock_guard<boost::mutex> lock(info->waitersMutex);
    if (info->waiters.empty())
        return CELL_OK;
    info->next.push_back(info->waiters.back());
    pthread_cond_broadcast(&info->cv);
    return CELL_OK;
}

int32_t sys_cond_signal_all(sys_cond_t cond) {
    auto optinfo = cvs.try_get(cond);
    if (!optinfo)
        return CELL_ESRCH;
    auto& info = *optinfo;
    INFO(libs, sync) << ssnprintf("sys_cond_signal_all(%x) c:%s m:%x(%s)",
                            cond,
                            info->name,
                            info->m->id,
                            info->m->name);
    boost::lock_guard<boost::mutex> lock(info->waitersMutex);
    for (auto waiter : info->waiters)
        info->next.push_back(waiter);
    pthread_cond_broadcast(&info->cv);
    return CELL_OK;
}

int32_t sys_cond_signal_to(sys_cond_t cond, sys_ppu_thread_t ppu_thread_id) {
    auto optinfo = cvs.try_get(cond);
    if (!optinfo)
        return CELL_ESRCH;
    auto& info = *optinfo;
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
    pthread_cond_broadcast(&info->cv);
    return CELL_OK;
}
