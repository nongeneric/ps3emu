#pragma once

#include "GLSampler.h"
#include "GLShader.h"
#include "Tracer.h"
#include "GLProgramPipeline.h"
#include "GLTexture.h"
#include "GLFramebuffer.h"
#include "Cache.h"
#include "../shaders/ShaderGenerator.h"

struct TextureSamplerInfo {
    bool enable = false;
    GLSampler glSampler = GLSampler(0);
    float minlod;
    float maxlod;
    uint32_t wraps;
    uint32_t wrapt;
    uint8_t fragmentWrapr;
    uint8_t fragmentZfunc;
    uint8_t fragmentAnisoBias;
    float bias;
    uint8_t fragmentMin;
    uint8_t fragmentMag;
    uint8_t fragmentConv;
    uint8_t fragmentAlphaKill;
    std::array<float, 4> borderColor;
    RsxTextureInfo texture;
};

struct DisplayBufferInfo {
    ps3_uintptr_t offset;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
};

struct ColorConvertionInfo {
    uint32_t fmt;
    int16_t clipX;
    int16_t clipY;
    uint16_t clipW;
    uint16_t clipH;
    int16_t outX;
    int16_t outY;
    uint16_t outW;
    uint16_t outH;
    float dsdx;
    float dtdy;
};

struct TransferInfo {
    uint32_t format;
    uint32_t destOffset;
    uint32_t sourceOffset;
    uint32_t surfaceOffsetDestin;
    int32_t destPitch;
    int32_t sourcePitch;
    uint32_t lineLength;
    uint32_t lineCount;
    uint32_t sourceFormat;
    uint32_t destFormat;
    uint16_t pointX; 
    uint16_t pointY; 
    uint16_t outSizeX;
    uint16_t outSizeY; 
    uint16_t inSizeX;
    uint16_t inSizeY;
    MemoryLocation destTransferLocation = MemoryLocation::Local;
    MemoryLocation sourceDataLocation = MemoryLocation::Local;
    MemoryLocation destDataLocation = MemoryLocation::Local;
    MemoryLocation sourceImageLocation = MemoryLocation::Local;
    uint32_t surfaceType;
    ColorConvertionInfo conv;
};

struct TextureCacheKey {
    uint32_t offset;
    uint32_t location;
    uint32_t width;
    uint32_t height;
    uint8_t internalType;
    inline bool operator<(TextureCacheKey const& other) const {
        return std::tie(offset, location, width, height, internalType)
             < std::tie(other.offset, other.location, other.width, other.height, other.internalType);
    }
};

struct VertexShaderCacheKey {
    std::vector<uint8_t> bytecode;
    std::array<uint8_t, 16> arraySizes;
    inline bool operator<(VertexShaderCacheKey const& other) const {
        auto size = bytecode.size();
        auto othersize = other.bytecode.size();
        return std::tie(size, bytecode, arraySizes)
             < std::tie(othersize, other.bytecode, other.arraySizes);
    }
};

struct FragmentShaderCacheKey {
    uint32_t va;
    uint32_t size;
    inline bool operator<(FragmentShaderCacheKey const& other) const {
        return std::tie(va, size)
             < std::tie(other.va, other.size);
    }
};

struct ViewPortInfo {
    float x = 0;
    float y = 0;
    float width = 4096;
    float height = 4096;
    float zmin = 0;
    float zmax = 1;
    float scale[3] = { 2048, 2048, 0.5 };
    float offset[3] = { 2048, 2048, 0.5 };
};

struct VertexDataArrayFormatInfo {
    uint16_t frequency;
    uint8_t stride;
    uint8_t size;
    uint8_t type;
    MemoryLocation location;
    uint32_t offset;
    GLuint binding = 0;
};

struct IndexArrayInfo {
    uint32_t offset = 0;
    MemoryLocation location = MemoryLocation::Local;
    GLuint glType = 0;
};

class FragmentShaderUpdateFunctor;
class GLFramebuffer;
class TextureRenderer;
class RsxContext {
public:
    Tracer tracer;
    uint32_t frame = 0;
    uint32_t commandNum = 0;
    SurfaceInfo surface;
    ViewPortInfo viewPort;
    uint32_t colorMask;
    bool isDepthTestEnabled;
    uint32_t depthFunc;
    bool isCullFaceEnabled;
    bool isFlatShadeMode;
    glm::vec4 colorClearValue;
    GLuint glClearSurfaceMask;
    std::array<VertexDataArrayFormatInfo, 16> vertexDataArrays;
    GLuint glVertexArrayMode;
    VertexShader* vertexShader = nullptr;
    FragmentShader* fragmentShader = nullptr;
    GLPersistentCpuBuffer vertexConstBuffer;
    GLPersistentCpuBuffer vertexSamplersBuffer;
    GLPersistentCpuBuffer vertexViewportBuffer;
    GLPersistentCpuBuffer fragmentSamplersBuffer;
    GLPersistentCpuBuffer elementArrayIndexBuffer;
    bool vertexShaderDirty = false;
    bool fragmentShaderDirty = false;
    uint32_t fragmentVa = 0;
    std::vector<uint8_t> lastFrame;
    std::array<VertexShaderInputFormat, 16> vertexInputs;
    std::array<uint8_t, 512 * 16> vertexInstructions;
    uint32_t vertexLoadOffset = 0;
    IndexArrayInfo indexArray;
    TextureSamplerInfo vertexTextureSamplers[4];
    TextureSamplerInfo fragmentTextureSamplers[16];
    DisplayBufferInfo displayBuffers[8];
    std::unique_ptr<GLFramebuffer> framebuffer;
    std::unique_ptr<TextureRenderer> textureRenderer;
    uint32_t semaphoreOffset = 0;
    TransferInfo transfer;
    GLProgramPipeline pipeline;
    GLPersistentCpuBuffer mainMemoryBuffer;
    GLPersistentCpuBuffer localMemoryBuffer;
    Cache<TextureCacheKey, GLTexture, 256 * (2 >> 20)> textureCache;
    Cache<VertexShaderCacheKey, VertexShader, 10 * (2 >> 20)> vertexShaderCache;
    Cache<FragmentShaderCacheKey, FragmentShader, 10 * (2 >> 20), FragmentShaderUpdateFunctor> fragmentShaderCache;
    uint32_t vBlankHandlerDescr = 0;
    MemoryLocation reportLocation = MemoryLocation::Local;
    
    inline void trace(CommandId id, std::vector<GcmCommandArg> const& args) {
        tracer.trace(frame, commandNum++, id, args);
    }
};