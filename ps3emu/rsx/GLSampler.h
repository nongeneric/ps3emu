#pragma once

#include "GLUtils.h"
#include "HandleWrapper.h"
#include <glad/glad.h>

inline void deleteSampler(GLuint handle) {
    glDeleteSamplers(1, &handle);
}

class GLSampler : public HandleWrapper<GLuint, deleteSampler> {
    float _minLOD = 0;
    float _maxLOD = 0;
    float _lodBias = 0;
    GLenum _minFilter = 0;
    GLenum _magFilter = 0;
    GLenum _wrapS = 0;
    GLenum _wrapT = 0;

public:
    inline GLSampler(GLuint handle) : HandleWrapper(handle) { }
    inline GLSampler() {
        glCreateSamplers(1, &_handle);
    }

    inline void setMinLOD(float v) {
        if (v != _minLOD) {
            glSamplerParameterf(handle(), GL_TEXTURE_MIN_LOD, v);
            _minLOD = v;
        }
    }

    inline void setMaxLOD(float v) {
        if (v != _maxLOD) {
            glSamplerParameterf(handle(), GL_TEXTURE_MAX_LOD, v);
            _maxLOD = v;
        }
    }

    inline void setLODBias(float v) {
        if (v != _lodBias) {
            glSamplerParameterf(handle(), GL_TEXTURE_LOD_BIAS, v);
            _lodBias = v;
        }
    }

    inline void setMinFilter(GLenum v) {
        if (v != _minFilter) {
            glSamplerParameteri(handle(), GL_TEXTURE_MIN_FILTER, v);
            _minFilter = v;
        }
    }

    inline void setMagFilter(GLenum v) {
        if (v != _magFilter) {
            glSamplerParameteri(handle(), GL_TEXTURE_MAG_FILTER, v);
            _magFilter = v;
        }
    }

    inline void setWrapS(GLenum v) {
        if (v != _wrapS) {
            glSamplerParameteri(handle(), GL_TEXTURE_WRAP_S, v);
            _wrapS = v;
        }
    }

    inline void setWrapT(GLenum v) {
        if (v != _wrapT) {
            glSamplerParameteri(handle(), GL_TEXTURE_WRAP_T, v);
            _wrapT = v;
        }
    }
};
