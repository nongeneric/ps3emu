#pragma once

#include "GLProgram.h"
#include <glad/glad.h>
#include <string>

template <GLuint Type>
class Shader {
    GLProgram _program;
    GLuint create(const char* text) {
        return glCreateShaderProgramv(Type, 1, &text);
    }
public:
    Shader() : _program(0) { }
    Shader(const char* text) : _program(create(text)) { }
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

class VertexShader : public Shader<GL_VERTEX_SHADER> {
public:
    VertexShader() { }
    VertexShader(const char* text) : Shader(text) { }
};

class FragmentShader : public Shader<GL_FRAGMENT_SHADER> {
public:
    FragmentShader() { }
    FragmentShader(const char* text) : Shader(text) { }
};