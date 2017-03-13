#include "GLShader.h"
#include "ps3emu/utils.h"

#include <boost/algorithm/string.hpp>
#include <vector>
#include <string>

std::string Shader::source() {
    return _source;
}

std::string Shader::log() {
    GLint len;
    glGetProgramiv(_program.handle(), GL_INFO_LOG_LENGTH, &len);
    std::string str;
    str.resize(len);
    glGetProgramInfoLog(_program.handle(), len, nullptr, &str[0]);
    return str;   
}

Shader::Shader(GLuint type, const char* text) : _program(create(type, text)) {
#if DEBUG || TESTS
    int i = 1;
    std::vector<std::string> split;
    boost::split(split, text, boost::is_any_of("\n"));
    for (auto l : split) {
        _source += ssnprintf("%03d:  %s\n", i, l);
        i++;
    }
#endif
}
