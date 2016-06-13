#pragma once

#include <glad/glad.h>
#include <assert.h>
#include <algorithm>

template <typename H, void (*Deleter)(H)>
class HandleWrapper {
protected:
    H _handle = H();
public:
    HandleWrapper(H handle = H()) : _handle(handle) { }
    
    HandleWrapper(HandleWrapper&& other) {
        _handle = other._handle;
        other._handle = H();
    }
    
    HandleWrapper& operator=(HandleWrapper&& other) {
        std::swap(_handle, other._handle);
        return *this;
    }
    
    virtual ~HandleWrapper() {
        if (_handle != H()) {
            Deleter(_handle);
        }
    }
    
    GLuint handle() {
        return _handle;
    }
};
