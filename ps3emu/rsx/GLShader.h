#pragma once

#include "GLProgram.h"
#include <glad/glad.h>
#include <string>

class Shader {
    GLProgram _program;
    std::string _source;
    
    inline GLuint create(GLuint type, const char* text) {
        return glCreateShaderProgramv(type, 1, &text);
    }
    
public:
    inline Shader() : _program(0) { }
    Shader(GLuint type, const char* text);
    inline GLuint handle() {
        return _program.handle();
    }
    std::string log();
    std::string source();
    // TODO: ~Shader()
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

class GeometryShader : public Shader {
public:
    GeometryShader() {}
    GeometryShader(const char* text) : Shader(GL_GEOMETRY_SHADER, text) {}
};
