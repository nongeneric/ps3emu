#pragma once

#include "HandleWrapper.h"
#include "GLUtils.h"

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
    GLuint init(GLBufferType type, uint32_t size, void* data);
    GLBuffer(GLBuffer&) = delete;
    GLBuffer& operator=(GLBuffer&) = delete;
public:
    GLBuffer();
    GLBuffer& operator=(GLBuffer&& other);
    GLBuffer(GLBufferType type, uint32_t size, void* data = nullptr);
    uint32_t size();
    void* map();
    void unmap(bool sync = true);
};

class GLPersistentGpuBuffer : public HandleWrapper<GLuint, deleteBuffer> {
    GLuint init(uint32_t size);
    uint32_t _size;
public:
    GLPersistentGpuBuffer();
    GLPersistentGpuBuffer& operator=(GLPersistentGpuBuffer&& other);
    GLPersistentGpuBuffer(uint32_t size);
    uint32_t size() const;
};

class GLPersistentCpuBuffer : public HandleWrapper<GLuint, deleteBuffer> {
    GLuint init(uint32_t size);
    uint32_t _size;
    void* _ptr;
public:
    GLPersistentCpuBuffer();
    GLPersistentCpuBuffer& operator=(GLPersistentCpuBuffer&& other);
    GLPersistentCpuBuffer(uint32_t size);
    uint8_t* mapped();
    uint32_t size();
};