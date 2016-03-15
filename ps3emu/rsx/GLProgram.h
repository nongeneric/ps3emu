#pragma once

#include "HandleWrapper.h"
#include "GLUtils.h"

inline void deleteProgram(GLuint handle) {
    glDeleteProgram(handle);
}

class GLProgram : public HandleWrapper<GLuint, deleteProgram> {
public:
    inline GLProgram(GLuint handle) : HandleWrapper(handle) { }
    inline GLProgram() : HandleWrapper(glCreateProgram()) { }
};