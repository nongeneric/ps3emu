#pragma once

#include <glad/glad.h>
#include <algorithm>

template <typename H, void (*Deleter)(H)>
class HandleWrapper {
    H _handle = H();
public:
    HandleWrapper(H handle) : _handle(handle) { }
    
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