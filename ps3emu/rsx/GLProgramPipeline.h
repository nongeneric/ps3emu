#pragma once

#include "GLUtils.h"
#include "GLShader.h"
#include "HandleWrapper.h"
#include <glad/glad.h>

inline void deleteProgramPipeline(GLuint handle) {
    glDeleteProgramPipelines(1, &handle);
}

class GLProgramPipeline : public HandleWrapper<GLuint, deleteProgramPipeline> {
    GLuint create() {
        GLuint handle = 0;
        glcall(glGenProgramPipelines(1, &handle));
        return handle;
    }
public:
    GLProgramPipeline() : HandleWrapper(create()) { }
    void bind() {
        glcall(glBindProgramPipeline(handle()));
    }
    void useShader(VertexShader& shader) {
        glcall(glUseProgramStages(handle(), GL_VERTEX_SHADER_BIT, shader.handle()));
    }
    void useShader(FragmentShader& shader) {
        glcall(glUseProgramStages(handle(), GL_FRAGMENT_SHADER_BIT, shader.handle()));
    }
    void validate() {
        glcall(glValidateProgramPipeline(handle()));
    }
};