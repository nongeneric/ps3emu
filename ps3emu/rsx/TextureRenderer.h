#pragma once

#include "GLProgramPipeline.h"
#include "GLShader.h"
#include "GLSampler.h"
#include "GLBuffer.h"

class GLSimpleTexture;

class TextureRenderer {
    GLProgramPipeline _pipeline;
    VertexShader _vertexShader;
    FragmentShader _fragmentShader;
    GLSampler _sampler;
    GLBuffer _buffer;
public:
    TextureRenderer();
    void render(GLSimpleTexture* tex);
};