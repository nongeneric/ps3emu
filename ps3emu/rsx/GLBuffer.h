#pragma once

#include "HandleWrapper.h"

#include <glad/glad.h>
#include <algorithm>
#include <stdint.h>

enum class GLBufferType {
    MapRead, MapWrite, Dynamic, Static
};

void deleteBuffer(GLuint handle);

class GLBuffer : public HandleWrapper<GLuint, deleteBuffer> {
    GLuint _mapFlags;
    bool _mapped = false;
    uint32_t _size;
    GLBufferType _type;
    GLBuffer(GLBuffer&) = delete;
    GLBuffer& operator=(GLBuffer&) = delete;
public:
    GLBuffer();
    GLBuffer& operator=(GLBuffer&& other);
    GLBuffer(GLBufferType type, uint32_t size, const void* data = nullptr);
    uint32_t size();
    void* map();
    void unmap(bool sync = true);
};

class GLPersistentCpuBuffer : public HandleWrapper<GLuint, deleteBuffer> {
    uint32_t _size;
    void* _ptr;
    GLuint64 _gpuPtr;
public:
    GLPersistentCpuBuffer();
    GLPersistentCpuBuffer(uint32_t size, bool resident = false, bool client = true);
    uint8_t* mapped();
    uint32_t size();
    GLuint64 gpuPointer();
};
