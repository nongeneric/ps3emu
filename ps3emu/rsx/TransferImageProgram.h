#pragma once

#include "RsxContext.h"
#include "GLShader.h"
#include "GLBuffer.h"

class TransferImageProgram {
    Shader _shader;
    GLPersistentCpuBuffer _uniformBuffer;

public:
    TransferImageProgram();
    void transfer(GLuint srcBuffer,
                  GLuint dstBuffer,
                  const ScaleSettings& scale,
                  const SurfaceSettings& surface,
                  const SwizzleSettings& swizzle);
};
