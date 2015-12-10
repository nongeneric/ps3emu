#include "GLBuffer.h"

#include <stdexcept>

GLBuffer::GLBuffer() : HandleWrapper(0) { }

GLBuffer::GLBuffer(GLBufferType type, uint32_t size, void* data) 
    : HandleWrapper(init(type, size, data)), _size(size) { }

GLuint GLBuffer::init(GLBufferType type, uint32_t size, void* data) {
    if (!data && type == GLBufferType::Static)
        throw std::runtime_error("static buffer should be initilized with data");
    GLuint handle;
    auto flags = type == GLBufferType::Dynamic ? GL_DYNAMIC_STORAGE_BIT
               : type == GLBufferType::MapRead ? 
                    GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT
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

void GLBuffer::unmap() {
    if (!_mapFlags || !_mapped)
        throw std::runtime_error("buffer can't be unmapped");
    _mapped = false;
    glcall(glUnmapNamedBuffer(handle()));
}

void deleteBuffer(GLuint handle) {
    glcall(glDeleteBuffers(1, &handle));
}

GLBuffer& GLBuffer::operator=(GLBuffer&& other) {
    _mapFlags = other._mapFlags;
    _mapped = other._mapped;
    _size = other._size;
    HandleWrapper::operator=(std::move(other));
    return *this;
}
