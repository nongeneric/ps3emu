#pragma once

#include "GLProgram.h"
#include <glad/glad.h>
#include <string>

class Shader {
    GLProgram _program;
    GLuint create(GLuint type, const char* text) {
        return glCreateShaderProgramv(type, 1, &text);
    }
public:
    Shader() : _program(0) { }
    Shader(GLuint type, const char* text) : _program(create(type, text)) {}
    GLuint handle() {
        return _program.handle();
    }
    std::string log() {
        GLint len;
        glcall(glGetProgramiv(_program.handle(), GL_INFO_LOG_LENGTH, &len));
        std::string str;
        str.resize(len);
        glcall(glGetProgramInfoLog(_program.handle(), len, nullptr, &str[0]));
        return str;   
    }
};

class VertexShader : public Shader {
public:
    VertexShader() {}
    VertexShader(const char* text) : Shader(GL_VERTEX_SHADER, text) {}
};

class FragmentShader : public Shader {
public:
    FragmentShader() {}
    FragmentShader(const char* text) : Shader(GL_FRAGMENT_SHADER, text) {}
};
