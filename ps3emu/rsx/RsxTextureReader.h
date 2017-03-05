#pragma once

#include "GLTexture.h"
#include "GLShader.h"
#include "GLBuffer.h"

class RsxTextureReader {
    Shader _shader;
    GLPersistentCpuBuffer _uniformBuffer;
    
public:
    void init();
    void loadTexture(RsxTextureInfo const& info, GLuint buffer, GLuint texture);
};
