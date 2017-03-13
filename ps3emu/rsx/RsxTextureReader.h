#pragma once

#include "GLTexture.h"
#include "GLShader.h"
#include "GLBuffer.h"

class RsxTextureReader {
    Shader _destFirstShader;
    Shader _sourceFirstShader;
#ifdef DEBUG
    Shader _debugFillShader;
#endif
    GLPersistentCpuBuffer _uniformBuffer;
    
public:
    void init();
    void loadTexture(RsxTextureInfo const& info,
                     GLuint buffer,
                     GLuint texture,
                     std::vector<uint64_t> const& levelHandles);
};
