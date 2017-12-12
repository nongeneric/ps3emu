#pragma once

#include <atomic>

class SpinLock {
    std::atomic_flag _flag = ATOMIC_FLAG_INIT;

public:
    inline void lock() {
        while (_flag.test_and_set(std::memory_order_acquire)) ;
    }

    inline bool try_lock() {
        return !_flag.test_and_set(std::memory_order_acquire);
    }
    
    inline void unlock() {
        _flag.clear(std::memory_order_release);
    }
};
