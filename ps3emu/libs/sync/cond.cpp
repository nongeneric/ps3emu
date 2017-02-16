#include "cond.h"

#include "ps3emu/utils.h"
#include "ps3emu/IDMap.h"
#include "ps3emu/log.h"
#include <boost/thread/condition_variable.hpp>

namespace {
    struct cv_info_t {
        pthread_cond_t cv;
        PthreadMutexInfo* m;
        std::string name;
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
    LOG << ssnprintf("sys_cond_create(%d, %x, %s)", *cond_id, mutex, attr->name);
    return CELL_OK;
}

int32_t sys_cond_destroy(sys_cond_t cond) {
    LOG << ssnprintf("sys_cond_destroy(%x)", cond);
    // TODO: error handling
    auto info = cvs.get(cond);
    pthread_cond_destroy(&info->cv);
    cvs.destroy(cond);
    return CELL_OK;
}

int32_t sys_cond_wait(sys_cond_t cond, usecond_t timeout) {
    assert(timeout == 0);
    auto info = cvs.get(cond);
    INFO(libs) << ssnprintf("sys_cond_wait(%x) c:%s m:%s",
                            cond,
                            info->name,
                            info->m->name);
    // TODO: error handling
    pthread_cond_wait(&info->cv, &info->m->mutex);
    return CELL_OK;
}

int32_t sys_cond_signal(sys_cond_t cond) {
    auto info = cvs.try_get(cond);
    if (!info) // noop
        return CELL_OK;
    INFO(libs) << ssnprintf("sys_cond_signal(%x) c:%s m:%s",
                            cond,
                            (*info)->name,
                            (*info)->m->name);
    // TODO: error handling
    pthread_cond_signal(&(*info)->cv);
    return CELL_OK;
}

int32_t sys_cond_signal_all(sys_cond_t cond) {
    auto info = cvs.get(cond);
    INFO(libs) << ssnprintf("sys_cond_signal_all(%x) c:%s m:%s",
                            cond,
                            info->name,
                            info->m->name);
    // TODO: error handling
    pthread_cond_broadcast(&info->cv);
    return CELL_OK;
}
