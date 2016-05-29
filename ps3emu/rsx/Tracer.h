#pragma once

#include "../utils.h"
#include "../gcmviz/GcmDatabase.h"
#include "../libs/graphics/gcm.h"
#include <vector>

#define X(x) x,
#define CommandIdX \
    X(SurfaceClipHorizontal) \
    X(SurfacePitchC) \
    X(SurfaceCompression) \
    X(WindowOffset) \
    X(ClearRectHorizontal) \
    X(ClipIdTestEnable) \
    X(Control0) \
    X(FlatShadeOp) \
    X(VertexAttribOutputMask) \
    X(FrequencyDividerOperation) \
    X(TexCoordControl) \
    X(ShaderWindow) \
    X(ReduceDstColor) \
    X(FogMode) \
    X(AnisoSpread) \
    X(VertexDataBaseOffset) \
    X(AlphaFunc) \
    X(AlphaTestEnable) \
    X(ShaderControl) \
    X(TransformProgramLoad) \
    X(TransformProgram) \
    X(VertexAttribInputMask) \
    X(TransformTimeout) \
    X(ShaderProgram) \
    X(ViewportHorizontal) \
    X(ClipMin) \
    X(ViewportOffset) \
    X(ContextDmaColorA) \
    X(ContextDmaColorB) \
    X(ContextDmaColorC_2) \
    X(ContextDmaColorC_1) \
    X(ContextDmaColorD) \
    X(ContextDmaZeta) \
    X(SurfaceFormat) \
    X(SurfacePitchZ) \
    X(SurfaceColorTarget) \
    X(ColorMask) \
    X(DepthFunc) \
    X(CullFaceEnable) \
    X(DepthTestEnable) \
    X(ShadeMode) \
    X(ColorClearValue) \
    X(ClearSurface) \
    X(VertexDataArrayFormat) \
    X(VertexDataArrayOffset) \
    X(BeginEnd) \
    X(DrawArrays) \
    X(InlineArray) \
    X(TransformConstantLoad) \
    X(RestartIndexEnable) \
    X(RestartIndex) \
    X(IndexArrayAddress) \
    X(DrawIndexArray) \
    X(VertexTextureOffset) \
    X(VertexTextureControl3) \
    X(VertexTextureImageRect) \
    X(VertexTextureControl0) \
    X(VertexTextureAddress) \
    X(VertexTextureFilter) \
    X(VertexTextureBorderColor) \
    X(TextureOffset) \
    X(TextureImageRect) \
    X(TextureControl3) \
    X(TextureControl2) \
    X(TextureControl1) \
    X(TextureAddress) \
    X(TextureBorderColor) \
    X(TextureControl0) \
    X(TextureFilter) \
    X(SetReference) \
    X(SemaphoreOffset) \
    X(BackEndWriteSemaphoreRelease) \
    X(OffsetDestin) \
    X(ColorFormat_2) \
    X(ColorFormat_3) \
    X(Point) \
    X(Color) \
    X(ContextDmaImageDestin) \
    X(OffsetIn_1) \
    X(OffsetOut) \
    X(PitchIn) \
    X(DmaBufferIn) \
    X(OffsetIn_9) \
    X(BufferNotify) \
    X(Nv3089ContextDmaImage) \
    X(Nv3089ContextSurface) \
    X(Nv3089SetColorConversion) \
    X(ImageInSize) \
    X(ContextDmaImage) \
    X(Nv309eSetFormat) \
    X(BlendEnable) \
    X(BlendFuncSFactor) \
    X(LogicOpEnable) \
    X(BlendEquation) \
    X(ZStencilClearValue) \
    X(VertexData4fM) \
    X(CullFace) \
    X(FrontPolygonMode) \
    X(StencilTestEnable) \
    X(StencilMask) \
    X(StencilFunc) \
    X(StencilOpFail) \
    X(ContextDmaReport) \
    X(GetReport) \
    X(setSurfaceColorLocation) \
    X(setDisplayBuffer) \
    X(waitForIdle) \
    X(addTextureToCache) \
    X(addFragmentShaderToCache) \
    X(updateOffsetTableForReplay) \
    X(EmuFlip) \
    X(StopReplay) \
    X(UpdateBufferCache) \
    X(UpdateTextureCache) \
    X(UpdateFragmentCache) \
    
enum class CommandId { CommandIdX };
#undef X

class Tracer {
    bool _enabled = false;
    GcmDatabase _db;
    std::vector<uint8_t> _blob;
    bool _blobSet = false;
public:
    void enable();
    void pushBlob(const void* ptr, uint32_t size);
    void trace(uint32_t frame,
               uint32_t num,
               CommandId command,
               std::vector<GcmCommandArg> args);
};

template<typename T> struct GcmType { static constexpr uint32_t type = (uint32_t)GcmArgType::None; };
template<> struct GcmType<uint8_t> { static constexpr uint32_t type = (uint32_t)GcmArgType::UInt8; };
template<> struct GcmType<uint16_t> { static constexpr uint32_t type = (uint32_t)GcmArgType::UInt16; };
template<> struct GcmType<uint32_t> { static constexpr uint32_t type = (uint32_t)GcmArgType::UInt32; };
template<> struct GcmType<float> { static constexpr uint32_t type = (uint32_t)GcmArgType::Float; };
template<> struct GcmType<bool> { static constexpr uint32_t type = (uint32_t)GcmArgType::Bool; };
template<> struct GcmType<int32_t> { static constexpr uint32_t type = (uint32_t)GcmArgType::Int32; };
template<> struct GcmType<int16_t> { static constexpr uint32_t type = (uint32_t)GcmArgType::Int16; };

template <typename T> struct ConvertType {
    static uint32_t convert(T value) {
        return value;
    }
};

template <> struct ConvertType<MemoryLocation> {
    static uint32_t convert(MemoryLocation value) {
        return (uint32_t)value;
    }
};

template <> struct ConvertType<float> {
    static uint32_t convert(float value) {
        return union_cast<float, uint32_t>(value);
    }
};

/*
def a(i):
     r = "#define TRACE" + str(i) + "(id"
     for n in range(0, i): r += ", a" + str(n)
     r += ") _context->trace(CommandId::id, { ARG(a0)"
     for n in range(1, i): r += ", ARG(a" + str(n) + ")"
     r += " });"
     print(r)
for z in range(1, 14): a(z) 
*/

#define ARG(a) GcmCommandArg { ConvertType<decltype(a)>::convert(a), \
                               #a, GcmType<decltype(a)>::type }
#define TRACE0(id) _context->trace(CommandId::id, {});
#define TRACE1(id, a0) _context->trace(CommandId::id, { ARG(a0) });
#define TRACE2(id, a0, a1) _context->trace(CommandId::id, { ARG(a0), ARG(a1) });
#define TRACE3(id, a0, a1, a2) _context->trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2) });
#define TRACE4(id, a0, a1, a2, a3) _context->trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3) });
#define TRACE5(id, a0, a1, a2, a3, a4) _context->trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4) });
#define TRACE6(id, a0, a1, a2, a3, a4, a5) _context->trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5) });
#define TRACE7(id, a0, a1, a2, a3, a4, a5, a6) _context->trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6) });
#define TRACE8(id, a0, a1, a2, a3, a4, a5, a6, a7) _context->trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7) });
#define TRACE9(id, a0, a1, a2, a3, a4, a5, a6, a7, a8) _context->trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7), ARG(a8) });
#define TRACE10(id, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) _context->trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7), ARG(a8), ARG(a9) });
#define TRACE11(id, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) _context->trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7), ARG(a8), ARG(a9), ARG(a10) });
#define TRACE12(id, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) _context->trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7), ARG(a8), ARG(a9), ARG(a10), ARG(a11) });
#define TRACE13(id, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) _context->trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7), ARG(a8), ARG(a9), ARG(a10), ARG(a11), ARG(a12) });

const char* printCommandId(CommandId id);
std::string printArgHex(GcmCommandArg const& arg);
std::string printArgDecimal(GcmCommandArg const& arg);