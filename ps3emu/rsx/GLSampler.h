#pragma once

#include "GLUtils.h"
#include "HandleWrapper.h"
#include <glad/glad.h>

inline void deleteSampler(GLuint handle) {
    glDeleteSamplers(1, &handle);
}

class GLSampler : public HandleWrapper<GLuint, deleteSampler> {
public:
    GLSampler(GLuint handle) : HandleWrapper(handle) { }
    GLSampler() : HandleWrapper(init()) { }
    GLuint init() {
        GLuint handle;
        glcall(glCreateSamplers(1, &handle));
        return handle;
    }
};