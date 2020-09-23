#pragma once

#include "ps3emu/libs/graphics/graphics.h"
#include "ps3emu/constants.h"
#include "ps3emu/libs/graphics/gcm.h"
#include "ps3emu/libs/ConcurrentQueue.h"
#include "ps3emu/gcmviz/GcmDatabase.h"
#include "ps3emu/BitField.h"
#include "ps3emu/utils/SpinLock.h"
#include "ps3emu/shaders/ShaderGenerator.h"
#include "ps3emu/TimedCounter.h"
#include "ps3emu/profiler.h"
#include "ps3emu/libs/sync/queue.h"
#include "GLQuery.h"
#include "GLFramebuffer.h"
#include "RsxTextureReader.h"
#include "ps3emu/enum.h"
#include <boost/thread.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/chrono.hpp>
#include <memory>
#include <map>
#include <atomic>

namespace emu {

typedef struct {
    boost::endian::big_uint32_t put;
    boost::endian::big_uint32_t get;
    boost::endian::big_uint32_t ref;
} CellGcmControl;

}

struct MethodHeader {
    uint32_t value;
    BIT_FIELD(prefix, 0, 3)
    BIT_FIELD(count, 3, 14)
    BIT_FIELD(suffix, 14, 16)
    BIT_FIELD(offset, 16, 32)
    BIT_FIELD(jumpoffset, 3, 32)
    BIT_FIELD(calloffset, 0, 30)
    BIT_FIELD(callsuffix, 30, 32)
};

ENUM(GcmBlendEquation,
    (FUNC_ADD, 0x8006),
    (MIN, 0x8007),
    (MAX, 0x8008),
    (FUNC_SUBTRACT, 0x800A),
    (FUNC_REVERSE_SUBTRACT, 0x800B),
    (FUNC_REVERSE_SUBTRACT_SIGNED, 0x0000F005),
    (FUNC_ADD_SIGNED, 0x0000F006),
    (FUNC_REVERSE_ADD_SIGNED, 0x0000F007)
)

ENUM(GcmBlendFunc,
    (ZERO, 0),
    (ONE, 1),
    (SRC_COLOR, 0x0300),
    (ONE_MINUS_SRC_COLOR, 0x0301),
    (SRC_ALPHA, 0x0302),
    (ONE_MINUS_SRC_ALPHA, 0x0303),
    (DST_ALPHA, 0x0304),
    (ONE_MINUS_DST_ALPHA, 0x0305),
    (DST_COLOR, 0x0306),
    (ONE_MINUS_DST_COLOR, 0x0307),
    (SRC_ALPHA_SATURATE, 0x0308),
    (CONSTANT_COLOR, 0x8001),
    (ONE_MINUS_CONSTANT_COLOR, 0x8002),
    (CONSTANT_ALPHA, 0x8003),
    (ONE_MINUS_CONSTANT_ALPHA, 0x8004)
)

ENUM(GcmLogicOp,
    (CLEAR, 0x1500),
    (AND, 0x1501),
    (AND_REVERSE, 0x1502),
    (COPY, 0x1503),
    (AND_INVERTED, 0x1504),
    (NOOP, 0x1505),
    (XOR, 0x1506),
    (OR, 0x1507),
    (NOR, 0x1508),
    (EQUIV, 0x1509),
    (INVERT, 0x150A),
    (OR_REVERSE, 0x150B),
    (COPY_INVERTED, 0x150C),
    (OR_INVERTED, 0x150D),
    (NAND, 0x150E),
    (SET, 0x150F)
)

ENUMF(GcmColorMask,
    (B, 1 << 0),
    (G, 1 << 8),
    (R, 1 << 16),
    (A, 1 << 24)
)

ENUM(GcmCullFace,
    (FRONT, 0x0404),
    (BACK, 0x0405),
    (FRONT_AND_BACK, 0x0408)
)

ENUM(GcmFrontFace,
    (CW, 0x0900),
    (CCW, 0x0901)
)

ENUM(GcmOperator,
    (NEVER, 0x0200),
    (LESS, 0x0201),
    (EQUAL, 0x0202),
    (LEQUAL, 0x0203),
    (GREATER, 0x0204),
    (NOTEQUAL, 0x0205),
    (GEQUAL, 0x0206),
    (ALWAYS, 0x0207)
)

ENUM(GcmPrimitive,
    (NONE, 0),
    (POINTS, 1),
    (LINES, 2),
    (LINE_LOOP, 3),
    (LINE_STRIP, 4),
    (TRIANGLES, 5),
    (TRIANGLE_STRIP, 6),
    (TRIANGLE_FAN, 7),
    (QUADS, 8),
    (QUAD_STRIP, 9),
    (POLYGON, 10)
)

ENUM(GcmDrawIndexArrayType,
    (_32, 0),
    (_16, 1)
)

ENUMF(GcmClearMask,
    (Z, 1<<0),
    (S, 1<<1),
    (R, 1<<4),
    (G, 1<<5),
    (B, 1<<6),
    (A, 1<<7),
    (M, 0xf3)
)

ENUMF(PointSpriteTex,
    (Tex0, (1<<8)),
    (Tex1, (1<<9)),
    (Tex2, (1<<10)),
    (Tex3, (1<<11)),
    (Tex4, (1<<12)),
    (Tex5, (1<<13)),
    (Tex6, (1<<14)),
    (Tex7, (1<<15)),
    (Tex8, (1<<16)),
    (Tex9, (1<<17))
)

ENUMF(InputMask,
    (VDA15, 1 << 0),
    (VDA14, 1 << 1),
    (VDA13, 1 << 2),
    (VDA12, 1 << 3),
    (VDA11, 1 << 4),
    (VDA10, 1 << 5),
    (VDA9, 1 << 6),
    (VDA8, 1 << 7),
    (VDA7, 1 << 8),
    (VDA6, 1 << 9),
    (VDA5, 1 << 10),
    (VDA4, 1 << 11),
    (VDA3, 1 << 12),
    (VDA2, 1 << 13),
    (VDA1, 1 << 14),
    (VDA0, 1 << 15)
)

struct __attribute__ ((__packed__)) VertexShaderSamplerUniform {
    std::array<uint32_t, 4> wraps[4];
    std::array<float, 4> borderColor[4];
    std::array<float, 4> disabledInputValues[16];
    std::array<uint32_t, 4> enabledInputs[16];
    std::array<uint32_t, 4> inputBufferBases[16];
    std::array<uint32_t, 4> inputBufferStrides[16];
    std::array<uint32_t, 4> inputBufferOps[16];
    std::array<uint32_t, 4> inputBufferFrequencies[16];
    std::array<uint64_t, 2> inputBuffers[16];
};

struct __attribute__ ((__packed__)) FragmentShaderSamplerUniform {
    uint32_t flip[16];
    float xOffset[16];
    float yOffset[16];
    float xScale[16];
    float yScale[16];
    uint32_t pointSpriteControl[16];
    uint32_t outputFromH;
    uint32_t reserved0;
    uint32_t reserved1;
    uint32_t reserved2;
};

constexpr auto FragmentProgramSize = 512 * 16;

enum class RsxOperationMode {
    Run, RunCapture, Replay
};

struct GcmCommandReplayInfo {
    GcmCommand command;
    bool notifyCompletion;
    std::function<void(boost::chrono::nanoseconds)> action;
};

class Rsx;
struct MethodMapEntry {
    void (Rsx::*handler)(int index);
    const char* name;
    int index;
    __itt_string_handle* task;
};

struct RsxContext;
class MainMemory;
class Process;
struct GcmCommandArg;
struct GcmCommand;
class GLTexture;
class GLBuffer;
class FragmentShader;
struct RsxTextureInfo;
class GLPersistentCpuBuffer;
class FpsLimiter;
class Rsx {
    static RsxOperationMode _mode;
    uint32_t* _get = nullptr;
    uint32_t* _put = nullptr;
    uint32_t* _ref = nullptr;
    std::atomic<uint32_t> _ret = 0;
    big_uint32_t* _isFlipInProgress = nullptr;
    big_uint64_t* _lastFlipTime = nullptr;
    bool _shutdown = false;
    bool _initialized = false;
    mutable boost::mutex _mutex;
    boost::condition_variable _cv;
    mutable boost::mutex _initMutex;
    boost::condition_variable _initCv;
    std::unique_ptr<boost::thread> _thread;
    std::unique_ptr<RsxContext> _context;
    std::map<uint32_t, uint32_t> _semaphores;
    uint32_t _activeSemaphoreHandle = 0;
    uint32_t _gcmIoSize;
    ps3_uintptr_t _gcmIoAddress;
    Window _window;
    ConcurrentFifoQueue<GcmCommandReplayInfo> _replayQueue;
    ConcurrentFifoQueue<bool> _completionQueue;
    std::vector<uint8_t> _currentReplayBlob;
    bool _frameCapturePending = false;
    bool _shortTrace = false;
    RsxTextureReader* _textureReader;
    std::map<MethodMapEntry*, TimedCounter> _perfMap;
    GLQuery _transformFeedbackQuery;
    TimedCounter _fpsCounter;
    TimedCounter _idleCounter;
    TimedCounter _textureCounter;
    TimedCounter _vertexShaderCounter;
    TimedCounter _fragmentShaderCounter;
    TimedCounter _vertexShaderRetrieveCounter;
    TimedCounter _fragmentShaderRetrieveCounter;
    TimedCounter _textureCacheCounter;
    TimedCounter _resetCacheCounter;
    TimedCounter _linkShaderCounter;
    TimedCounter _vdaCounter;
    TimedCounter _waitingForIdleCounter;
    TimedCounter _indexArrayProcessingCounter;
    TimedCounter _loadTextureCounter;
    TimedCounter _callbackCounter;
    TimedCounter _semaphoreAcquireCounter;
    std::vector<MethodMapEntry> _methodMap;
    __itt_string_handle* _loopProfilerTask;
    __itt_domain* _profilerDomain;
    sys_event_queue_t _callbackQueue;
    sys_event_port_t _callbackQueuePort;
    std::unique_ptr<FpsLimiter> _fpsLimiter;

    // loop
    const uint32_t* _currentGet = nullptr;
    uint32_t _currentGetValue = 0;
    int _currentCount = 0;
    bool _drawActive = false;
    uint32_t _drawArrayFirst = -1;
    uint32_t _drawIndexFirst = -1;
    uint32_t _drawCount = 0;

    void watchTextureCache();
    void resetCacheWatch();
    void invalidateCaches(uint32_t va, uint32_t size);
    void waitForIdle();
    void loop();
    void runLoop();
    void replayLoop();
    void setSurfaceColorLocation(unsigned index, uint32_t location);
    void initGcm();
    void shutdownGcm();
    void DriverQueue(uint32_t id);
    void DriverFlip(uint32_t value);
    bool linkShaderProgram();
    void updateVertexDataArrays(unsigned first, unsigned count);
    void updateShaders();
    void updateTextures();
    void updateViewPort();
    GLTexture* getTextureFromCache(uint32_t samplerId, bool isFragment);
    GLTexture* addTextureToCache(uint32_t samplerId, bool isFragment);
    GLBuffer* getBufferFromCache(uint32_t va, uint32_t size, bool wordReversed);
    void updateScissor();

    void ChannelSetContextDmaSemaphore(uint32_t handle);
    void ChannelSemaphoreOffset(uint32_t offset);
    void ChannelSemaphoreAcquire(uint32_t value);
    void TextureReadSemaphoreRelease(uint32_t value);
    void SemaphoreRelease(uint32_t value);
    void SurfaceClipHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h);
    void SurfacePitchC(uint32_t pitchC, uint32_t pitchD, uint32_t offsetC, uint32_t offsetD);
    void SurfaceCompression(uint32_t x);
    void WindowOffset(uint16_t x, uint16_t y);
    void ClearRectHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h);
    void ClipIdTestEnable(uint32_t x);
    void Control0(uint32_t x);
    void FlatShadeOp(uint32_t x);
    void VertexAttribOutputMask(uint32_t mask);
    void FrequencyDividerOperation(uint16_t op);
    void TexCoordControl(unsigned index, uint32_t control);
    void ShaderWindow(uint16_t height, uint8_t origin, uint16_t pixelCenters);
    void ReduceDstColor(bool enable);
    void FogMode(uint32_t mode);
    void AnisoSpread(unsigned index,
                     bool reduceSamplesEnable,
                     bool hReduceSamplesEnable,
                     bool vReduceSamplesEnable,
                     uint8_t spacingSelect,
                     uint8_t hSpacingSelect,
                     uint8_t vSpacingSelect);
    void VertexDataBaseOffset(uint32_t baseOffset, uint32_t baseIndex);
    void AlphaFunc(GcmOperator af, uint32_t ref);
    void AlphaTestEnable(bool enable);
    void ShaderControl(bool depthReplace, bool outputFromH0, bool pixelKill, uint8_t registerCount);
    void TransformProgramLoad(uint32_t load, uint32_t start);
    void TransformProgram(uint32_t locationOffset, unsigned size);
    void VertexAttribInputMask(InputMask mask);
    void TransformTimeout(uint16_t count, uint16_t registerCount);
    void ShaderProgram(uint32_t offset, uint32_t location);
    void ViewportHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h);
    void ClipMin(float min, float max);
    void ViewportOffset(float offset0,
                        float offset1,
                        float offset2,
                        float offset3,
                        float scale0,
                        float scale1,
                        float scale2,
                        float scale3);
    void ContextDmaColorA(uint32_t context);
    void ContextDmaColorB(uint32_t context);
    void ContextDmaColorC_2(uint32_t contextC, uint32_t contextD);
    void ContextDmaColorC_1(uint32_t contextC);
    void ContextDmaColorD(uint32_t context);
    void ContextDmaZeta(uint32_t context);
    void SurfaceFormat(GcmSurfaceColor colorFormat,
                       SurfaceDepthFormat depthFormat,
                       uint8_t antialias,
                       uint8_t type,
                       uint8_t width,
                       uint8_t height,
                       uint32_t pitchA,
                       uint32_t offsetA,
                       uint32_t offsetZ,
                       uint32_t offsetB,
                       uint32_t pitchB);
    void SurfacePitchZ(uint32_t pitch);
    void SurfaceColorTarget(uint32_t target);
    void ColorMask(GcmColorMask mask);
    void DepthFunc(GcmOperator zf);
    void CullFaceEnable(bool enable);
    void FrontFace(GcmFrontFace dir);
    void DepthTestEnable(bool enable);
    void ShadeMode(uint32_t sm);
    void ColorClearValue(uint32_t color);
    void ClearSurface(GcmClearMask mask);
    void VertexDataArrayFormat(uint8_t index,
                               uint16_t frequency,
                               uint8_t stride,
                               uint8_t size,
                               VertexInputType type);
    void VertexDataArrayOffset(unsigned index, uint8_t location, uint32_t offset);
    void BeginEnd(GcmPrimitive mode);
    void DrawArrays(unsigned first, unsigned count);
    void InlineArray(uint32_t offset, unsigned count);
    void TransformConstantLoad(uint32_t loadAt, uint32_t offset, uint32_t count);
    void RestartIndexEnable(bool enable);
    void RestartIndex(uint32_t index);
    void IndexArrayAddress(uint8_t location, uint32_t offset, GcmDrawIndexArrayType type);
    void IndexArrayAddress1(uint32_t offset);
    void IndexArrayDma(uint8_t location, GcmDrawIndexArrayType type);
    void DrawIndexArray(uint32_t first, uint32_t count);
    void VertexTextureOffset(unsigned index,
                             uint32_t offset,
                             uint8_t mipmap,
                             GcmTextureFormat format,
                             GcmTextureLnUn lnUn,
                             uint8_t dimension,
                             uint8_t location);
    void VertexTextureControl3(unsigned index, uint32_t pitch);
    void VertexTextureImageRect(unsigned index, uint16_t width, uint16_t height);
    void VertexTextureControl0(unsigned index, bool enable, float minlod, float maxlod);
    void VertexTextureAddress(unsigned index, uint8_t wraps, uint8_t wrapt);
    void VertexTextureFilter(unsigned index, float bias);
    void VertexTextureBorderColor(unsigned index, float a, float r, float g, float b);
    void TextureOffset(unsigned index,
                       uint32_t offset,
                       uint16_t mipmap,
                       GcmTextureFormat format,
                       GcmTextureLnUn lnUn,
                       uint8_t dimension,
                       bool border,
                       bool cubemap,
                       uint8_t location);
    void TextureImageRect(unsigned index, uint16_t width, uint16_t height);
    void TextureControl3(unsigned index, uint16_t depth, uint32_t pitch);
    void TextureControl2(unsigned index, uint8_t slope, bool iso, bool aniso);
    void TextureControl1(unsigned index, uint32_t remap);
    void TextureAddress(unsigned index,
                        uint8_t wraps,
                        uint8_t wrapt,
                        uint8_t wrapr,
                        uint8_t unsignedRemap,
                        uint8_t zfunc,
                        uint8_t gamma,
                        uint8_t anisoBias,
                        uint8_t signedRemap);
    void TextureBorderColor(unsigned index, float a, float r, float g, float b);
    void TextureControl0(unsigned index,
                         uint8_t alphaKill,
                         uint8_t maxaniso,
                         float maxlod,
                         float minlod,
                         bool enable);
    void TextureFilter(unsigned index,
                       float bias,
                       uint8_t min,
                       uint8_t mag,
                       uint8_t conv,
                       uint8_t as,
                       uint8_t rs,
                       uint8_t gs,
                       uint8_t bs);
    void SetReference(uint32_t ref);
    void SemaphoreOffset(uint32_t offset);
    void BackEndWriteSemaphoreRelease(uint32_t value);
    void OffsetDestin(uint32_t offset);
    void ColorFormat_3(uint32_t format, uint16_t pitch, uint32_t offset);
    void ColorFormat_2(uint32_t format, uint16_t pitch);
    void Point(uint16_t pointX,
               uint16_t pointY,
               uint16_t outSizeX,
               uint16_t outSizeY,
               uint16_t inSizeX,
               uint16_t inSizeY);
    void Color(uint32_t ptr, uint32_t count);
    void ContextDmaImageDestin(uint32_t location);
    void OffsetIn_1(uint32_t offset);
    void OffsetOut(uint32_t offset);
    void PitchIn(int32_t inPitch,
                 int32_t outPitch,
                 uint32_t lineLength,
                 uint32_t lineCount,
                 uint8_t inFormat,
                 uint8_t outFormat);
    void DmaBufferIn(uint32_t sourceLocation, uint32_t dstLocation);
    void OffsetIn_9(uint32_t inOffset,
                  uint32_t outOffset,
                  int32_t inPitch,
                  int32_t outPitch,
                  uint32_t lineLength,
                  uint32_t lineCount,
                  uint8_t inFormat,
                  uint8_t outFormat,
                  uint32_t notify);
    void Nv309eSetFormat(uint16_t format,
                         uint8_t width,
                         uint8_t height,
                         uint32_t offset);
    void BufferNotify(uint32_t notify);
    void Nv3089ContextDmaImage(uint32_t location);
    void Nv3089ContextSurface(uint32_t surfaceType);
    void Nv3089SetColorConversion(uint32_t conv,
                                  uint32_t fmt,
                                  uint32_t op,
                                  int16_t x,
                                  int16_t y,
                                  uint16_t w,
                                  uint16_t h,
                                  int16_t outX,
                                  int16_t outY,
                                  uint16_t outW,
                                  uint16_t outH,
                                  float dsdx,
                                  float dtdy);
    void ContextDmaImage(uint32_t location);
    void ImageInSize(uint16_t inW,
                     uint16_t inH,
                     uint16_t pitch,
                     uint8_t origin,
                     uint8_t interpolator,
                     uint32_t offset,
                     float inX,
                     float inY);
    void BlendEnable(bool enable);
    void BlendFuncSFactor(GcmBlendFunc sfcolor,
                          GcmBlendFunc sfalpha,
                          GcmBlendFunc dfcolor,
                          GcmBlendFunc dfalpha);
    void LogicOpEnable(bool enable);
    void BlendEquation(GcmBlendEquation color, GcmBlendEquation alpha);
    void ZStencilClearValue(uint32_t value);
    void VertexData4fM(unsigned index, float x, float y, float z, float w);
    void CullFace(GcmCullFace cfm);
    void FrontPolygonMode(uint32_t mode);
    void BackPolygonMode(uint32_t mode);
    void StencilTestEnable(bool enable);
    void StencilMask(uint32_t sm);
    void StencilFunc(GcmOperator func, int32_t ref, uint32_t mask);
    void StencilOpFail(uint32_t fail, uint32_t depthFail, uint32_t depthPass);
    void ContextDmaReport(uint32_t handle);
    void GetReport(uint8_t type, uint32_t offset);
    void ScissorHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h);
    void TransformProgramStart(uint32_t startSlot);
    void LineWidth(float width);
    void LineSmoothEnable(bool enable);
    void LogicOp(GcmLogicOp op);
    void PolySmoothEnable(bool enable);
    void PolyOffsetLineEnable(bool enable);
    void PolyOffsetFillEnable(bool enable);
    void DriverInterrupt(uint32_t cause);
    void PointSize(float size);
    void PointParamsEnable(bool enable);
    void PointSpriteControl(bool enable, uint16_t rmode, PointSpriteTex tex);

    // loop
    void initMethodMap();
    uint32_t readarg(int n);
    void unknown_impl(int index);
    void CELL_GCM_NV406E_SET_REFERENCE_impl(int index);
    void CELL_GCM_NV406E_SET_CONTEXT_DMA_SEMAPHORE_impl(int index);
    void CELL_GCM_NV406E_SEMAPHORE_OFFSET_impl(int index);
    void CELL_GCM_NV406E_SEMAPHORE_ACQUIRE_impl(int index);
    void CELL_GCM_NV406E_SEMAPHORE_RELEASE_impl(int index);
    void CELL_GCM_NV4097_SET_OBJECT_impl(int index);
    void CELL_GCM_NV4097_NO_OPERATION_impl(int index);
    void CELL_GCM_NV4097_NOTIFY_impl(int index);
    void CELL_GCM_NV4097_WAIT_FOR_IDLE_impl(int index);
    void CELL_GCM_NV4097_PM_TRIGGER_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_NOTIFIES_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_A_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_B_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_B_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_STATE_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_A_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_ZETA_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_A_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_B_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_SEMAPHORE_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_REPORT_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_CLIP_ID_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_CULL_DATA_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_C_impl(int index);
    void CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_D_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_CLIP_HORIZONTAL_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_CLIP_VERTICAL_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_FORMAT_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_PITCH_A_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_COLOR_AOFFSET_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_ZETA_OFFSET_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_COLOR_BOFFSET_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_PITCH_B_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_COLOR_TARGET_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_PITCH_Z_impl(int index);
    void CELL_GCM_NV4097_INVALIDATE_ZCULL_impl(int index);
    void CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP_impl(int index);
    void CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP1_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_PITCH_C_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_PITCH_D_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_COLOR_COFFSET_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_COLOR_DOFFSET_impl(int index);
    void CELL_GCM_NV4097_SET_WINDOW_OFFSET_impl(int index);
    void CELL_GCM_NV4097_SET_WINDOW_CLIP_TYPE_impl(int index);
    void CELL_GCM_NV4097_SET_WINDOW_CLIP_HORIZONTAL_impl(int index);
    void CELL_GCM_NV4097_SET_WINDOW_CLIP_VERTICAL_impl(int index);
    void CELL_GCM_NV4097_SET_DITHER_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_ALPHA_TEST_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_ALPHA_FUNC_impl(int index);
    void CELL_GCM_NV4097_SET_ALPHA_REF_impl(int index);
    void CELL_GCM_NV4097_SET_BLEND_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_BLEND_FUNC_SFACTOR_impl(int index);
    void CELL_GCM_NV4097_SET_BLEND_FUNC_DFACTOR_impl(int index);
    void CELL_GCM_NV4097_SET_BLEND_COLOR_impl(int index);
    void CELL_GCM_NV4097_SET_BLEND_EQUATION_impl(int index);
    void CELL_GCM_NV4097_SET_COLOR_MASK_impl(int index);
    void CELL_GCM_NV4097_SET_STENCIL_TEST_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_STENCIL_MASK_impl(int index);
    void CELL_GCM_NV4097_SET_STENCIL_FUNC_impl(int index);
    void CELL_GCM_NV4097_SET_STENCIL_FUNC_REF_impl(int index);
    void CELL_GCM_NV4097_SET_STENCIL_FUNC_MASK_impl(int index);
    void CELL_GCM_NV4097_SET_STENCIL_OP_FAIL_impl(int index);
    void CELL_GCM_NV4097_SET_STENCIL_OP_ZFAIL_impl(int index);
    void CELL_GCM_NV4097_SET_STENCIL_OP_ZPASS_impl(int index);
    void CELL_GCM_NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_BACK_STENCIL_MASK_impl(int index);
    void CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_impl(int index);
    void CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_REF_impl(int index);
    void CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_MASK_impl(int index);
    void CELL_GCM_NV4097_SET_BACK_STENCIL_OP_FAIL_impl(int index);
    void CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZFAIL_impl(int index);
    void CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZPASS_impl(int index);
    void CELL_GCM_NV4097_SET_SHADE_MODE_impl(int index);
    void CELL_GCM_NV4097_SET_BLEND_ENABLE_MRT_impl(int index);
    void CELL_GCM_NV4097_SET_COLOR_MASK_MRT_impl(int index);
    void CELL_GCM_NV4097_SET_LOGIC_OP_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_LOGIC_OP_impl(int index);
    void CELL_GCM_NV4097_SET_BLEND_COLOR2_impl(int index);
    void CELL_GCM_NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MIN_impl(int index);
    void CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MAX_impl(int index);
    void CELL_GCM_NV4097_SET_CLIP_MIN_impl(int index);
    void CELL_GCM_NV4097_SET_CLIP_MAX_impl(int index);
    void CELL_GCM_NV4097_SET_CONTROL0_impl(int index);
    void CELL_GCM_NV4097_SET_LINE_WIDTH_impl(int index);
    void CELL_GCM_NV4097_SET_LINE_SMOOTH_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_ANISO_SPREAD_impl(int index);
    void CELL_GCM_NV4097_SET_SCISSOR_HORIZONTAL_impl(int index);
    void CELL_GCM_NV4097_SET_SCISSOR_VERTICAL_impl(int index);
    void CELL_GCM_NV4097_SET_FOG_MODE_impl(int index);
    void CELL_GCM_NV4097_SET_FOG_PARAMS_impl(int index);
    void CELL_GCM_NV4097_SET_SHADER_PROGRAM_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_TEXTURE_OFFSET_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FORMAT_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_TEXTURE_ADDRESS_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL0_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL3_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FILTER_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR_impl(int index);
    void CELL_GCM_NV4097_SET_VIEWPORT_HORIZONTAL_impl(int index);
    void CELL_GCM_NV4097_SET_VIEWPORT_VERTICAL_impl(int index);
    void CELL_GCM_NV4097_SET_POINT_CENTER_MODE_impl(int index);
    void CELL_GCM_NV4097_ZCULL_SYNC_impl(int index);
    void CELL_GCM_NV4097_SET_VIEWPORT_OFFSET_impl(int index);
    void CELL_GCM_NV4097_SET_VIEWPORT_SCALE_impl(int index);
    void CELL_GCM_NV4097_SET_POLY_OFFSET_POINT_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_POLY_OFFSET_LINE_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_POLY_OFFSET_FILL_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_DEPTH_FUNC_impl(int index);
    void CELL_GCM_NV4097_SET_DEPTH_MASK_impl(int index);
    void CELL_GCM_NV4097_SET_DEPTH_TEST_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR_impl(int index);
    void CELL_GCM_NV4097_SET_POLYGON_OFFSET_BIAS_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA_SCALED4S_M_impl(int index);
    void CELL_GCM_NV4097_SET_TEXTURE_CONTROL2_impl(int index);
    void CELL_GCM_NV4097_SET_TEX_COORD_CONTROL_impl(int index);
    void CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_impl(int index);
    void CELL_GCM_NV4097_SET_SPECULAR_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_TWO_SIDE_LIGHT_EN_impl(int index);
    void CELL_GCM_NV4097_CLEAR_ZCULL_SURFACE_impl(int index);
    void CELL_GCM_NV4097_SET_PERFORMANCE_PARAMS_impl(int index);
    void CELL_GCM_NV4097_SET_FLAT_SHADE_OP_impl(int index);
    void CELL_GCM_NV4097_SET_EDGE_FLAG_impl(int index);
    void CELL_GCM_NV4097_SET_USER_CLIP_PLANE_CONTROL_impl(int index);
    void CELL_GCM_NV4097_SET_POLYGON_STIPPLE_impl(int index);
    void CELL_GCM_NV4097_SET_POLYGON_STIPPLE_PATTERN_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA3F_M_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_OFFSET_impl(int index);
    void CELL_GCM_NV4097_INVALIDATE_VERTEX_CACHE_FILE_impl(int index);
    void CELL_GCM_NV4097_INVALIDATE_VERTEX_FILE_impl(int index);
    void CELL_GCM_NV4097_PIPE_NOP_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_OFFSET_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_INDEX_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_FORMAT_impl(int index);
    void CELL_GCM_NV4097_CLEAR_REPORT_VALUE_impl(int index);
    void CELL_GCM_NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE_impl(int index);
    void CELL_GCM_NV4097_GET_REPORT_impl(int index);
    void CELL_GCM_NV4097_SET_ZCULL_STATS_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_BEGIN_END_impl(int index);
    void CELL_GCM_NV4097_ARRAY_ELEMENT16_impl(int index);
    void CELL_GCM_NV4097_ARRAY_ELEMENT32_impl(int index);
    void CELL_GCM_NV4097_DRAW_ARRAYS_impl(int index);
    void CELL_GCM_NV4097_INLINE_ARRAY_impl(int index);
    void CELL_GCM_NV4097_SET_INDEX_ARRAY_ADDRESS_impl(int index);
    void CELL_GCM_NV4097_SET_INDEX_ARRAY_DMA_impl(int index);
    void CELL_GCM_NV4097_DRAW_INDEX_ARRAY_impl(int index);
    void CELL_GCM_NV4097_SET_FRONT_POLYGON_MODE_impl(int index);
    void CELL_GCM_NV4097_SET_BACK_POLYGON_MODE_impl(int index);
    void CELL_GCM_NV4097_SET_CULL_FACE_impl(int index);
    void CELL_GCM_NV4097_SET_FRONT_FACE_impl(int index);
    void CELL_GCM_NV4097_SET_POLY_SMOOTH_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_CULL_FACE_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_TEXTURE_CONTROL3_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA2F_M_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA2S_M_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA4UB_M_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA4S_M_impl(int index);
    void CELL_GCM_NV4097_SET_TEXTURE_OFFSET_impl(int index);
    void CELL_GCM_NV4097_SET_TEXTURE_FORMAT_impl(int index);
    void CELL_GCM_NV4097_SET_TEXTURE_ADDRESS_impl(int index);
    void CELL_GCM_NV4097_SET_TEXTURE_CONTROL0_impl(int index);
    void CELL_GCM_NV4097_SET_TEXTURE_CONTROL1_impl(int index);
    void CELL_GCM_NV4097_SET_TEXTURE_FILTER_impl(int index);
    void CELL_GCM_NV4097_SET_TEXTURE_IMAGE_RECT_impl(int index);
    void CELL_GCM_NV4097_SET_TEXTURE_BORDER_COLOR_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA4F_M_impl(int index);
    void CELL_GCM_NV4097_SET_COLOR_KEY_COLOR_impl(int index);
    void CELL_GCM_NV4097_SET_SHADER_CONTROL_impl(int index);
    void CELL_GCM_NV4097_SET_INDEXED_CONSTANT_READ_LIMITS_impl(int index);
    void CELL_GCM_NV4097_SET_SEMAPHORE_OFFSET_impl(int index);
    void CELL_GCM_NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE_impl(int index);
    void CELL_GCM_NV4097_TEXTURE_READ_SEMAPHORE_RELEASE_impl(int index);
    void CELL_GCM_NV4097_SET_ZMIN_MAX_CONTROL_impl(int index);
    void CELL_GCM_NV4097_SET_ANTI_ALIASING_CONTROL_impl(int index);
    void CELL_GCM_NV4097_SET_SURFACE_COMPRESSION_impl(int index);
    void CELL_GCM_NV4097_SET_ZCULL_EN_impl(int index);
    void CELL_GCM_NV4097_SET_SHADER_WINDOW_impl(int index);
    void CELL_GCM_NV4097_SET_ZSTENCIL_CLEAR_VALUE_impl(int index);
    void CELL_GCM_NV4097_SET_COLOR_CLEAR_VALUE_impl(int index);
    void CELL_GCM_NV4097_CLEAR_SURFACE_impl(int index);
    void CELL_GCM_NV4097_SET_CLEAR_RECT_HORIZONTAL_impl(int index);
    void CELL_GCM_NV4097_SET_CLEAR_RECT_VERTICAL_impl(int index);
    void CELL_GCM_NV4097_SET_CLIP_ID_TEST_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_RESTART_INDEX_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_RESTART_INDEX_impl(int index);
    void CELL_GCM_NV4097_SET_LINE_STIPPLE_impl(int index);
    void CELL_GCM_NV4097_SET_LINE_STIPPLE_PATTERN_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_DATA1F_M_impl(int index);
    void CELL_GCM_NV4097_SET_TRANSFORM_EXECUTION_MODE_impl(int index);
    void CELL_GCM_NV4097_SET_RENDER_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_LOAD_impl(int index);
    void CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_START_impl(int index);
    void CELL_GCM_NV4097_SET_ZCULL_CONTROL0_impl(int index);
    void CELL_GCM_NV4097_SET_ZCULL_CONTROL1_impl(int index);
    void CELL_GCM_NV4097_SET_SCULL_CONTROL_impl(int index);
    void CELL_GCM_NV4097_SET_POINT_SIZE_impl(int index);
    void CELL_GCM_NV4097_SET_POINT_PARAMS_ENABLE_impl(int index);
    void CELL_GCM_NV4097_SET_POINT_SPRITE_CONTROL_impl(int index);
    void CELL_GCM_NV4097_SET_TRANSFORM_TIMEOUT_impl(int index);
    void CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD_impl(int index);
    void CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_impl(int index);
    void CELL_GCM_NV4097_SET_FREQUENCY_DIVIDER_OPERATION_impl(int index);
    void CELL_GCM_NV4097_SET_ATTRIB_COLOR_impl(int index);
    void CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD_impl(int index);
    void CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD_EX_impl(int index);
    void CELL_GCM_NV4097_SET_ATTRIB_UCLIP0_impl(int index);
    void CELL_GCM_NV4097_SET_ATTRIB_UCLIP1_impl(int index);
    void CELL_GCM_NV4097_INVALIDATE_L2_impl(int index);
    void CELL_GCM_NV4097_SET_REDUCE_DST_COLOR_impl(int index);
    void CELL_GCM_NV4097_SET_NO_PARANOID_TEXTURE_FETCHES_impl(int index);
    void CELL_GCM_NV4097_SET_SHADER_PACKER_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_ATTRIB_INPUT_MASK_impl(int index);
    void CELL_GCM_NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK_impl(int index);
    void CELL_GCM_NV4097_SET_TRANSFORM_BRANCH_BITS_impl(int index);
    void CELL_GCM_NV0039_SET_OBJECT_impl(int index);
    void CELL_GCM_NV0039_SET_CONTEXT_DMA_NOTIFIES_impl(int index);
    void CELL_GCM_NV0039_SET_CONTEXT_DMA_BUFFER_IN_impl(int index);
    void CELL_GCM_NV0039_SET_CONTEXT_DMA_BUFFER_OUT_impl(int index);
    void CELL_GCM_NV0039_OFFSET_IN_impl(int index);
    void CELL_GCM_NV0039_OFFSET_OUT_impl(int index);
    void CELL_GCM_NV0039_PITCH_IN_impl(int index);
    void CELL_GCM_NV0039_PITCH_OUT_impl(int index);
    void CELL_GCM_NV0039_LINE_LENGTH_IN_impl(int index);
    void CELL_GCM_NV0039_LINE_COUNT_impl(int index);
    void CELL_GCM_NV0039_FORMAT_impl(int index);
    void CELL_GCM_NV0039_BUFFER_NOTIFY_impl(int index);
    void CELL_GCM_NV3062_SET_OBJECT_impl(int index);
    void CELL_GCM_NV3062_SET_CONTEXT_DMA_NOTIFIES_impl(int index);
    void CELL_GCM_NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE_impl(int index);
    void CELL_GCM_NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN_impl(int index);
    void CELL_GCM_NV3062_SET_COLOR_FORMAT_impl(int index);
    void CELL_GCM_NV3062_SET_PITCH_impl(int index);
    void CELL_GCM_NV3062_SET_OFFSET_SOURCE_impl(int index);
    void CELL_GCM_NV3062_SET_OFFSET_DESTIN_impl(int index);
    void CELL_GCM_NV309E_SET_OBJECT_impl(int index);
    void CELL_GCM_NV309E_SET_CONTEXT_DMA_NOTIFIES_impl(int index);
    void CELL_GCM_NV309E_SET_CONTEXT_DMA_IMAGE_impl(int index);
    void CELL_GCM_NV309E_SET_FORMAT_impl(int index);
    void CELL_GCM_NV309E_SET_OFFSET_impl(int index);
    void CELL_GCM_NV308A_SET_OBJECT_impl(int index);
    void CELL_GCM_NV308A_SET_CONTEXT_DMA_NOTIFIES_impl(int index);
    void CELL_GCM_NV308A_SET_CONTEXT_COLOR_KEY_impl(int index);
    void CELL_GCM_NV308A_SET_CONTEXT_CLIP_RECTANGLE_impl(int index);
    void CELL_GCM_NV308A_SET_CONTEXT_PATTERN_impl(int index);
    void CELL_GCM_NV308A_SET_CONTEXT_ROP_impl(int index);
    void CELL_GCM_NV308A_SET_CONTEXT_BETA1_impl(int index);
    void CELL_GCM_NV308A_SET_CONTEXT_BETA4_impl(int index);
    void CELL_GCM_NV308A_SET_CONTEXT_SURFACE_impl(int index);
    void CELL_GCM_NV308A_SET_COLOR_CONVERSION_impl(int index);
    void CELL_GCM_NV308A_SET_OPERATION_impl(int index);
    void CELL_GCM_NV308A_SET_COLOR_FORMAT_impl(int index);
    void CELL_GCM_NV308A_POINT_impl(int index);
    void CELL_GCM_NV308A_SIZE_OUT_impl(int index);
    void CELL_GCM_NV308A_SIZE_IN_impl(int index);
    void CELL_GCM_NV308A_COLOR_impl(int index);
    void CELL_GCM_NV3089_SET_OBJECT_impl(int index);
    void CELL_GCM_NV3089_SET_CONTEXT_DMA_NOTIFIES_impl(int index);
    void CELL_GCM_NV3089_SET_CONTEXT_DMA_IMAGE_impl(int index);
    void CELL_GCM_NV3089_SET_CONTEXT_PATTERN_impl(int index);
    void CELL_GCM_NV3089_SET_CONTEXT_ROP_impl(int index);
    void CELL_GCM_NV3089_SET_CONTEXT_BETA1_impl(int index);
    void CELL_GCM_NV3089_SET_CONTEXT_BETA4_impl(int index);
    void CELL_GCM_NV3089_SET_CONTEXT_SURFACE_impl(int index);
    void CELL_GCM_NV3089_SET_COLOR_CONVERSION_impl(int index);
    void CELL_GCM_NV3089_SET_COLOR_FORMAT_impl(int index);
    void CELL_GCM_NV3089_SET_OPERATION_impl(int index);
    void CELL_GCM_NV3089_CLIP_POINT_impl(int index);
    void CELL_GCM_NV3089_CLIP_SIZE_impl(int index);
    void CELL_GCM_NV3089_IMAGE_OUT_POINT_impl(int index);
    void CELL_GCM_NV3089_IMAGE_OUT_SIZE_impl(int index);
    void CELL_GCM_NV3089_DS_DX_impl(int index);
    void CELL_GCM_NV3089_DT_DY_impl(int index);
    void CELL_GCM_NV3089_IMAGE_IN_SIZE_impl(int index);
    void CELL_GCM_NV3089_IMAGE_IN_FORMAT_impl(int index);
    void CELL_GCM_NV3089_IMAGE_IN_OFFSET_impl(int index);
    void CELL_GCM_NV3089_IMAGE_IN_impl(int index);
    void CELL_GCM_DRIVER_INTERRUPT_impl(int index);
    void CELL_GCM_DRIVER_QUEUE_impl(int index);
    void CELL_GCM_DRIVER_FLIP_impl(int index);
    void CELL_GCM_unknown_240_impl(int index);
    void CELL_GCM_unknown_e000_impl(int index);
    void parseTextureAddress(int argi, int index);
    void parseTextureControl0(int argi, int index);
    void parseTextureFilter(int argi, int index);
    void parseTextureBorderColor(int argi, int index);
    void parseTextureImageRect(int argi, int index);

    void invokeHandler(uint32_t num, uint32_t arg);
    void drawStats();
    void advanceBuffers();

    // Replay-specific
    void UpdateBufferCache(MemoryLocation location, uint32_t offset, uint32_t size);
    inline void StopReplay() { }
    void transferImage();
    void startCopy2d();
    void beginTransformFeedback();
    void endTransformFeedback();
    void updateDisplayBuffersForCapture();

public:
    Rsx();
    ~Rsx();
    static void setOperationMode(RsxOperationMode mode);
    void shutdown();
    void setPut(uint32_t put);
    void setGet(uint32_t get);
    uint32_t getPut();
    uint32_t getGet();
    uint32_t getRef();
    uint32_t getRet();
    void setRef(uint32_t ref);
    void setLabel(int index, uint32_t value, bool waitForIdle = true);
    bool isFlipInProgress() const;
    void setFlipStatus(bool inProgress);
    void setGcmContext(uint32_t ioSize, ps3_uintptr_t ioAddress);
    void setDisplayBuffer(uint8_t id, uint32_t offset, uint32_t height);
    void init(sys_event_queue_t callbackQueue);
    void encodeJump(ps3_uintptr_t va, uint32_t destOffset);
    void updateOffsetTableForReplay();
    void sendCommand(GcmCommandReplayInfo info);
    bool receiveCommandCompletion();
    RsxContext* context();
    GLPersistentCpuBuffer* getBuffer(MemoryLocation location);
    void captureFrames();
    void resetContext();
    int64_t interpret(uint32_t get, const uint32_t* read);
};

MemoryLocation gcmEnumToLocation(uint32_t enumValue);
uint32_t getReportDataAddressLocation(uint32_t index, MemoryLocation location);
bool isMrt(SurfaceInfo const& surface);
std::array<int, 16> getFragmentSamplerSizes(const RsxContext* context);
unsigned vertexDataArrayTypeSize(VertexInputType type);
