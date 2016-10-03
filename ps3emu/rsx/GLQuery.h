#pragma once

#include "HandleWrapper.h"
#include <glad/glad.h>

inline void deleteQuery(GLuint handle) {
    glDeleteQueries(1, &handle);
}

class GLQuery : public HandleWrapper<GLuint, deleteQuery> {
public:
    inline GLQuery(GLuint handle) : HandleWrapper(handle) { }
    inline GLQuery() {
        glGenQueries(1, &_handle);
    }
};
