#pragma once

#include <atomic>

class spinlock {
    std::atomic<int> _m;
public:
    inline spinlock() : _m(0) { }

    inline void lock() {
        while (_m.exchange(1) == 1) ;
    }
    
    inline void unlock() {
        _m.store(0);
    }
    
    inline void wait() const {
        while (_m.load() == 1) ;
    }
    
    inline bool is_locked() const {
        return _m.load();
    }
};