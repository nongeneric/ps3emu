#include "lwcond.h"

#include "lwmutex.h"
#include <boost/thread/condition_variable.hpp>

namespace {
    struct cv_info_t {
        boost::condition_variable_any cv;
        IMutex* m;
    };
    
    using cv_map_t = std::map<ps3_uintptr_t, std::unique_ptr<cv_info_t>>;
    cv_map_t cvs;
    boost::mutex map_mutex;
}

cv_map_t::const_iterator find_cv_iter(ps3_uintptr_t mutex_id) {
    auto it = cvs.find(mutex_id);
    if (it == end(cvs))
        throw std::runtime_error("destroying uninitialized condition variable");
    return it;
}

int32_t sys_lwcond_create(ps3_uintptr_t lwcond, ps3_uintptr_t lwmutex, const sys_lwcond_attribute_t* attr) {
    auto info = std::make_unique<cv_info_t>();
    info->m = find_mutex(lwmutex);
    boost::unique_lock<boost::mutex> lock(map_mutex);
     cvs.emplace(lwcond, std::move(info));
    return CELL_OK;
}

int32_t sys_lwcond_destroy(ps3_uintptr_t lwcond) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    cvs.erase(find_cv_iter(lwcond));
    return CELL_OK;
}

int32_t sys_lwcond_wait(ps3_uintptr_t lwcond, usecond_t timeout) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto info = find_cv_iter(lwcond)->second.get();
    lock.unlock();
    info->cv.wait(*info->m);
    return CELL_OK;
}

int32_t sys_lwcond_signal(ps3_uintptr_t lwcond) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto info = find_cv_iter(lwcond)->second.get();
    lock.unlock();
    info->cv.notify_one();
    return CELL_OK;
}

int32_t sys_lwcond_signal_all(ps3_uintptr_t lwcond) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto info = find_cv_iter(lwcond)->second.get();
    lock.unlock();
    info->cv.notify_all();
    return CELL_OK;
}
