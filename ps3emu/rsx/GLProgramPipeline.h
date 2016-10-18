#pragma once

#include "GLUtils.h"
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
        glcall(glGenProgramPipelines(1, &handle));
        return handle;
    }
public:
    inline GLProgramPipeline() : HandleWrapper(create()) { }
    inline void bind() {
        glBindProgramPipeline(handle());
    }
    inline void useShader(VertexShader& shader) {
        glUseProgramStages(handle(), GL_VERTEX_SHADER_BIT, shader.handle());
    }
    inline void useShader(FragmentShader& shader) {
        glUseProgramStages(handle(), GL_FRAGMENT_SHADER_BIT, shader.handle());
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
        }
    }
};
