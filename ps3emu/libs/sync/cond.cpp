#include "cond.h"

#include "../../utils.h"
#include "../../IDMap.h"
#include "../../log.h"
#include <boost/thread/condition_variable.hpp>

namespace {
    struct cv_info_t {
        boost::condition_variable_any cv;
        IMutex* m;
    };
    
    ThreadSafeIDMap<sys_cond_t, std::shared_ptr<cv_info_t>> cvs;
}

int32_t sys_cond_create(sys_cond_t* cond_id, 
                        sys_mutex_t lwmutex,
                        const sys_cond_attribute_t* attr)
{
    auto info = std::make_shared<cv_info_t>();
    info->m = find_mutex(lwmutex).get();
    *cond_id = cvs.create(std::move(info));
    LOG << ssnprintf("sys_cond_create(%x, %s)", *cond_id, attr->name);
    return CELL_OK;
}

int32_t sys_cond_destroy(sys_cond_t cond) {
    LOG << ssnprintf("sys_cond_destroy(%x)", cond);
    cvs.destroy(cond);
    return CELL_OK;
}

int32_t sys_cond_wait(sys_cond_t cond, usecond_t timeout) {
    assert(timeout == 0);
    auto info = cvs.get(cond);
    info->cv.wait(*info->m);
    return CELL_OK;
}

int32_t sys_cond_signal(sys_cond_t cond) {
    auto info = cvs.try_get(cond);
    if (!info) // noop
        return CELL_OK;
    info.value()->cv.notify_one();
    return CELL_OK;
}

int32_t sys_cond_signal_all(sys_cond_t cond) {
    auto info = cvs.get(cond);
    info->cv.notify_all();
    return CELL_OK;
}
