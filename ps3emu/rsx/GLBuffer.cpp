#include "GLBuffer.h"

#include <stdexcept>

GLBuffer::GLBuffer() : HandleWrapper(0) { }

GLBuffer::GLBuffer(GLBufferType type, uint32_t size, const void* data) 
    : _size(size), _type(type) {
    if (!data && type == GLBufferType::Static)
        throw std::runtime_error("static buffer should be initialized with data");
    auto flags = type == GLBufferType::Dynamic ? GL_DYNAMIC_STORAGE_BIT
               : type == GLBufferType::MapRead ?
                    GL_MAP_READ_BIT
               : type == GLBufferType::MapWrite ?
                    GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT
               : 0;
    _mapFlags = type == GLBufferType::MapRead ? GL_MAP_READ_BIT
              : type == GLBufferType::MapWrite ? GL_MAP_WRITE_BIT
              : 0;
    glCreateBuffers(1, &_handle);
    glNamedBufferStorage(_handle, size, data, flags);
}

void* GLBuffer::map() {
    if (!_mapFlags || _mapped)
        throw std::runtime_error("buffer can't be mapped");
    _mapped = true;
    return glMapNamedBufferRange(handle(), 0, _size, _mapFlags);
}

void GLBuffer::unmap(bool sync) {
    if (!_mapFlags || !_mapped)
        throw std::runtime_error("buffer can't be unmapped");
    _mapped = false;
    glUnmapNamedBuffer(handle());
    if (sync && _type == GLBufferType::MapWrite) {
        //glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    }
}

void deleteBuffer(GLuint handle) {
    glDeleteBuffers(1, &handle);
}

GLBuffer& GLBuffer::operator=(GLBuffer&& other) {
    _mapFlags = other._mapFlags;
    _mapped = other._mapped;
    _size = other._size;
    _type = other._type;
    HandleWrapper::operator=(std::move(other));
    return *this;
}

uint32_t GLBuffer::size() {
    return _size;
}

GLPersistentCpuBuffer::GLPersistentCpuBuffer() : HandleWrapper(0) { }

GLPersistentCpuBuffer::GLPersistentCpuBuffer(uint32_t size, bool resident, bool client)
    : _size(size), _gpuPtr(0) {
    glCreateBuffers(1, &_handle);
    auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    if (client) {
        flags |= GL_CLIENT_STORAGE_BIT;
    }
    glNamedBufferStorage(_handle, size, 0, flags);
    if (resident) {
        glMakeNamedBufferResidentNV(_handle, GL_READ_ONLY);
        glGetNamedBufferParameterui64vNV(_handle, GL_BUFFER_GPU_ADDRESS_NV, &_gpuPtr);
    }
    auto mapFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    _ptr = glMapNamedBufferRange(_handle, 0, size, mapFlags);
}

uint8_t* GLPersistentCpuBuffer::mapped() {
    return (uint8_t*)_ptr;
}

uint32_t GLPersistentCpuBuffer::size() {
    return _size;
}

GLuint64 GLPersistentCpuBuffer::gpuPointer() {
    assert(_gpuPtr);
    return _gpuPtr;
}
