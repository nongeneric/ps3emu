#pragma once

#include "HandleWrapper.h"
#include <glad/glad.h>

inline void deleteSync(GLsync sync) {
    glDeleteSync(sync);
}

class GLSync : public HandleWrapper<GLsync, deleteSync> {
public:
    inline GLSync(GLsync handle) : HandleWrapper(handle) { }
    inline GLSync() {
        _handle = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    inline void serverWait() {
        if (_handle) {
            glWaitSync(_handle, 0, GL_TIMEOUT_IGNORED);
            deleteSync(_handle);
            _handle = 0;
        }
    }

    inline void clientWait(bool flush = true) {
        if (_handle) {
            glClientWaitSync(_handle, flush ? GL_SYNC_FLUSH_COMMANDS_BIT : 0, -1);
            deleteSync(_handle);
            _handle = 0;
        }
    }
};
