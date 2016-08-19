#include "lwcond.h"

#include "lwmutex.h"
#include "ps3emu/IDMap.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/ppu/PPUThread.h"
#include <set>

namespace {
    struct CondInfo {
        pthread_cond_t cond;
        std::set<uint32_t> waiters;
        std::set<uint32_t> wakeup_next;
        pthread_mutex_t mutex;
    };
    
    ThreadSafeIDMap<uint32_t, std::shared_ptr<CondInfo>, LwCondIdBase> cvars;
}

bool islocked(pthread_mutex_t *mutex) {
    if (pthread_mutex_trylock(mutex) == 0) {
        pthread_mutex_unlock(mutex);
        return true;
    }
    return false;
}

int32_t sys_lwcond_create(big_uint32_t* lwcond_queue_id,
                          big_uint32_t sleep_queue_id,
                          big_uint32_t channel,
                          big_uint64_t name,
                          uint32_t flags) {
    assert(flags == 0);
    auto info = std::make_shared<CondInfo>();
    *lwcond_queue_id = cvars.create(info);
    
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_cond_init(&info->cond, &attr);
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutex_init(&info->mutex, &mattr);
    
    LOG << ssnprintf("sys_lwcond_create(%x, %x, %x, %s)",
                     *lwcond_queue_id,
                     sleep_queue_id,
                     channel,
                     (char*)&name);
    return CELL_OK;
}

int32_t sys_lwcond_destroy(ps3_uintptr_t lwcond) {
    LOG << ssnprintf("sys_lwcond_destroy(%x)", lwcond);
    auto info = cvars.get(lwcond);
    auto res = pthread_cond_destroy(&info->cond);
    assert(res == 0);
    cvars.destroy(lwcond);
    return CELL_OK;
}

bool shouldWakeup(CondInfo* info, uint32_t me) {
    for (auto next : info->wakeup_next) {
        if (next == me)
            return true;
    }
    return false;
}

int32_t sys_lwcond_queue_wait(ps3_uintptr_t lwcond,
                              uint32_t sleep_queue_id,
                              useconds_t timeout) {
    LOG << ssnprintf("sys_lwcond_queue_wait(%x, %x)", lwcond, sleep_queue_id);
    //assert(timeout == 0);
    auto mutex = find_lwmutex(sleep_queue_id);
    //auto locked = pthread_mutex_trylock(mutex.get());
    //assert(islocked(mutex.get()));
    
    auto info = cvars.get(lwcond);
    //pthread_mutex_lock(&info->mutex);
    
    auto id = g_state.th->getId();
    info->waiters.insert(id);
    while (!shouldWakeup(info.get(), id)) {
        auto res = pthread_cond_wait(&info->cond, mutex.get());
        assert(res == 0);
    }
    info->waiters.erase(id);
    info->wakeup_next.erase(id);
    
    //pthread_mutex_unlock(&info->mutex);
    
    return CELL_OK;
}

int32_t sys_lwcond_signal_to(ps3_uintptr_t lwcond,
                             uint32_t sleep_queue_id,
                             uint32_t thread_id,
                             uint32_t unk2) {
    LOG << ssnprintf("sys_lwcond_signal_to(%x, %x, %x, %x)",
                     lwcond,
                     sleep_queue_id,
                     thread_id,
                     unk2);
    //assert(islocked(find_lwmutex(sleep_queue_id).get()));
    //auto mutex = find_lwmutex(sleep_queue_id);
    int res = 0;
    //auto res = pthread_mutex_lock(mutex.get());
    //assert(res == 0);
    assert(thread_id == -1u);
    assert(unk2 == 1);
    assert(sleep_queue_id != 0);
    auto info = cvars.get(lwcond);
    if (thread_id == -1u && !info->waiters.empty()) {
        info->wakeup_next.insert(*begin(info->waiters));
    }
    res = pthread_cond_broadcast(&info->cond);
    assert(res == 0);
    //res = pthread_mutex_unlock(mutex.get());
    //assert(res == 0);
    return CELL_OK;
}

int32_t sys_lwcond_signal_all(ps3_uintptr_t lwcond,
                              uint32_t sleep_queue_id,
                              uint32_t unk2) {
    assert(sleep_queue_id != 0);
    assert(unk2 == 1);
    //auto mutex = find_lwmutex(sleep_queue_id);
    LOG << ssnprintf(
        "sys_lwcond_signal_all(%x, %x, %x)", lwcond, sleep_queue_id, unk2);
    auto info = cvars.get(lwcond);
    for (auto th : info->waiters) {
        info->wakeup_next.insert(th);
    }
    auto woken_up = info->wakeup_next.size();
    auto res = pthread_cond_broadcast(&info->cond);
    assert(res == 0);
    return woken_up;
}
