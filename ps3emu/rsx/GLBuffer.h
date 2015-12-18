#pragma once

#include <glad/glad.h>
#include <algorithm>
#include <stdint.h>

#define glcall(a) { a; glcheck(__LINE__, #a); }
void glcheck(int line, const char* call);

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

enum class GLBufferType {
    MapRead, MapWrite, Dynamic, Static
};

void deleteBuffer(GLuint handle);

class GLBuffer : public HandleWrapper<GLuint, deleteBuffer> {
    GLuint _mapFlags;
    bool _mapped = false;
    uint32_t _size;
    GLBufferType _type;
    GLuint init(GLBufferType type, uint32_t size, void* data);
    GLBuffer(GLBuffer&) = delete;
    GLBuffer& operator=(GLBuffer&) = delete;
public:
    GLBuffer();
    GLBuffer& operator=(GLBuffer&& other);
    GLBuffer(GLBufferType type, uint32_t size, void* data = nullptr);
    void* map();
    void unmap(bool sync = true);
};