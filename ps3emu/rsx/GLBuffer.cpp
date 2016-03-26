#include "GLBuffer.h"

#include <stdexcept>

GLBuffer::GLBuffer() : HandleWrapper(0) { }

GLBuffer::GLBuffer(GLBufferType type, uint32_t size, void* data) 
    : HandleWrapper(init(type, size, data)), _size(size), _type(type) { }

GLuint GLBuffer::init(GLBufferType type, uint32_t size, void* data) {
    if (!data && type == GLBufferType::Static)
        throw std::runtime_error("static buffer should be initialized with data");
    GLuint handle;
    auto flags = type == GLBufferType::Dynamic ? GL_DYNAMIC_STORAGE_BIT
               : type == GLBufferType::MapRead ? 
                    GL_MAP_READ_BIT
               : type == GLBufferType::MapWrite ?
                    GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT
               : 0;
    _mapFlags = type == GLBufferType::MapRead ? GL_MAP_READ_BIT
              : type == GLBufferType::MapWrite ? GL_MAP_WRITE_BIT
              : 0;
    glcall(glCreateBuffers(1, &handle));
    glcall(glNamedBufferStorage(handle, size, data, flags));
    return handle;
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
    glcall(glUnmapNamedBuffer(handle()));
    if (sync && _type == GLBufferType::MapWrite) {
        //glcall(glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT));
    }
}

void deleteBuffer(GLuint handle) {
    glcall(glDeleteBuffers(1, &handle));
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

GLPersistentBuffer::GLPersistentBuffer() : HandleWrapper(0) { }

GLPersistentBuffer& GLPersistentBuffer::operator=(GLPersistentBuffer&& other) {
    _ptr = other._ptr;
    HandleWrapper::operator=(std::move(other));
    return *this;
}

GLPersistentBuffer::GLPersistentBuffer(uint32_t size)
    : HandleWrapper(init(size)), _size(size) {}

void* GLPersistentBuffer::mapped() {
    return _ptr;
}

void GLPersistentBuffer::flush(uint32_t offset, uint32_t size) {
    //glFlushMappedNamedBufferRange(handle(), offset, size);
    //glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
}

GLuint GLPersistentBuffer::init(uint32_t size) {
    GLuint handle;
    glCreateBuffers(1, &handle);
//     auto flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
//     glNamedBufferStorage(handle, size, 0, flags);
//     auto mapFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT |
//                     GL_MAP_FLUSH_EXPLICIT_BIT;
    auto flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    glNamedBufferStorage(handle, size, 0, flags);
    auto mapFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    _ptr = glMapNamedBufferRange(handle, 0, size, mapFlags);
    return handle;
}

uint32_t GLPersistentBuffer::size() {
    return _size;
}

