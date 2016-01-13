#include "cond.h"

#include "../../ps3emu/utils.h"
#include "../../ps3emu/IDMap.h"
#include <boost/thread/condition_variable.hpp>
#include <boost/log/trivial.hpp>

namespace {
    struct cv_info_t {
        boost::condition_variable_any cv;
        IMutex* m;
    };
    
    IDMap<sys_cond_t, std::shared_ptr<cv_info_t>> cvs;
    boost::mutex map_mutex;
}

int32_t sys_cond_create(sys_cond_t* cond_id, 
                        sys_mutex_t lwmutex,
                        const sys_cond_attribute_t* attr)
{
    auto info = std::make_shared<cv_info_t>();
    info->m = find_mutex(lwmutex).get();
    boost::unique_lock<boost::mutex> lock(map_mutex);
    *cond_id = cvs.create(std::move(info));
    lock.unlock();
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sys_cond_create(%x, %s)", *cond_id, attr->name);
    return CELL_OK;
}

int32_t sys_cond_destroy(sys_cond_t cond) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sys_cond_destroy(%x)", cond);
    boost::unique_lock<boost::mutex> lock(map_mutex);
    cvs.destroy(cond);
    return CELL_OK;
}

int32_t sys_cond_wait(sys_cond_t cond, usecond_t timeout) {
    assert(timeout == 0);
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto info = cvs.get(cond);
    lock.unlock();
    info->cv.wait(*info->m);
    return CELL_OK;
}

int32_t sys_cond_signal(sys_cond_t cond) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto info = cvs.get(cond);
    lock.unlock();
    info->cv.notify_one();
    return CELL_OK;
}

int32_t sys_cond_signal_all(sys_cond_t cond) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto info = cvs.get(cond);
    lock.unlock();
    info->cv.notify_all();
    return CELL_OK;
}