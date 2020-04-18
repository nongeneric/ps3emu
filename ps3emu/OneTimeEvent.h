#pragma once

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include "assert.h"

class OneTimeEvent {
    boost::mutex _mutex;
    boost::condition_variable _cv;
    bool _fired = false;
    
public:
    inline void signal() {
        boost::unique_lock<boost::mutex> lock(_mutex);
        assert(!_fired);
        _fired = true;
        _cv.notify_all();
    }
    
    inline void wait() {
        boost::unique_lock<boost::mutex> lock(_mutex);
        _cv.wait(lock, [this] { return _fired; });
    }
};
