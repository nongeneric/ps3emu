#pragma once

#include "../../libs/graphics/graphics.h"
#include "../constants.h"
#include "../../libs/graphics/gcm.h"
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

constexpr uint32_t EmuFlipCommandMethod = 0xacac;

class RsxContext;
class PPU;
class Rsx {
    uint32_t _get = 0;
    uint32_t _put = 0;
    uint32_t _ref = 0xffffffff;
    std::atomic<uint32_t> _ret;
    bool _shutdown = false;
    bool _initialized = false;
    PPU* _ppu;
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
    int64_t interpret(uint32_t get);
    Window window;
    void memoryBreakHandler(uint32_t va, uint32_t size);
    void waitForIdle();
    void loop();
    void setSurfaceColorLocation(unsigned index, uint32_t location);
    void initGcm();
    void EmuFlip(uint32_t buffer, uint32_t label, uint32_t labelValue);
    bool linkShaderProgram();
    void updateVertexDataArrays(unsigned first, unsigned count);
    void updateShaders();
    void updateTextures();
    void updateFramebuffer();
    void ChannelSetContextDmaSemaphore(uint32_t handle);
    void ChannelSemaphoreOffset(uint32_t offset);
    void ChannelSemaphoreAcquire(uint32_t value);
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
    void AlphaFunc(uint32_t af, uint32_t ref);
    void AlphaTestEnable(bool enable);
    void ShaderControl(uint32_t control, uint8_t registerCount);
    void TransformProgramLoad(uint32_t load, uint32_t start);
    void TransformProgram(uint32_t locationOffset, unsigned size);
    void VertexAttribInputMask(uint32_t mask);
    void TransformTimeout(uint16_t count, uint16_t registerCount);
    void ShaderProgram(uint32_t locationOffset);
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
    void ContextDmaColorC(uint32_t contextC, uint32_t contextD);
    void ContextDmaColorC(uint32_t contextC);
    void ContextDmaColorD(uint32_t context);
    void ContextDmaZeta(uint32_t context);
    void SurfaceFormat(uint8_t colorFormat,
                       uint8_t depthFormat,
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
    void ColorMask(uint32_t mask);
    void DepthFunc(uint32_t zf);
    void CullFaceEnable(bool enable);
    void DepthTestEnable(bool enable);
    void ShadeMode(uint32_t sm);
    void ColorClearValue(uint32_t color);
    void ClearSurface(uint32_t mask);
    void VertexDataArrayFormat(uint8_t index,
                               uint16_t frequency,
                               uint8_t stride,
                               uint8_t size,
                               uint8_t type);
    void VertexDataArrayOffset(unsigned index, uint8_t location, uint32_t offset);
    void BeginEnd(uint32_t mode);
    void DrawArrays(unsigned first, unsigned count);
    void TransformConstantLoad(uint32_t loadAt, std::vector<uint32_t> const& vals);
    void RestartIndexEnable(bool enable);
    void RestartIndex(uint32_t index);
    void IndexArrayAddress(uint8_t location, uint32_t offset, uint32_t type);
    void DrawIndexArray(uint32_t first, uint32_t count);
    void VertexTextureOffset(unsigned index, 
                             uint32_t offset, 
                             uint8_t mipmap,
                             uint8_t format,
                             uint8_t dimension,
                             uint8_t location);
    void VertexTextureControl3(unsigned index, uint32_t pitch);
    void VertexTextureImageRect(unsigned index, uint16_t width, uint16_t height);
    void VertexTextureControl0(unsigned index, bool enable, float minlod, float maxlod);
    void VertexTextureAddress(unsigned index, uint8_t wraps, uint8_t wrapt);
    void VertexTextureFilter(unsigned index, float bias);
    void VertexTextureBorderColor(unsigned index, std::array<float, 4> argb);
    void TextureOffset(unsigned index,
                       uint32_t offset,
                       uint16_t mipmap,
                       uint8_t format,
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
    void TextureBorderColor(unsigned index, std::array<float, int(4)> argb);
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
    void ColorFormat(uint32_t format, uint16_t dstPitch, uint16_t srcPitch);
    void Point(uint16_t pointX, 
               uint16_t pointY, 
               uint16_t outSizeX, 
               uint16_t outSizeY, 
               uint16_t inSizeX, 
               uint16_t inSizeY);
    void Color(std::vector<uint32_t> const& vec);
public:
    Rsx(PPU* ppu);
    ~Rsx();
    void shutdown();
    void setPut(uint32_t put);
    void setGet(uint32_t get);
    uint32_t getGet();
    uint32_t getRef();
    void setRef(uint32_t ref);
    bool isCallActive();
    void setLabel(int index, uint32_t value);
    bool isFlipInProgress() const;
    void resetFlipStatus();
    void setGcmContext(uint32_t ioSize, ps3_uintptr_t ioAddress);
    void setDisplayBuffer(uint8_t id,
                          uint32_t offset,
                          uint32_t pitch,
                          uint32_t width,
                          uint32_t height);
    void init();
    void encodeJump(ps3_uintptr_t va, uint32_t destOffset);
};