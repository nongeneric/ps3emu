#pragma once

#include "GLUtils.h"
#include "HandleWrapper.h"
#include <glad/glad.h>

inline void deleteSampler(GLuint handle) {
    glDeleteSamplers(1, &handle);
}

class GLSampler : public HandleWrapper<GLuint, deleteSampler> {
public:
    inline GLSampler(GLuint handle) : HandleWrapper(handle) { }
    inline GLSampler() {
        glCreateSamplers(1, &_handle);
    }
};
