#pragma once

#include "GLSampler.h"
#include "GLShader.h"
#include "Tracer.h"
#include "GLProgramPipeline.h"
#include "GLTexture.h"
#include "GLFramebuffer.h"
#include "Cache.h"
#include "Rsx.h"
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

enum class ScaleSettingsSurfaceType {
    Linear, Swizzle
};

enum class ScaleSettingsFormat {
    r5g6b5, a8r8g8b8, y32
};

struct ScaleSettings {
    ScaleSettingsSurfaceType type = ScaleSettingsSurfaceType::Linear;
    ScaleSettingsFormat format = ScaleSettingsFormat::r5g6b5;
    int16_t clipX = 0;
    int16_t clipY = 0;
    uint16_t clipW = 0;
    uint16_t clipH = 0;
    int16_t outX = 0;
    int16_t outY = 0;
    uint16_t outW = 0;
    uint16_t outH = 0;
    uint16_t inW = 0;
    uint16_t inH = 0;
    uint16_t pitch = 0;
    uint32_t offset = 0;
    float inX = 0;
    float inY = 0;
    
    float dsdx = 0;
    float dtdy = 0;
    MemoryLocation location = MemoryLocation::Local;
};

struct SwizzleSettings {
    ScaleSettingsFormat format = ScaleSettingsFormat::r5g6b5;
    uint8_t logWidth = 0;
    uint8_t logHeight = 0;
    uint32_t offset = 0;
    MemoryLocation location = MemoryLocation::Local;
};

struct SurfaceSettings {
    ScaleSettingsFormat format = ScaleSettingsFormat::r5g6b5;
    int16_t pitch = 0;
    uint32_t destOffset = 0;
    MemoryLocation destLocation = MemoryLocation::Local;
};

struct CopySettings {
    uint32_t srcOffset = 0;
    uint32_t destOffset = 0;
    MemoryLocation srcLocation = MemoryLocation::Local;
    MemoryLocation destLocation = MemoryLocation::Local;
    int32_t srcPitch = 0;
    int32_t destPitch = 0;
    uint32_t lineLength = 0;
    uint32_t lineCount = 0;
    uint8_t srcFormat = 0;
    uint8_t destFormat = 0;
};

struct InlineSettings {
    uint32_t pointX = 0;
    uint32_t pointY = 0;
    uint32_t srcSizeX = 0;
    uint32_t srcSizeY = 0;
    uint32_t destSizeX = 0;
    uint32_t destSizeY = 0;
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

struct SurfaceToFramebufferLink {
    uint32_t surfaceEa;
    uint32_t framebufferEa;
    inline bool operator<(SurfaceToFramebufferLink const& other) const {
        return surfaceEa < other.surfaceEa;
    }
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
    std::array<DisplayBufferInfo, 8> displayBuffers;
    std::unique_ptr<GLFramebuffer> framebuffer;
    std::unique_ptr<TextureRenderer> textureRenderer;
    uint32_t semaphoreOffset = 0;
    GLProgramPipeline pipeline;
    GLPersistentCpuBuffer mainMemoryBuffer;
    GLPersistentCpuBuffer localMemoryBuffer;
    Cache<TextureCacheKey, GLTexture, 256 * (2 >> 20)> textureCache;
    Cache<VertexShaderCacheKey, VertexShader, 10 * (2 >> 20)> vertexShaderCache;
    Cache<FragmentShaderCacheKey, FragmentShader, 10 * (2 >> 20), FragmentShaderUpdateFunctor> fragmentShaderCache;
    uint32_t vBlankHandlerDescr = 0;
    uint32_t flipHandlerDescr = 0;
    MemoryLocation reportLocation;
    
    ScaleSettings scale2d;
    SwizzleSettings swizzle2d;
    SurfaceSettings surface2d;
    CopySettings copy2d;
    InlineSettings inline2d;
    
    std::set<SurfaceToFramebufferLink> surfaceLinks;
    
    inline void trace(CommandId id, std::vector<GcmCommandArg> const& args) {
        tracer.trace(frame, commandNum++, id, args);
    }
};