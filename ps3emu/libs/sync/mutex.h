#pragma once

#include "../sys_defs.h"
#include "ps3emu/state.h"
#include <boost/chrono.hpp>
#include <memory>
#include <map>

typedef big_uint32_t sys_mutex_t;

class IMutex {
public:
    virtual bool lock(usecond_t timeout = 0) = 0;
    virtual void unlock() = 0;
    virtual bool try_lock(usecond_t timeout) = 0;
    virtual void destroy() = 0;
    virtual PPUThread* owner() = 0;
    virtual std::string name() = 0;
    virtual ~IMutex() = default;
};

template <typename M>
class Mutex : public IMutex {
    M* _m;
    PPUThread* _owner;
    std::string _name;
public:
    Mutex(std::string name) : _m(new M()), _owner(nullptr), _name(name) { }
    
    bool lock(usecond_t timeout) override {
        bool locked = false;
        if (timeout == 0) {
            _m->lock();
            locked = true;
        } else {
            locked = _m->try_lock_for( boost::chrono::microseconds(timeout) );
        }
        if (locked) {
            _owner = g_state.th;
        }
        return locked;
    }
    
    void unlock() override {
        _owner = nullptr;
        _m->unlock();
    }
    
    bool try_lock(usecond_t timeout) override {
        if (timeout == 0) {
            return _m->try_lock();
        } else {
            return _m->try_lock_for( boost::chrono::microseconds(timeout) );
        }
    }
    
    PPUThread* owner() override {
        return _owner;
    }
    
    std::string name() override {
        return _name;
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
std::map<uint32_t, std::shared_ptr<IMutex>> dbgDumpMutexes();
