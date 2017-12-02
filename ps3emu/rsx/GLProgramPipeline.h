#pragma once

#include "GLShader.h"
#include "HandleWrapper.h"
#include "ps3emu/log.h"
#include "ps3emu/utils.h"
#include <glad/glad.h>
#include <string>

inline void deleteProgramPipeline(GLuint handle) {
    glDeleteProgramPipelines(1, &handle);
}

class GLProgramPipeline : public HandleWrapper<GLuint, deleteProgramPipeline> {
    static GLuint create() {
        GLuint handle = 0;
        glGenProgramPipelines(1, &handle);
        return handle;
    }

    VertexShader* _vertex = nullptr;
    FragmentShader* _fragment = nullptr;
    GeometryShader* _geometry = nullptr;

public:
    inline GLProgramPipeline() : HandleWrapper(create()) { }
    inline void bind() {
        glBindProgramPipeline(handle());
    }
    inline void useShader(VertexShader* shader) {
        if (shader == _vertex)
            return;
        _vertex = shader;
        glUseProgramStages(handle(), GL_VERTEX_SHADER_BIT, shader->handle());
    }
    inline void useShader(FragmentShader* shader) {
        if (shader == _fragment)
            return;
        _fragment = shader;
        glUseProgramStages(handle(), GL_FRAGMENT_SHADER_BIT, shader->handle());
    }
    inline void useShader(GeometryShader* shader) {
        if (shader == _geometry)
            return;
        _geometry = shader;
        glUseProgramStages(handle(), GL_GEOMETRY_SHADER_BIT, shader->handle());
    }
    inline void validate() {
        glValidateProgramPipeline(handle());
        GLint status;
        glGetProgramPipelineiv(handle(), GL_VALIDATE_STATUS, &status);
        if (!status) {
            GLint len;
            glGetProgramPipelineiv(handle(), GL_INFO_LOG_LENGTH, &len);
            std::string log;
            log.resize(len);
            glGetProgramPipelineInfoLog(handle(), log.size(), nullptr, &log[0]);
            ERROR(rsx) << ssnprintf("pipeline validation failed\n%s", log);
            exit(1);
        }
    }
    inline void useDefaultShaders() {
        auto code = R""(
            #version 450 core
            void main(void) { }
        )"";
        auto fs = FragmentShader(code);
        auto vs = VertexShader(code);
        useShader(&fs);
        useShader(&vs);
        validate();
    }
};
