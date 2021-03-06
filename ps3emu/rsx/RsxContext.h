#pragma once

#include "GLSampler.h"
#include "GLShader.h"
#include "Tracer.h"
#include "GLProgramPipeline.h"
#include "GLTexture.h"
#include "GLFramebuffer.h"
#include "GLSync.h"
#include "GLProxy.h"
#include "GLVARs.h"
#include "RingBuffer.h"
#include "TextRenderer.h"
#include "RsxTypes.h"
#include "Cache.h"
#include "ps3emu/TimedCounter.h"
#include "GcmConstants.h"
#include "VertexShaderCache.h"
#include "FragmentShaderCache.h"
#include "../shaders/ShaderGenerator.h"
#include <boost/chrono.hpp>

struct TextureSamplerInfo {
    bool enable = false;
    GLSampler glSampler = GLSampler(0);
    float minlod = 0;
    float maxlod = 0;
    uint32_t wraps = 0;
    uint32_t wrapt = 0;
    uint8_t fragmentWrapr = 0;
    uint8_t fragmentZfunc = 0;
    uint8_t fragmentAnisoBias = 0;
    float bias = 0;
    uint8_t fragmentMin = 0;
    uint8_t fragmentMag = 0;
    uint8_t fragmentConv = 0;
    uint8_t fragmentAlphaKill = 0;
    std::array<float, 4> borderColor;
    RsxTextureInfo texture;
};

struct DisplayBufferInfo {
    ps3_uintptr_t offset = 0;
    uint32_t pitch = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

enum class ScaleSettingsSurfaceType {
    Linear, Swizzle
};

ENUM(ScaleSettingsFormat,
    (r5g6b5, CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5),
    (a8r8g8b8, CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8),
    (y32, CELL_GCM_TRANSFER_SURFACE_FORMAT_Y32)
)

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
    GcmTextureFormat internalType;
    GcmTextureLnUn lnUn;
    inline bool operator<(TextureCacheKey const& other) const {
        return std::tie(offset, location, width, height, internalType, lnUn) <
               std::tie(other.offset,
                        other.location,
                        other.width,
                        other.height,
                        other.internalType,
                        other.lnUn);
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
    VertexInputType type;
    MemoryLocation location;
    uint32_t offset;
    GLuint binding = 0;
};

struct IndexArrayInfo {
    uint32_t offset = 0;
    MemoryLocation location = MemoryLocation::Local;
    GLuint glType = 0;
    GcmDrawIndexArrayType type;
};

struct SurfaceToFramebufferLink {
    uint32_t surfaceEa;
    uint32_t framebufferEa;
    inline bool operator<(SurfaceToFramebufferLink const& other) const {
        return surfaceEa < other.surfaceEa;
    }
};

struct FragmentOps {
    bool blend;
    GcmBlendFunc sfcolor;
    GcmBlendFunc sfalpha;
    GcmBlendFunc dfcolor;
    GcmBlendFunc dfalpha;
    GcmBlendEquation blendColor;
    GcmBlendEquation blendAlpha;
    bool logic;
    GcmLogicOp logicOp;
    GcmOperator alphaFunc;
    bool depthTest;
    GcmOperator depthFunc;
    GcmOperator stencilFunc;
    uint32_t clearColor;
    GcmClearMask clearMask;
};

struct FragmentShaderControl {
    bool depthReplace = false;
    bool outputFromH0 = false;
    bool pixelKill = false;
    uint8_t registerCount = 0;
};

struct PointSpriteControl {
    bool enabled = false;
    uint16_t rmode;
    PointSpriteTex tex = PointSpriteTex();
};

enum BufferIndexes {
    vertexConstBuffer,
    vertexSamplersBuffer,
    vertexViewportBuffer,
    fragmentSamplersBuffer,
    fragmentConstBuffer
};

class GLFramebuffer;
class TextureRenderer;
struct RsxContext {
    Tracer tracer;
    uint32_t feedbackCount = 0;
    uint32_t feedbackMode;
    uint32_t frame = 0;
    uint32_t commandNum = 0;
    SurfaceInfo surface;
    ViewPortInfo viewPort;
    GcmColorMask colorMask;
    bool isCullFaceEnabled;
    GcmFrontFace cullFaceDirection;
    bool isFlatShadeMode;
    std::array<VertexDataArrayFormatInfo, 16> vertexDataArrays;
    GLuint glVertexArrayMode;
    GcmPrimitive vertexArrayMode;
    VertexShader* vertexShader = nullptr;
    FragmentShader* fragmentShader = nullptr;
    std::unique_ptr<RingBuffer> drawRingBuffer;
    GLPersistentCpuBuffer elementArrayIndexBuffer;
    bool vertexShaderDirty = false;
    bool fragmentShaderDirty = false;
    uint32_t fragmentBytecodeOffset = 0;
    uint32_t fragmentBytecodeLocation = 0;
    std::vector<uint8_t> fragmentBytecode;
    uint32_t fragmentConstCount = 0;
    std::vector<uint8_t> lastFrame;
    std::array<VertexShaderInputFormat, 16> vertexInputs;
    std::array<uint8_t, 512 * 16> vertexInstructions;
    uint32_t vertexLoadOffset = 0;
    IndexArrayInfo indexArray;
    std::array<TextureSamplerInfo, 4> vertexTextureSamplers;
    std::array<TextureSamplerInfo, 16> fragmentTextureSamplers;
    std::array<DisplayBufferInfo, 8> displayBuffers;
    std::unique_ptr<GLFramebuffer> framebuffer;
    std::unique_ptr<TextureRenderer> textureRenderer;
    uint32_t semaphoreOffset = 0;
    GLProgramPipeline pipeline;
    GLPersistentCpuBuffer mainMemoryBuffer;
    GLPersistentCpuBuffer localMemoryBuffer;
    GLPersistentCpuBuffer feedbackBuffer;
    Cache<TextureCacheKey, GLTexture, (256u << 20)> textureCache;
    FragmentShaderCache fragmentShaderCache;
    VertexShaderCache vertexShaderCache;
    MemoryLocation reportLocation;
    uint32_t surfaceClipWidth = 0;
    uint32_t surfaceClipHeight = 0;
    uint16_t frequencyDividerOperation = 0;
    FragmentOps fragmentOps;
    GcmCullFace cullFace;
    FragmentShaderControl fragmentShaderControl;
    bool pointSizeVertexOutputEnabled = false;
    float pointSize = 0;
    PointSpriteControl pointSpriteControl;
    InputMask vertexAttribInputMask = {};
    uint32_t flipBuffer = 0;
    GLVARs glVARs;

    ScaleSettings scale2d;
    SwizzleSettings swizzle2d;
    SurfaceSettings surface2d;
    CopySettings copy2d;
    InlineSettings inline2d;

    std::set<SurfaceToFramebufferLink> surfaceLinks;
    TextRenderer statText;

    inline void trace(CommandId id, std::vector<GcmCommandArg> const& args) {
        tracer.trace(frame, commandNum++, id, args);
    }

    RsxContext();
};
