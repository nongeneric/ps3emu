#include "lwmutex.h"

#include <map>
#include <memory>
#include "ps3emu/IDMap.h"
#include "ps3emu/log.h"
#include "ps3emu/utils.h"
#include <assert.h>

namespace {
    ThreadSafeIDMap<uint32_t, std::shared_ptr<pthread_mutex_t>, LwMutexIdBase> mutexes;
}

int sys_sleep_queue_create(big_uint32_t* queue,
                           uint32_t attr_protocol,
                           uint32_t channel,
                           uint32_t unk,
                           big_uint64_t name) {
    assert(unk == 0x80000001);
    auto mutex = std::make_shared<pthread_mutex_t>();
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    auto res = pthread_mutex_init(mutex.get(), &attr);
    assert(res == 0);
    *queue = mutexes.create(mutex);
    LOG << ssnprintf("sys_lwmutex_create(%x, %s)", *queue, (char*)&name);
    return CELL_OK;
}

int sys_sleep_queue_destroy(ps3_uintptr_t lwmutex_id) {
    LOG << ssnprintf("sys_lwmutex_destroy(%x)", lwmutex_id);
    auto mutex = mutexes.get(lwmutex_id);
    //auto res = pthread_mutex_destroy(mutex.get());
    //assert(res == 0);
//     if (res == EBUSY) { // assume by me
//         res = pthread_mutex_unlock(mutex.get());
//         assert(res == 0);
//         res = pthread_mutex_destroy(mutex.get());
//         assert(res == 0);
//     }
    mutexes.destroy(lwmutex_id);
    return CELL_OK;
}

int sys_sleep_queue_lock(ps3_uintptr_t lwmutex_id, usecond_t timeout) {
    auto mutex = mutexes.get(lwmutex_id);
    if (timeout == 0) {
        auto res = pthread_mutex_lock(mutex.get());
        assert(res == 0);
        return CELL_OK;
    } else {
        struct timespec time;
        time.tv_sec = timeout / 1000000000ull;
        time.tv_nsec = timeout % 1000000000ull;
        auto res = pthread_mutex_timedlock(mutex.get(), &time);
        if (res ==  ETIMEDOUT)
            return CELL_ETIMEDOUT;
        assert(res == 0);
        return CELL_OK;
    }
}

int sys_sleep_queue_unlock(ps3_uintptr_t lwmutex_id) {
    auto res = pthread_mutex_unlock(mutexes.get(lwmutex_id).get());
    assert(res == 0);
    return CELL_OK;
}

int sys_sleep_queue_trylock(ps3_uintptr_t lwmutex_id) {
    auto mutex = mutexes.get(lwmutex_id);
    return pthread_mutex_trylock(mutex.get()) ? CELL_EBUSY : CELL_OK;
}

std::shared_ptr<pthread_mutex_t> find_lwmutex(ps3_uintptr_t id) {
    return mutexes.get(id);
}
