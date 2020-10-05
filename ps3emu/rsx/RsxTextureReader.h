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
    RsxTextureReader();
    void loadTexture(RsxTextureInfo const& info,
                     GLuint buffer,
                     std::vector<uint64_t> const& levelHandles);
};
