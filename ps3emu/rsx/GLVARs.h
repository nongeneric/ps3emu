#pragma once

#include <array>
#include "assert.h"
#include <glm/glm.hpp>

struct GLVARState {
    bool enabled = false;
    glm::vec4 value = {0, 0, 0, 1};
};

class GLVARs {
    std::array<GLVARState, GL_MAX_VERTEX_ATTRIBS> _arr;

public:
    inline void enable(int i) {
        assert(i < _arr.size());
        if (!_arr[i].enabled) {
            glEnableVertexAttribArray(i);
            _arr[i].enabled = true;
        }
    }

    inline void disable(int i) {
        assert(i < _arr.size());
        if (_arr[i].enabled) {
            glDisableVertexAttribArray(i);
            _arr[i].enabled = false;
        }
    }

    inline void setValue(int i, glm::vec4 const& value) {
        assert(i < _arr.size());
        if (_arr[i].value != value) {
            glVertexAttrib4f(i, value.r, value.g, value.b, value.a);
            _arr[i].value = value;
        }
    }
};
