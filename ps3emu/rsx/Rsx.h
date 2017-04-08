#pragma once

#include "ps3emu/libs/graphics/graphics.h"
#include "ps3emu/constants.h"
#include "ps3emu/libs/graphics/gcm.h"
#include "ps3emu/libs/ConcurrentQueue.h"
#include "ps3emu/gcmviz/GcmDatabase.h"
#include "ps3emu/BitField.h"
#include "GLFramebuffer.h"
#include "RsxTextureReader.h"
#include "ps3emu/enum.h"
#include <boost/thread.hpp>
#include <boost/endian/arithmetic.hpp>
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

union MethodHeader {
    uint32_t val;
    BitField<0, 3> prefix;
    BitField<3, 14> count;
    BitField<14, 16> suffix;
    BitField<16, 32> offset;
    BitField<3, 32> jumpoffset;
    BitField<0, 30> calloffset;
    BitField<30, 32> callsuffix;
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

ENUMF(GcmClearMask,
    (Z, 1<<0),
    (S, 1<<1),
    (R, 1<<4),
    (G, 1<<5),
    (B, 1<<6),
    (A, 1<<7),
    (M, 0xf3)
)

constexpr uint32_t EmuFlipCommandMethod = 0xacac;
constexpr auto FragmentProgramSize = 512 * 16;

enum class RsxOperationMode {
    Run, RunCapture, Replay
};

struct GcmCommandReplayInfo {
    GcmCommand command;
    bool notifyCompletion;
    std::function<void()> action;
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
class Rsx {
    static RsxOperationMode _mode;
    uint32_t _get = 0;
    uint32_t _put = 0;
    uint32_t _ref = 0;
    uint32_t _ret = 0;
    std::atomic<bool> _isFlipInProgress = false;
    std::atomic<uint32_t> _lastFlipTime = 0;
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
    
    void watchTextureCache();
    void watchShaderCache();
    void resetCacheWatch();
    void invalidateCaches(uint32_t va, uint32_t size);
    void waitForIdle();
    void loop();
    void runLoop();
    void replayLoop();
    void setSurfaceColorLocation(unsigned index, uint32_t location);
    void initGcm();
    void shutdownGcm();
    void EmuFlip(uint32_t buffer, uint32_t label, uint32_t labelValue);
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
    void ShaderControl(uint32_t control, uint8_t registerCount);
    void TransformProgramLoad(uint32_t load, uint32_t start);
    void TransformProgram(uint32_t locationOffset, unsigned size);
    void VertexAttribInputMask(uint16_t mask);
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
    void DepthTestEnable(bool enable);
    void ShadeMode(uint32_t sm);
    void ColorClearValue(uint32_t color);
    void ClearSurface(GcmClearMask mask);
    void VertexDataArrayFormat(uint8_t index,
                               uint16_t frequency,
                               uint8_t stride,
                               uint8_t size,
                               uint8_t type);
    void VertexDataArrayOffset(unsigned index, uint8_t location, uint32_t offset);
    void BeginEnd(GcmPrimitive mode);
    void DrawArrays(unsigned first, unsigned count);
    void InlineArray(uint32_t offset, unsigned count);
    void TransformConstantLoad(uint32_t loadAt, uint32_t offset, uint32_t count);
    void RestartIndexEnable(bool enable);
    void RestartIndex(uint32_t index);
    void IndexArrayAddress(uint8_t location, uint32_t offset, uint32_t type);
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
    
    void invokeHandler(uint32_t descrEa);
    
    // Replay-specific
    void UpdateBufferCache(MemoryLocation location, uint32_t offset, uint32_t size);
    inline void StopReplay() { }
    void transferImage();
    void startCopy2d();
    
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
    void setRef(uint32_t ref);
    bool isCallActive();
    void setLabel(int index, uint32_t value, bool waitForIdle = true);
    bool isFlipInProgress() const;
    void setFlipStatus();
    void resetFlipStatus();
    void setGcmContext(uint32_t ioSize, ps3_uintptr_t ioAddress);
    void setDisplayBuffer(uint8_t id,
                          uint32_t offset,
                          uint32_t pitch,
                          uint32_t width,
                          uint32_t height);
    void init();
    void encodeJump(ps3_uintptr_t va, uint32_t destOffset);
    void setVBlankHandler(uint32_t descrEa);
    void setFlipHandler(uint32_t descrEa);
    void updateOffsetTableForReplay();
    void sendCommand(GcmCommandReplayInfo info);
    bool receiveCommandCompletion();
    RsxContext* context();
    GLPersistentCpuBuffer* getBuffer(MemoryLocation location);
    uint32_t getLastFlipTime();
    void captureFrames();
    void resetContext();
    int64_t interpret(uint32_t get, std::function<uint32_t(uint32_t)> read);
};

MemoryLocation gcmEnumToLocation(uint32_t enumValue);
uint32_t getReportDataAddressLocation(uint32_t index, MemoryLocation location);
bool isMrt(SurfaceInfo const& surface);
std::array<int, 16> getFragmentSamplerSizes(const RsxContext* context);
