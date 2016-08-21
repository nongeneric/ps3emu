#pragma once

#include "../sys_defs.h"
#include <boost/chrono.hpp>
#include <memory>

typedef big_uint32_t sys_mutex_t;

class IMutex {
public:
    virtual bool lock(usecond_t timeout = 0) = 0;
    virtual void unlock() = 0;
    virtual bool try_lock(usecond_t timeout) = 0;
    virtual void destroy() = 0;
    virtual ~IMutex() = default;
};

template <typename M>
class Mutex : public IMutex {
    M* _m;
public:
    Mutex() : _m(new M()) { }
    
    bool lock(usecond_t timeout) override {
        if (timeout == 0) {
            _m->lock();
            return true;
        } else {
            return _m->try_lock_for( boost::chrono::microseconds(timeout) );
        }
    }
    
    void unlock() override {
        _m->unlock();
    }
    
    bool try_lock(usecond_t timeout) override {
        if (timeout == 0) {
            return _m->try_lock();
        } else {
            return _m->try_lock_for( boost::chrono::microseconds(timeout) );
        }
    }
    
    void destroy() override {
        delete _m;
    }
};

struct sys_mutex_attribute_t {
    sys_protocol_t attr_protocol;
    sys_recursive_t attr_recursive;
    sys_process_shared_t attr_pshared;
    sys_adaptive_t attr_adaptive;
    sys_ipc_key_t key;
    big_uint32_t flags;
    uint32_t pad;
    char name[SYS_SYNC_NAME_SIZE];
};
static_assert(sizeof(sys_mutex_attribute_t) == 40, "");

int sys_mutex_create(sys_mutex_t* mutex_id, sys_mutex_attribute_t* attr);
int sys_mutex_destroy(sys_mutex_t mutex_id);
int sys_mutex_lock(sys_mutex_t mutex_id, usecond_t timeout);
int sys_mutex_trylock(sys_mutex_t mutex_id);
int sys_mutex_unlock(sys_mutex_t mutex_id);
std::shared_ptr<IMutex> find_mutex(sys_mutex_t id);
