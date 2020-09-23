#include "Rsx.h"

#include "GcmConstants.h"
#include "GLBuffer.h"
#include "Cache.h"
#include "GLFramebuffer.h"
#include "GLTexture.h"
#include "../Process.h"
#include "../MainMemory.h"
#include "../ELFLoader.h"
#include "../shaders/ShaderGenerator.h"
#include "../shaders/shader_dasm.h"
#include "../utils.h"
#include "ps3emu/ImageUtils.h"
#include "ps3emu/state.h"
#include "ps3emu/Config.h"
#include "ps3emu/int.h"
#include "ps3emu/utils/ranges.h"
#include <atomic>
#include <vector>
#include <fstream>
#include <boost/algorithm/clamp.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/thread.hpp>
#include <boost/range/numeric.hpp>
#include "ps3emu/libs/graphics/graphics.h"
#include "ps3emu/libs/message.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/AffinityManager.h"

#include "RsxContext.h"
#include "TextureRenderer.h"
#include "GLShader.h"
#include "GLProgramPipeline.h"
#include "GLSampler.h"
#include "Tracer.h"
#include "../log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <unistd.h>
#include <chrono>

using namespace boost::algorithm;
using namespace boost::chrono;
namespace br = boost::range;

typedef std::array<float, 4> glvec4_t;

enum RsxHandlers {
    RSX_HANDLER_QUEUE = 0b01000000,
    RSX_HANDLER_VBLANK = 0b00000010,
    RSX_HANDLER_FLIP = 0b00010000,
    RSX_HANDLER_USER = 0b10000000,
};

class FpsLimiter {
    using clock = std::chrono::steady_clock;

    clock::time_point _start{};
    clock::time_point _swap{};
    clock::duration _target{};
    clock::duration _swapDuration{};

public:
    FpsLimiter(int fps) {
        _target = std::chrono::duration_cast<clock::duration>(std::chrono::duration<float>(1.f / fps));
    }

    void startFrame() {
        _start = clock::now();
        if (_swap != clock::time_point()) {
            _swapDuration = _start - _swap;
        }
    }

    void wait() {
        auto elapsed = clock::now() - _start;
        if (elapsed < _target - _swapDuration) {
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(_target - _swapDuration - elapsed);
            nap(us);
        }
        _swap = clock::now();
    }
};

Rsx::Rsx()
    : _transformFeedbackQuery(0),
      _idleCounter(boost::chrono::seconds(1)) {
    auto regs = (uint32_t*)g_state.mm->getMemoryPointer(GcmControlRegisters, 12);
    _put = regs;
    _get = regs + 1;
    _ref = regs + 2;
    auto flipStatusVa = 0x40201100u;
    _isFlipInProgress = (big_uint32_t*)g_state.mm->getMemoryPointer(flipStatusVa, 4);
    auto lastFlipTimeVa = 0x402010f8u;
    _lastFlipTime = (big_uint64_t*)g_state.mm->getMemoryPointer(lastFlipTimeVa, 4);

    _fpsLimiter = std::make_unique<FpsLimiter>(g_state.config->fpsCap);
}

Rsx::~Rsx() {
    shutdown();
}

void glEnableb(GLenum e, bool enable) {
    if (enable) {
        glEnable(e);
    } else {
        glDisable(e);
    }
}

MemoryLocation gcmEnumToLocation(uint32_t enumValue) {
    if (enumValue == CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER ||
        enumValue == CELL_GCM_LOCATION_LOCAL)
        return MemoryLocation::Local;
    if (enumValue == CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER ||
        enumValue == CELL_GCM_LOCATION_MAIN)
        return MemoryLocation::Main;
    throw std::runtime_error("bad location");
}

void Rsx::setLabel(int index, uint32_t value, bool waitForIdle) {
    if (_mode == RsxOperationMode::Replay)
        return;

    if (waitForIdle) {
        this->waitForIdle();
    }
    auto offset = index * 0x10;
    INFO(rsx) << sformat("setting rsx label at offset {:x} ({:08x})",
                           offset,
                           GcmLabelBaseOffset + offset);
    g_state.mm->store32(GcmLabelBaseOffset + offset, value, g_state.granule);
}

void Rsx::ChannelSetContextDmaSemaphore(uint32_t handle) {
    _activeSemaphoreHandle = handle;
}

void Rsx::ChannelSemaphoreOffset(uint32_t offset) {
    _semaphores[_activeSemaphoreHandle] = offset;
}

void Rsx::ChannelSemaphoreAcquire(uint32_t value) {
    auto offset = _semaphores[_activeSemaphoreHandle];
    INFO(rsx) << sformat("acquiring semaphore {:x} at offset {:x} with value {:x}",
        _activeSemaphoreHandle, GcmLabelBaseOffset + offset, value
    );
    RangeCloser r(&_semaphoreAcquireCounter);

    // here gcm acquires a semaphore that is released by RSX
    // as there is no one to release the semaphore, just release it a second time
    if (_activeSemaphoreHandle == 0x56616661 && offset == 0x30 && value == 1) {
        g_state.mm->store32(GcmLabelBaseOffset + offset, value, g_state.granule);
    }

    if (_activeSemaphoreHandle == 0x66616661 && offset == 0x5a0 && value == 1) {
        g_state.mm->store32(GcmLabelBaseOffset + offset, value, g_state.granule);
    }

    while (g_state.mm->load32(GcmLabelBaseOffset + offset) != value) {
        ums_sleep(1);
    }
    INFO(rsx) << sformat("acquired");
}

void Rsx::SemaphoreRelease(uint32_t value) {
    auto offset = _semaphores[_activeSemaphoreHandle];
    INFO(rsx) << sformat("releasing semaphore {:x} at offset {:x} with value {:x}",
        _activeSemaphoreHandle, GcmLabelBaseOffset + offset, value
    );
    g_state.mm->store32(GcmLabelBaseOffset + offset, value, g_state.granule);
}

void Rsx::TextureReadSemaphoreRelease(uint32_t value) {
    // the label is set when all texture referencing processing
    // and fragment shader processing are complete
    auto offset = _context->semaphoreOffset;
    INFO(rsx) << sformat("releasing texture semaphore at offset {:x} with value {:x}",
                           GcmLabelBaseOffset + offset,
                           value);
    g_state.mm->store32(GcmLabelBaseOffset + offset, value, g_state.granule);
}

void Rsx::ClearRectHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    TRACE(ClearRectHorizontal, x, w, y, h);
    //TODO: implement
}

void Rsx::ClipIdTestEnable(uint32_t x) {
    TRACE(ClipIdTestEnable, x);
    //TODO: implement
}

void Rsx::FlatShadeOp(uint32_t x) {
    TRACE(FlatShadeOp, x);
    //TODO: implement
}

void Rsx::VertexAttribOutputMask(uint32_t mask) {
    TRACE(VertexAttribOutputMask, mask);
    union {
        uint32_t val;
        BitField<31, 32> COL0;
        BitField<30, 31> COL1;
        BitField<29, 30> BCOL0;
        BitField<28, 29> BCOL1;
        BitField<27, 28> FOG;
        BitField<26, 27> PSIZE;
        BitField<25, 26> CLP0;
        BitField<24, 25> CLP1;
        BitField<23, 24> CLP2;
        BitField<22, 23> CLP3;
        BitField<21, 22> CLP4;
        BitField<20, 21> CLP5;
        BitField<18, 19> TEX8;
        BitField<19, 20> TEX9;
        BitField<17, 18> TEX0;
        BitField<16, 17> TEX1;
        BitField<15, 16> TEX2;
        BitField<14, 15> TEX3;
        BitField<13, 14> TEX4;
        BitField<12, 13> TEX5;
        BitField<11, 12> TEX6;
        BitField<10, 11> TEX7;
    } umask = { mask };
    _context->pointSizeVertexOutputEnabled = umask.PSIZE.u();
}

void Rsx::FrequencyDividerOperation(uint16_t op) {
    TRACE(FrequencyDividerOperation, op);
    _context->frequencyDividerOperation = op;
}

void Rsx::TexCoordControl(unsigned int index, uint32_t control) {
    TRACE(TexCoordControl, index, control);
    //TODO: implement
}

void Rsx::ReduceDstColor(bool enable) {
    TRACE(ReduceDstColor, enable);
    //TODO: implement
}

void Rsx::FogMode(uint32_t mode) {
    TRACE(FogMode, mode);
    //TODO: implement
}

void Rsx::AnisoSpread(unsigned int index,
                      bool reduceSamplesEnable,
                      bool hReduceSamplesEnable,
                      bool vReduceSamplesEnable,
                      uint8_t spacingSelect,
                      uint8_t hSpacingSelect,
                      uint8_t vSpacingSelect) {
    TRACE(AnisoSpread,
          index,
          reduceSamplesEnable,
          hReduceSamplesEnable,
          vReduceSamplesEnable,
          spacingSelect,
          hSpacingSelect,
          vSpacingSelect);
    //TODO: implement
}

void Rsx::VertexDataBaseOffset(uint32_t baseOffset, uint32_t baseIndex) {
    TRACE(VertexDataBaseOffset, baseOffset, baseIndex);
    //TODO: implement
}

GLenum gcmOperatorToOpengl(GcmOperator mode) {
#define X(x) case GcmOperator:: x: return GL_##x;
    switch (mode) {
        X(NEVER)
        X(LESS)
        X(LEQUAL)
        X(GREATER)
        X(GEQUAL)
        X(EQUAL)
        X(NOTEQUAL)
        X(ALWAYS)
        default: throw std::runtime_error("bad stencil func");
    }
#undef X
}

void Rsx::AlphaFunc(GcmOperator af, uint32_t ref) {
    TRACE(AlphaFunc, af, ref);
    _context->fragmentOps.alphaFunc = af;
    glAlphaFunc(gcmOperatorToOpengl(af), ref);
}

void Rsx::AlphaTestEnable(bool enable) {
    TRACE(AlphaTestEnable, enable);
    glEnableb(GL_ALPHA_TEST, enable);
}

void Rsx::ShaderControl(bool depthReplace, bool outputFromH0, bool pixelKill, uint8_t registerCount) {
    TRACE(ShaderControl, depthReplace, outputFromH0, pixelKill, registerCount);
    _context->fragmentShaderControl.depthReplace = depthReplace;
    _context->fragmentShaderControl.outputFromH0 = outputFromH0;
    _context->fragmentShaderControl.pixelKill = pixelKill;
    _context->fragmentShaderControl.registerCount = registerCount;
}

void Rsx::TransformProgramLoad(uint32_t load, uint32_t start) {
    TRACE(TransformProgramLoad, load, start);
    assert(load == 0);
    assert(start == 0);
    _context->vertexLoadOffset = load;
}

void Rsx::TransformProgram(uint32_t locationOffset, unsigned size) {
    auto bytes = size * 4;

    const void* source;
    if (_mode == RsxOperationMode::Replay) {
        source = _currentReplayBlob.data();
    } else {
        source = g_state.mm->getMemoryPointer(rsxOffsetToEa(MemoryLocation::Main, locationOffset), bytes);
    }

    _context->tracer.pushBlob(source, bytes);
    TRACE(TransformProgram, locationOffset, size);

    memcpy(&_context->vertexInstructions[_context->vertexLoadOffset], source, bytes);
    _context->vertexShaderDirty = true;
    _context->vertexLoadOffset += bytes;
}

void Rsx::VertexAttribInputMask(InputMask mask) {
    TRACE(VertexAttribInputMask, mask);
    _context->vertexAttribInputMask = mask;
    auto val = (uint32_t)mask;
    for (auto i = 0u; i < 16; ++i) {
        if (!(val & 1)) {
            _context->glVARs.disable(i);
            _context->glVARs.setValue(i, {0, 0, 0, 1});
        }
        val >>= 1;
    }
}

void Rsx::TransformTimeout(uint16_t count, uint16_t registerCount) {
    TRACE(TransformTimeout, count, registerCount);
}

void Rsx::ShaderProgram(uint32_t offset, uint32_t location) {
    if (_mode == RsxOperationMode::Replay) {
        _context->fragmentBytecode = _currentReplayBlob;
    } else {
        auto ea = rsxOffsetToEa(gcmEnumToLocation(location), offset);
        auto ptr = g_state.mm->getMemoryPointer(ea, FragmentProgramSize);
        memcpy(&_context->fragmentBytecode[0], ptr, FragmentProgramSize);
    }

    auto ptr = &_context->fragmentBytecode[0];
    _context->tracer.pushBlob(ptr, FragmentProgramSize);
    TRACE(ShaderProgram, offset, location);

    _context->fragmentShaderDirty = true;
}

void Rsx::ViewportHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    TRACE(ViewportHorizontal, x, w, y, h);
    _context->viewPort.x = x;
    _context->viewPort.y = y;
    _context->viewPort.width = w;
    _context->viewPort.height = h;
}

void Rsx::ClipMin(float min, float max) {
    TRACE(ClipMin, min, max);
    _context->viewPort.zmin = min;
    _context->viewPort.zmax = max;
}

void Rsx::ViewportOffset(float offset0,
                         float offset1,
                         float offset2,
                         float offset3,
                         float scale0,
                         float scale1,
                         float scale2,
                         float scale3) {
    TRACE(ViewportOffset,
          offset0,
          offset1,
          offset2,
          offset3,
          scale0,
          scale1,
          scale2,
          scale3);
    assert(offset3 == 0);
    //assert(scale3 == 0);
    _context->viewPort.offset[0] = offset0;
    _context->viewPort.offset[1] = offset1;
    _context->viewPort.offset[2] = offset2;
    _context->viewPort.scale[0] = scale0;
    _context->viewPort.scale[1] = scale1;
    _context->viewPort.scale[2] = scale2;
    updateViewPort();
}

void Rsx::ColorMask(GcmColorMask mask) {
    TRACE(ColorMask, mask);
    _context->colorMask = mask;
    glColorMask(
        !!(mask & GcmColorMask::R) ? GL_TRUE : GL_FALSE,
        !!(mask & GcmColorMask::G) ? GL_TRUE : GL_FALSE,
        !!(mask & GcmColorMask::B) ? GL_TRUE : GL_FALSE,
        !!(mask & GcmColorMask::A) ? GL_TRUE : GL_FALSE
    );
}

void Rsx::DepthTestEnable(bool enable) {
    TRACE(DepthTestEnable, enable);
    _context->fragmentOps.depthTest = enable;
    glEnableb(GL_DEPTH_TEST, enable);
}

void Rsx::DepthFunc(GcmOperator zf) {
    TRACE(DepthFunc, zf);
    _context->fragmentOps.depthFunc = zf;
    glDepthFunc(gcmOperatorToOpengl(zf));
}

void Rsx::CullFaceEnable(bool enable) {
    TRACE(CullFaceEnable, enable);
    //_context->isCullFaceEnabled = enable;
    //glEnableb(GL_CULL_FACE, enable);
}

void Rsx::FrontFace(GcmFrontFace dir) {
    TRACE(FrontFace, dir);
    _context->cullFaceDirection = dir;
    glFrontFace(dir == GcmFrontFace::CW ? GL_CW : GL_CCW);
}

void Rsx::ShadeMode(uint32_t sm) {
    TRACE(ShadeMode, sm);
    _context->isFlatShadeMode = sm == CELL_GCM_FLAT;
    _context->fragmentShaderDirty = true;
    assert(sm == CELL_GCM_SMOOTH || sm == CELL_GCM_FLAT);
}

void Rsx::ColorClearValue(uint32_t color) {
    TRACE(ColorClearValue, color);
    _context->fragmentOps.clearColor = color;
}

void Rsx::ClearSurface(GcmClearMask mask) {
    TRACE(ClearSurface, mask);
    auto glmask = 0;
    auto colorMask =
        GcmClearMask::R | GcmClearMask::G | GcmClearMask::B | GcmClearMask::A;
    if (!!(mask & colorMask))
        glmask |= GL_COLOR_BUFFER_BIT;
    if (!!(mask & GcmClearMask::Z))
        glmask |= GL_DEPTH_BUFFER_BIT;
    if (!!(mask & GcmClearMask::S))
        glmask |= GL_STENCIL_BUFFER_BIT;

    union {
        uint32_t val;
        BitField<0, 8> a;
        BitField<8, 16> r;
        BitField<16, 24> g;
        BitField<24, 32> b;
    } c = { _context->fragmentOps.clearColor };

    _context->fragmentOps.clearMask = mask;
    glClearColor(c.r.u() / 255., c.g.u() / 255., c.b.u() / 255., c.a.u() / 255.);
    glClear(glmask);
}

void Rsx::VertexDataArrayFormat(uint8_t index,
                                uint16_t frequency,
                                uint8_t stride,
                                uint8_t size,
                                VertexInputType type) {
    TRACE(VertexDataArrayFormat, index, frequency, stride, size, type);
    auto& format = _context->vertexDataArrays[index];
    format.frequency = frequency;
    format.stride = stride;
    format.size = size;
    format.type = type;

    auto& vinput = _context->vertexInputs[index];
    vinput.rank = size;
    vinput.type = type;
}

void Rsx::VertexDataArrayOffset(unsigned index, uint8_t location, uint32_t offset) {
    TRACE(VertexDataArrayOffset, index, location, offset);
    auto& array = _context->vertexDataArrays[index];
    array.location = gcmEnumToLocation(location);
    array.offset = offset;
    array.binding = index;
    GLint maxBindings;
    (void)maxBindings;
    assert((GLint)array.binding < (glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &maxBindings), maxBindings));
}

GLenum gcmPrimitiveToOpengl(GcmPrimitive primitive) {
#define X(x) case GcmPrimitive:: x: return GL_##x;
    switch (primitive) {
        X(QUADS)
        X(QUAD_STRIP)
        X(POLYGON)
        X(POINTS)
        X(LINES)
        X(LINE_LOOP)
        X(LINE_STRIP)
        X(TRIANGLES)
        X(TRIANGLE_STRIP)
        X(TRIANGLE_FAN)
        case GcmPrimitive::NONE: return 0;
        default: assert(false); return 0;
    }
#undef X
}

void Rsx::BeginEnd(GcmPrimitive mode) {
    TRACE(BeginEnd, mode);
    _context->vertexArrayMode = mode;
    _context->glVertexArrayMode = gcmPrimitiveToOpengl(mode);
}

GLuint primitiveTypeToFeedbackPrimitiveType(GLuint type) {
    switch (type) {
        case GL_TRIANGLES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
        case GL_QUADS:
        case GL_QUAD_STRIP:
            return GL_TRIANGLES;
        case GL_LINES:
        case GL_LINE_LOOP:
        case GL_LINE_STRIP:
            return GL_LINES;
        case GL_POINTS:
            return GL_POINTS;
        default: assert(false);
    }
    return 0;
}

void Rsx::beginTransformFeedback() {
    if (_mode == RsxOperationMode::Replay) {
        _transformFeedbackQuery = GLQuery();
        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, _transformFeedbackQuery.handle());
        _context->feedbackMode =
            primitiveTypeToFeedbackPrimitiveType(_context->glVertexArrayMode);
        glBeginTransformFeedback(_context->feedbackMode);
    }
}

void Rsx::endTransformFeedback() {
    if (_mode == RsxOperationMode::Replay) {
        glEndTransformFeedback();
        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
        glGetQueryObjectuiv(_transformFeedbackQuery.handle(), GL_QUERY_RESULT, &_context->feedbackCount);
        if (_context->feedbackMode == GL_TRIANGLES)
            _context->feedbackCount *= 3;
        if (_context->feedbackMode == GL_LINES)
            _context->feedbackCount *= 2;
    }
}

void Rsx::advanceBuffers() {
    _context->drawRingBuffer->advance();
}

void Rsx::DrawArrays(unsigned first, unsigned count) {
    updateTextures();
    updateShaders();
    watchTextureCache();
    resetCacheWatch();
    linkShaderProgram();
    updateScissor();
    updateVertexDataArrays(first, count);

    beginTransformFeedback();
    glDrawArrays(_context->glVertexArrayMode, 0, count);
    endTransformFeedback();
    // flush to make the buffers available faster
    glFlush();

    // Right after a gcm draw command completes, the next command might immediately
    // update buffers or shader constants. OpenGL draw commands are asynchronous
    // and as such need to be synchronized.
    // OpenGL guarantees that all buffers are immediately available for change
    // after a draw command, but this isn't true for persistent buffers.
    advanceBuffers();

    TRACE(DrawArrays, first, count);
}

unsigned vertexDataArrayTypeSize(VertexInputType type) {
    switch (type) {
        case VertexInputType::x11y11z10n:
        case VertexInputType::u32:
        case VertexInputType::f32: return 4;
        case VertexInputType::u16:
        case VertexInputType::f16:
        case VertexInputType::s1: return 2;
        case VertexInputType::u8: return 1;
        default: throw std::runtime_error("unsupported vertex data array type");
    }
}

void Rsx::InlineArray(uint32_t offset, unsigned count) {
    TRACE(InlineArray, offset, count);
    auto stride = 0u;
    for (auto i = 0u; i < _context->vertexDataArrays.size(); ++i) {
        auto& format = _context->vertexDataArrays[i];
        auto& input = _context->vertexInputs[i];
        if (input.rank) {
            format.offset = offset + stride;
            format.location = MemoryLocation::Main;
            stride += format.size * vertexDataArrayTypeSize(format.type);
        }
    }
    if (_mode == RsxOperationMode::Replay)
        return;
    DrawArrays(0, count * 4 / stride);
}

void Rsx::setFlipStatus(bool inProgress) {
    *_isFlipInProgress = inProgress ? 0 : 0x80000000;
}

struct VertexShaderViewportUniform {
    glm::mat4 glInverseGcm;
};

static_assert(sizeof(VertexShaderViewportUniform) == 4 * 16, "must be packed");

void Rsx::setGcmContext(uint32_t ioSize, ps3_uintptr_t ioAddress) {
    _gcmIoSize = ioSize;
    _gcmIoAddress = ioAddress;
}

void Rsx::invokeHandler(uint32_t num, uint32_t arg) {
    if (!_callbackQueue)
        return;
    g_state.mm->store32(0x402012cc, arg, g_state.granule);
    sys_event_port_send(_callbackQueuePort, 0, num, 0);
}

void Rsx::resetContext() {
    _context->reportLocation = MemoryLocation::Local;
    for (auto i = 0u; i < _context->vertexInputs.size(); ++i) {
        _context->vertexInputs[i].rank = 0;
        _context->glVARs.disable(i);
    }
    for (auto& s : _context->vertexTextureSamplers) {
        s.enable = false;
    }
    for (auto& s : _context->fragmentTextureSamplers) {
        s.enable = false;
        s.fragmentMin = CELL_GCM_TEXTURE_NEAREST_LINEAR;
        s.fragmentMag = CELL_GCM_TEXTURE_LINEAR;
    }
}

void Rsx::DriverQueue(uint32_t id) {
    TRACE(DriverQueue, id);
    _context->flipBuffer = id;
    setFlipStatus(true);
}

void Rsx::DriverFlip(uint32_t value) {
    TRACE(DriverFlip, value);
    auto& fb = _context->displayBuffers[_context->flipBuffer];
    auto va = fb.offset + RsxFbBaseAddr;
    FramebufferTextureKey key{va, fb.width, fb.height, GL_RGBA8}; // GL_RGB32F
    auto tex = _context->framebuffer->findTexture(key).texture;
    if (!tex && _mode == RsxOperationMode::Replay)
        return;

    if (!tex) {
        auto it = br::find_if(_context->surfaceLinks, [&](auto& link) {
            return link.framebufferEa == va;
        });
        if (it != end(_context->surfaceLinks)) {
            key.offset = it->surfaceEa;
            tex = _context->framebuffer->findTexture(key).texture;
        }
    }
    _context->framebuffer->bindDefault();
    if (tex) {
        _context->textureRenderer->render(tex);
        emuMessageDraw(tex->width(), tex->height());
    }
    drawStats();
    _context->pipeline.bind();

    _fpsLimiter->wait();
    _window.swapBuffers();

    _fpsLimiter->startFrame();

    __itt_frame_end_v3(_profilerDomain, NULL);
    __itt_frame_begin_v3(_profilerDomain, NULL);

    _fpsCounter.openRange(counter_clock_t::now());
    _fpsCounter.closeRange(counter_clock_t::now());

    *_lastFlipTime = g_state.proc->getTimeBase();
    _context->frame++;
    _context->commandNum = 0;

#if TESTS
    static int framenum = 0;
    auto id = getpid();
    if (framenum < 22 && tex) {
        auto filename = sformat("{}/ps3frame_{}_{}.png", getTestOutputDir(), id, framenum);
        dumpOpenGLTexture(tex->handle(), false, 0, filename, true, true);
        framenum++;
    }
#endif

    setFlipStatus(false);

    // RSX releases the semaphore here
    this->setLabel(1, 0, false);

    invokeHandler(RSX_HANDLER_VBLANK | RSX_HANDLER_FLIP, 0);

    if (_frameCapturePending) {
        _frameCapturePending = false;
        _context->frame = 0;
        _context->commandNum = 0;
        _mode = RsxOperationMode::RunCapture;
        _context->tracer.enable(true);
        updateDisplayBuffersForCapture();
    }

    if (_context->frame > 2 && _shortTrace) {
        _shortTrace = false;
        _mode = RsxOperationMode::Run;
        _context->tracer.enable(false);
    }

    resetContext();
}

void Rsx::updateDisplayBuffersForCapture() {
    for (auto i = 0u; i < _context->displayBuffers.size(); ++i) {
        auto& b = _context->displayBuffers[i];
        setDisplayBuffer(i, b.offset, b.height);
    }
}

void Rsx::drawStats() {
    _context->statText.clear();

    auto fpsCount = std::get<1>(_fpsCounter.value(counter_clock_t::now()));
    auto line = sformat("FPS: {}", fpsCount);
    _context->statText.line(0, 0, line);

    auto [idleSum, idleCount] = _idleCounter.value();
    boost::chrono::duration<float> loadDuration = boost::chrono::seconds(1) - idleSum;

    _context->statText.line(sformat("RSX load: {:1.2f}, wait count: {}",
                                      loadDuration.count(),
                                      idleCount));

    static std::vector<std::tuple<MethodMapEntry*, int, counter_duration_t>> vec;
    vec.clear();
    for (auto& [entry, counter] : _perfMap) {
        auto [sum, count] = counter.value();
        vec.push_back({entry, count, sum});
    }
    ranges::sort(vec, std::greater<>(), [](auto& x) { return std::get<2>(x); });

    int i = 0;
    for (auto& [entry, count, sum] : vec) {
        line = sformat("{:<8} {:<1.4f} %{:<1.4f}   {}",
                         count,
                         boost::chrono::duration<float>(sum).count(),
                         boost::chrono::duration<float>(sum).count() / fpsCount,
                         entry->name + 9);
        _context->statText.line(line);
        if (i++ == 10)
            break;
    }

    float total = 0;

    _context->statText.move(500, 0);

#define STAT_LINE(name, counter)                                                    \
    {                                                                               \
        auto[sum, count] = counter.value();                                         \
        auto sumValue = boost::chrono::duration<float>(sum).count();                \
        _context->statText.line(sformat("{:<14} {:<8}  {:<1.4f}  {:<1.6f}",         \
                                          name,                                     \
                                          count,                                    \
                                          sumValue,                                 \
                                          sumValue / fpsCount));                    \
        total += sumValue;                                                          \
    }

    if (log_should(log_info, log_type_t::perf)) {
        STAT_LINE("texture", _textureCounter);
        STAT_LINE("fragment sh", _fragmentShaderCounter);
        STAT_LINE("vertex sh", _vertexShaderCounter);
        STAT_LINE("fragment retr", _fragmentShaderRetrieveCounter);
        STAT_LINE("vertex retr", _vertexShaderRetrieveCounter);
        STAT_LINE("textureCache", _textureCacheCounter);
        STAT_LINE("resetCache", _resetCacheCounter);
        STAT_LINE("linkShader", _linkShaderCounter);
        STAT_LINE("vda", _vdaCounter);
        STAT_LINE("waitForIdle", _waitingForIdleCounter);
        STAT_LINE("index copying", _indexArrayProcessingCounter);
        STAT_LINE("load texture", _loadTextureCounter);
        STAT_LINE("callback", _callbackCounter);
        STAT_LINE("semaphore", _semaphoreAcquireCounter);

        _context->statText.line(
            sformat("total time: {:1.4f} (real), {:1.4f} (frame adjusted)",
                    total,
                    total / fpsCount));
    }

    _context->statText.render(_window.width(), _window.height(), {0,1,0,1}, {});
}

void Rsx::TransformConstantLoad(uint32_t loadAt, uint32_t offset, uint32_t count) {
    //assert(count % 4 == 0);
    auto size = count * sizeof(float);

    static std::vector<uint8_t> source;
    if (_mode == RsxOperationMode::Replay) {
        source = _currentReplayBlob;
    } else {
        source.resize(size);
        g_state.mm->readMemory(
            rsxOffsetToEa(MemoryLocation::Main, offset), &source[0], source.size());
    }

    _context->tracer.pushBlob(&source[0], source.size());
    TRACE(TransformConstantLoad, loadAt, offset, count);

    for (auto i = 0u; i < count; ++i) {
        auto u = (uint32_t*)&source[i * 4];
        boost::endian::endian_reverse_inplace(*u);
    }

    for (auto i = 0u; i < count; i += 4) {
        auto u = (float*)&source[i * 4];
        INFO(rsx) << sformat(
            "vconst[{:03}] = {{ {}, {}, {}, {} }}", loadAt + i, u[0], u[1], u[2], u[3]);
    }

    auto mapped = (uint8_t*)_context->drawRingBuffer->current(vertexConstBuffer) + loadAt * 16;
    memcpy(mapped, source.data(), size);
}

bool Rsx::linkShaderProgram() {
    RangeCloser r(&_linkShaderCounter);

    if (!_context->fragmentShader || !_context->vertexShader)
        return false;

    _context->pipeline.useShader(_context->vertexShader);
    _context->pipeline.useShader(_context->fragmentShader);
#if !NDEBUG
    _context->pipeline.validate();
#endif

    _context->drawRingBuffer->bindUniform(vertexConstBuffer, VertexShaderConstantBinding);
    _context->drawRingBuffer->bindUniform(vertexSamplersBuffer, VertexShaderSamplesInfoBinding);
    _context->drawRingBuffer->bindUniform(vertexViewportBuffer, VertexShaderViewportMatrixBinding);
    _context->drawRingBuffer->bindUniform(fragmentSamplersBuffer, FragmentShaderSamplesInfoBinding);

    if (_mode == RsxOperationMode::Replay) {
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,
                         TransformFeedbackBufferBinding,
                         _context->feedbackBuffer.handle());
    }
    return true;
}

void Rsx::RestartIndexEnable(bool enable) {
    TRACE(RestartIndexEnable, enable);
    glEnableb(GL_PRIMITIVE_RESTART, enable);
}

void Rsx::RestartIndex(uint32_t index) {
    TRACE(RestartIndex, index);
    glPrimitiveRestartIndex(index);
}

void Rsx::IndexArrayAddress1(uint32_t offset) {
    TRACE(IndexArrayAddress1, offset);
    _context->indexArray.offset = offset;
}

void Rsx::IndexArrayDma(uint8_t location, GcmDrawIndexArrayType type) {
    TRACE(IndexArrayDma, location, type);
    _context->indexArray.location = gcmEnumToLocation(location);
    _context->indexArray.type = type;
    _context->indexArray.glType = type == GcmDrawIndexArrayType::_16
                                      ? GL_UNSIGNED_SHORT
                                      : GL_UNSIGNED_INT;
}

void Rsx::IndexArrayAddress(uint8_t location, uint32_t offset, GcmDrawIndexArrayType type) {
    TRACE(IndexArrayAddress, location, offset, type);
    _context->indexArray.location = gcmEnumToLocation(location);
    _context->indexArray.offset = offset;
    _context->indexArray.type = type;
    _context->indexArray.glType = type == GcmDrawIndexArrayType::_16
                                      ? GL_UNSIGNED_SHORT
                                      : GL_UNSIGNED_INT;
}

void Rsx::DrawIndexArray(uint32_t first, uint32_t count) {
    assert(first == 0);

    auto destBuffer = &_context->elementArrayIndexBuffer;
    auto sourceBuffer = getBuffer(_context->indexArray.location);
    auto byteSize = _context->indexArray.type == GcmDrawIndexArrayType::_16 ? 2 : 4;
    assert(count * byteSize <= destBuffer->size());

    {
        RangeCloser r(&_indexArrayProcessingCounter);

        auto source = (void*)((uintptr_t)sourceBuffer->mapped() + _context->indexArray.offset);
        auto dest = destBuffer->mapped();

        if (byteSize == 2) {
            fast_endian_reverse<2>(dest, source, count);
        } else {
            fast_endian_reverse<4>(dest, source, count);
        }
    }

    if (_mode != RsxOperationMode::Replay) {
        UpdateBufferCache(_context->indexArray.location,
                          _context->indexArray.offset + first * byteSize,
                          count * byteSize);
    }

    updateTextures();
    updateShaders();
    watchTextureCache();
    resetCacheWatch();
    linkShaderProgram();
    updateScissor();
    updateVertexDataArrays(first, count);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destBuffer->handle());

    auto offset = (void*)(uintptr_t)(first * byteSize);
    beginTransformFeedback();
    glDrawElements(_context->glVertexArrayMode, count, _context->indexArray.glType, offset);
    endTransformFeedback();
    glFlush();

    // see DrawArrays for the rationale
    advanceBuffers();
    waitForIdle(); // TODO: remove after handling GL_ELEMENT_ARRAY_BUFFER

    TRACE(DrawIndexArray, first, count);
}

std::array<int, 16> getFragmentSamplerSizes(const RsxContext* context) {
    std::array<int, 16> sizes = { 0 };
    for (int i = 0; i < 16; ++i) {
        auto& s = context->fragmentTextureSamplers[i];
        if (!s.enable)
            continue;
        sizes[i] = s.texture.fragmentCubemap ? 6 : s.texture.dimension;
    }
    return sizes;
}

bool isMrt(SurfaceInfo const& surface) {
    return boost::accumulate(surface.colorTarget, 0) > 1;
}

void Rsx::updateShaders() {
    glEnableb(GL_PROGRAM_POINT_SIZE, _context->pointSizeVertexOutputEnabled);
    auto fragmentSamplerUniform = (FragmentShaderSamplerUniform*)_context->drawRingBuffer->current(fragmentSamplersBuffer);
    fragmentSamplerUniform->outputFromH = _context->fragmentShaderControl.outputFromH0;
    fragmentSamplerUniform->pointSpriteControl[0] = _context->pointSpriteControl.enabled;
    fragmentSamplerUniform->pointSpriteControl[1] = !!(_context->pointSpriteControl.tex & PointSpriteTex::Tex0);
    fragmentSamplerUniform->pointSpriteControl[2] = !!(_context->pointSpriteControl.tex & PointSpriteTex::Tex1);
    fragmentSamplerUniform->pointSpriteControl[3] = !!(_context->pointSpriteControl.tex & PointSpriteTex::Tex2);
    fragmentSamplerUniform->pointSpriteControl[4] = !!(_context->pointSpriteControl.tex & PointSpriteTex::Tex3);
    fragmentSamplerUniform->pointSpriteControl[5] = !!(_context->pointSpriteControl.tex & PointSpriteTex::Tex4);
    fragmentSamplerUniform->pointSpriteControl[6] = !!(_context->pointSpriteControl.tex & PointSpriteTex::Tex5);
    fragmentSamplerUniform->pointSpriteControl[7] = !!(_context->pointSpriteControl.tex & PointSpriteTex::Tex6);
    fragmentSamplerUniform->pointSpriteControl[8] = !!(_context->pointSpriteControl.tex & PointSpriteTex::Tex7);
    fragmentSamplerUniform->pointSpriteControl[9] = !!(_context->pointSpriteControl.tex & PointSpriteTex::Tex8);
    fragmentSamplerUniform->pointSpriteControl[10] = !!(_context->pointSpriteControl.tex & PointSpriteTex::Tex9);

    if (_context->fragmentShaderDirty) {
        _context->fragmentShaderDirty = false;

        _fragmentShaderRetrieveCounter.openRange();

        auto fconst = (std::array<float, 4>*)_context->drawRingBuffer->current(fragmentConstBuffer);
        auto key = _context->fragmentShaderCache.unzip(&_context->fragmentBytecode[0], fconst, isMrt(_context->surface));
        auto shader = _context->fragmentShaderCache.retrieve(key);

        _fragmentShaderRetrieveCounter.closeRange();

        if (!shader) {
            RangeCloser r(&_fragmentShaderCounter);

            auto sizes = getFragmentSamplerSizes(_context.get());
            INFO(libs) << sformat("Updated fragment shader:\n{}\n{}",
                            PrintFragmentBytecode(&_context->fragmentBytecode[0]),
                            PrintFragmentProgram(&_context->fragmentBytecode[0]));
            auto text = GenerateFragmentShader(_context->fragmentBytecode,
                                               sizes,
                                               _context->isFlatShadeMode,
                                               isMrt(_context->surface));
            shader = new FragmentShader(text.c_str());
            INFO(libs) << sformat("Updated fragment shader (2):\n{}\n{}", text, shader->log());
            _context->fragmentShaderCache.insert(key, shader);
        }
        _context->fragmentShader = shader;
    }

    _context->drawRingBuffer->bindUniform(fragmentConstBuffer, FragmentShaderConstantBinding);

    if (_context->vertexShaderDirty) {
        _context->vertexShaderDirty = false;

        _vertexShaderRetrieveCounter.openRange();

        std::array<int, 4> samplerSizes = {
            _context->vertexTextureSamplers[0].enable
                ? _context->vertexTextureSamplers[0].texture.dimension
                : 2,
            _context->vertexTextureSamplers[1].enable
                ? _context->vertexTextureSamplers[1].texture.dimension
                : 2,
            _context->vertexTextureSamplers[2].enable
                ? _context->vertexTextureSamplers[2].texture.dimension
                : 2,
            _context->vertexTextureSamplers[3].enable
                ? _context->vertexTextureSamplers[3].texture.dimension
                : 2,
        };

        std::array<uint8_t, 16> arraySizes;
        assert(_context->vertexInputs.size() == arraySizes.size());
        for (auto i = 0u; i < arraySizes.size(); ++i) {
            arraySizes[i] = _context->vertexInputs[i].rank;
        }

        auto shader = _context->vertexShaderCache.retrieve(&_context->vertexInstructions[0],
                                                           _context->vertexLoadOffset,
                                                           arraySizes);

        _vertexShaderRetrieveCounter.closeRange();

        if (!shader) {
            RangeCloser r(&_vertexShaderCounter);
            INFO(rsx) << "updating vertex shader";
            INFO(rsx) << sformat("updated vertex shader:\n{}\n{}",
                     PrintVertexBytecode(&_context->vertexInstructions[0]),
                     PrintVertexProgram(&_context->vertexInstructions[0]));
            auto text = GenerateVertexShader(&_context->vertexInstructions[0],
                                             _context->vertexInputs,
                                             samplerSizes,
                                             0, // TODO: loadAt
                                             nullptr,
                                             _mode == RsxOperationMode::Replay);
            shader = new VertexShader(text.c_str());
            INFO(rsx) << sformat("updated vertex shader (2):\n{}\n{}", text, shader->log());
            _context->vertexShaderCache.insert(&_context->vertexInstructions[0],
                                               _context->vertexLoadOffset,
                                               arraySizes,
                                               shader);
        }
        _context->vertexShader = shader;
    }
}

void Rsx::VertexTextureOffset(unsigned index,
                              uint32_t offset,
                              uint8_t mipmap,
                              GcmTextureFormat format,
                              GcmTextureLnUn lnUn,
                              uint8_t dimension,
                              uint8_t location)
{
    TRACE(VertexTextureOffset, index, offset, mipmap, format, lnUn, dimension, location);
    auto& t = _context->vertexTextureSamplers[index].texture;
    t.offset = offset;
    t.mipmap = mipmap;
    t.format = format;
    t.lnUn = lnUn;
    t.dimension = dimension;
    t.location = gcmEnumToLocation(location);
}

void Rsx::VertexTextureControl3(unsigned index, uint32_t pitch) {
    TRACE(VertexTextureControl3, index, pitch);
    _context->vertexTextureSamplers[index].texture.pitch = pitch;
}

void Rsx::VertexTextureImageRect(unsigned index, uint16_t width, uint16_t height) {
    TRACE(VertexTextureImageRect, index, width, height);
    _context->vertexTextureSamplers[index].texture.width = width;
    _context->vertexTextureSamplers[index].texture.height = height;
}

void Rsx::VertexTextureControl0(unsigned index, bool enable, float minlod, float maxlod) {
    TRACE(VertexTextureControl0, index, enable, minlod, maxlod);
    _context->vertexTextureSamplers[index].enable = enable;
    _context->vertexTextureSamplers[index].minlod = minlod;
    _context->vertexTextureSamplers[index].maxlod = maxlod;
}

void Rsx::VertexTextureAddress(unsigned index, uint8_t wraps, uint8_t wrapt) {
    TRACE(VertexTextureAddress, index, wraps, wrapt);
    _context->vertexTextureSamplers[index].wraps = wraps;
    _context->vertexTextureSamplers[index].wrapt = wrapt;
}

void Rsx::VertexTextureFilter(unsigned int index, float bias) {
    TRACE(VertexTextureFilter, index, bias);
    _context->vertexTextureSamplers[index].bias = bias;
}

GLTexture* Rsx::addTextureToCache(uint32_t samplerId, bool isFragment) {
    auto& info = isFragment ? _context->fragmentTextureSamplers.at(samplerId).texture
                            : _context->vertexTextureSamplers.at(samplerId).texture;
    TextureCacheKey key { info.offset, (uint32_t)info.location, info.width, info.height, info.format };
    uint32_t va = rsxOffsetToEa(info.location, key.offset);
    auto texelByteSize = 16; // TODO: calculate from format
    auto size = (info.pitch == 0 ? (info.width * texelByteSize) : info.pitch) * info.height;
    auto updater = new SimpleCacheItemUpdater<GLTexture> {
        va, size,
        [=, this](auto t) {
            RangeCloser r(&_loadTextureCounter);
//             std::vector<uint8_t> buf(size);
//             g_state.mm->readMemory(va, &buf[0], size);
//             t->update(buf);

            auto buffer = getBuffer(info.location);
            _textureReader->loadTexture(info, buffer->handle(), t->levelHandles());
        }
    };
    auto texture = new GLTexture(info);
    _context->textureCache.insert(key, texture, updater);
    return _context->textureCache.retrieve(key);
}

GLTexture* Rsx::getTextureFromCache(uint32_t samplerId, bool isFragment) {
    auto& info = isFragment ? _context->fragmentTextureSamplers[samplerId].texture
                            : _context->vertexTextureSamplers[samplerId].texture;
    TextureCacheKey key {
        info.offset,
        (uint32_t)info.location,
        info.width,
        info.height,
        info.format
    };

    if (_mode != RsxOperationMode::Replay) {
        auto height = info.width * info.height * 4;
        height *= 2; // append possible mipmaps
        UpdateBufferCache(info.location, info.offset, height);
    }

    auto texture = _context->textureCache.retrieve(key);
    if (!texture) {
        texture = addTextureToCache(samplerId, isFragment);
    }
    return texture;
}

GLenum gcmTextureMinFilterToOpengl(uint8_t filter) {
    static_assert(CELL_GCM_TEXTURE_LINEAR_NEAREST ==
                  CELL_GCM_TEXTURE_CONVOLUTION_MAG, "");
    switch (filter) {
        case CELL_GCM_TEXTURE_CONVOLUTION_MIN:
        case CELL_GCM_TEXTURE_NEAREST: return GL_NEAREST;
        case CELL_GCM_TEXTURE_LINEAR: return GL_LINEAR;
        case CELL_GCM_TEXTURE_NEAREST_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
        case CELL_GCM_TEXTURE_LINEAR_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
        case CELL_GCM_TEXTURE_NEAREST_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
        case CELL_GCM_TEXTURE_LINEAR_LINEAR: return GL_LINEAR_MIPMAP_LINEAR;
        default: throw std::runtime_error("bad filter value");
    }
}

GLenum gcmTextureMagFilterToOpengl(uint8_t filter) {
    switch (filter) {
        case CELL_GCM_TEXTURE_CONVOLUTION_MAG:
        case CELL_GCM_TEXTURE_NEAREST: return GL_NEAREST;
        case CELL_GCM_TEXTURE_LINEAR: return GL_LINEAR;
        default: throw std::runtime_error("bad filter value");
    }
}

GLenum gcmFragmentTextureWrapToOpengl(unsigned wrap) {
    switch (wrap) {
        case 0:
        case CELL_GCM_TEXTURE_WRAP: return GL_REPEAT;
        case CELL_GCM_TEXTURE_MIRROR: return GL_MIRRORED_REPEAT;
        case CELL_GCM_TEXTURE_BORDER:
        case CELL_GCM_TEXTURE_CLAMP:
        case CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER:
        case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP:
        case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE:
        case CELL_GCM_TEXTURE_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
        default: throw std::runtime_error("unknown wrap mode");
    }
}

void Rsx::updateTextures() {
    RangeCloser r(&_textureCounter);
    int i = 0;
    auto vertexSamplerUniform = (VertexShaderSamplerUniform*)_context->drawRingBuffer->current(vertexSamplersBuffer);
    for (auto& sampler : _context->vertexTextureSamplers) {
        if (sampler.enable) {
            auto textureUnit = i + VertexTextureUnit;
            auto texture = getTextureFromCache(i, false);
            texture->bind(textureUnit);
            auto handle = sampler.glSampler.handle();
            if (!handle) {
                sampler.glSampler = GLSampler();
                handle = sampler.glSampler.handle();
                glBindSampler(textureUnit, handle);
            }
            glSamplerParameterf(handle, GL_TEXTURE_MIN_LOD, sampler.minlod);
            glSamplerParameterf(handle, GL_TEXTURE_MAX_LOD, sampler.maxlod);
            glSamplerParameterf(handle, GL_TEXTURE_LOD_BIAS, sampler.bias);
            glSamplerParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glSamplerParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glSamplerParameteri(handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glSamplerParameteri(handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            vertexSamplerUniform->wraps[i] = { sampler.wraps, sampler.wrapt, 0, 0 };
            vertexSamplerUniform->borderColor[i] = sampler.borderColor;
        }
        i++;
    }

    i = 0;
    auto fragmentSamplerUniform = (FragmentShaderSamplerUniform*)_context->drawRingBuffer->current(fragmentSamplersBuffer);
    for (auto& sampler : _context->fragmentTextureSamplers) {
        if (sampler.enable && isTextureValid(sampler.texture)) {
            auto textureUnit = i + FragmentTextureUnit;
            auto va = rsxOffsetToEa(sampler.texture.location, sampler.texture.offset);
            FramebufferTextureKey key{va, sampler.texture.width, sampler.texture.height, GL_RGBA8}; // GL_RGB32F
            auto surfaceTexResult = _context->framebuffer->findTexture(key);
            auto surfaceTex = surfaceTexResult.texture;
            if (surfaceTex) {
                fragmentSamplerUniform->xOffset[i] = surfaceTexResult.xOffset;
                fragmentSamplerUniform->yOffset[i] = surfaceTexResult.yOffset;
                fragmentSamplerUniform->xScale[i] = surfaceTexResult.xScale;
                fragmentSamplerUniform->yScale[i] = surfaceTexResult.yScale;
                glBindTextureUnit(textureUnit, surfaceTex->handle());
            } else {
                fragmentSamplerUniform->xOffset[i] = 0;
                fragmentSamplerUniform->yOffset[i] = 0;
                fragmentSamplerUniform->xScale[i] = 1;
                fragmentSamplerUniform->yScale[i] = 1;
                auto texture = getTextureFromCache(i, true);
                texture->bind(textureUnit);
            }

            auto handle = sampler.glSampler.handle();
            if (!handle) {
                sampler.glSampler = GLSampler();
                handle = sampler.glSampler.handle();
                glBindSampler(textureUnit, handle);
            }

            sampler.glSampler.setMinLOD(sampler.minlod);
            sampler.glSampler.setMaxLOD(sampler.maxlod);
            sampler.glSampler.setLODBias(sampler.bias);
            sampler.glSampler.setMinFilter(gcmTextureMinFilterToOpengl(sampler.fragmentMin));
            sampler.glSampler.setMagFilter(gcmTextureMagFilterToOpengl(sampler.fragmentMag));
            sampler.glSampler.setWrapS(gcmFragmentTextureWrapToOpengl(sampler.wraps));
            sampler.glSampler.setWrapT(gcmFragmentTextureWrapToOpengl(sampler.wrapt));

            // TODO: handle viewport CELL_GCM_WINDOW_ORIGIN_TOP
            fragmentSamplerUniform->flip[i] = surfaceTex != nullptr;
        }
        i++;
    }
}

void Rsx::init(sys_event_queue_t callbackQueue) {
    INFO(rsx) << "waiting for rsx loop to initialize";
    _profilerDomain = __itt_domain_create("rsx");
    _loopProfilerTask = __itt_string_handle_create("loop");
    _callbackQueue = callbackQueue;

    if (_callbackQueue) {
        auto res = sys_event_port_create(&_callbackQueuePort, SYS_EVENT_PORT_LOCAL, 0);
        assert(!res); (void)res;
        res = sys_event_port_connect_local(_callbackQueuePort, _callbackQueue);
        assert(!res);
    }

    boost::unique_lock<boost::mutex> lock(_initMutex);
    _thread = std::make_unique<boost::thread>([this]{ loop(); });
    assignAffinity(_thread->native_handle(), AffinityGroup::PPUHost);
    initMethodMap();
    _initCv.wait(lock, [this] { return _initialized; });

    INFO(rsx) << "rsx loop completed initialization";
}

void Rsx::VertexTextureBorderColor(unsigned int index, float a, float r, float g, float b) {
    TRACE(VertexTextureBorderColor, index, a, r, g, b);
    _context->vertexTextureSamplers[index].borderColor = { a, r, g, b };
}

void Rsx::TextureAddress(unsigned index,
                         uint8_t wraps,
                         uint8_t wrapt,
                         uint8_t wrapr,
                         uint8_t unsignedRemap,
                         uint8_t zfunc,
                         uint8_t gamma,
                         uint8_t anisoBias,
                         uint8_t signedRemap) {
    TRACE(TextureAddress,
          index,
          wraps,
          wrapt,
          wrapr,
          unsignedRemap,
          zfunc,
          gamma,
          anisoBias,
          signedRemap);
    auto& s = _context->fragmentTextureSamplers[index];
    s.wraps = wraps;
    s.wrapt = wrapt;
    s.fragmentWrapr = wrapr;
    s.fragmentZfunc = zfunc;
    s.fragmentAnisoBias = anisoBias;
    s.texture.fragmentUnsignedRemap = unsignedRemap;
    s.texture.fragmentGamma = gamma;
    s.texture.fragmentSignedRemap = signedRemap;
}

void Rsx::TextureBorderColor(unsigned index, float a, float r, float g, float b) {
    TRACE(TextureBorderColor, index, a, r, g, b);
    _context->vertexTextureSamplers[index].borderColor = { a, r, g, b };
}

void Rsx::TextureFilter(unsigned index,
                        float bias,
                        uint8_t min,
                        uint8_t mag,
                        uint8_t conv,
                        uint8_t as,
                        uint8_t rs,
                        uint8_t gs,
                        uint8_t bs) {
    TRACE(TextureFilter, index, bias, min, mag, conv, as, rs, gs, bs);
    auto& s = _context->fragmentTextureSamplers[index];
    s.bias = bias;
    s.fragmentMin = min;
    s.fragmentMag = mag;
    s.fragmentConv = conv;
    s.texture.fragmentAs = as;
    s.texture.fragmentRs = rs;
    s.texture.fragmentGs = gs;
    s.texture.fragmentBs = bs;
}

void Rsx::TextureOffset(unsigned index,
                        uint32_t offset,
                        uint16_t mipmap,
                        GcmTextureFormat format,
                        GcmTextureLnUn lnUn,
                        uint8_t dimension,
                        bool border,
                        bool cubemap,
                        uint8_t location)
{
    TRACE(TextureOffset, index, offset, mipmap, format, lnUn, dimension, border, cubemap, location);
    _context->fragmentTextureSamplers[index].enable = true;
    auto& t = _context->fragmentTextureSamplers[index].texture;
    t.offset = offset;
    t.mipmap = mipmap;
    t.format = format;
    t.lnUn = lnUn;
    t.dimension = dimension == 1 ? 2 : dimension;
    t.fragmentBorder = border;
    t.fragmentCubemap = cubemap;
    t.location = gcmEnumToLocation(location);
}

void Rsx::TextureImageRect(unsigned int index, uint16_t width, uint16_t height) {
    TRACE(TextureImageRect, index, width, height);
    _context->fragmentTextureSamplers[index].texture.width = width;
    _context->fragmentTextureSamplers[index].texture.height = height;
}

void Rsx::TextureControl3(unsigned int index, uint16_t depth, uint32_t pitch) {
    TRACE(TextureControl3, index, depth, pitch);
    _context->fragmentTextureSamplers[index].texture.pitch = pitch;
    _context->fragmentTextureSamplers[index].texture.fragmentDepth = depth;
}

void Rsx::TextureControl1(unsigned int index, uint32_t remap) {
    TRACE(TextureControl1, index, remap);
    _context->fragmentTextureSamplers[index].texture.fragmentRemapCrossbarSelect = remap;
}

void Rsx::TextureControl2(unsigned int index, uint8_t slope, bool iso, bool aniso) {
    TRACE(TextureControl2, index, slope, iso, aniso);
    // ignore these optimizations
}

void Rsx::TextureControl0(unsigned index,
                          uint8_t alphaKill,
                          uint8_t maxaniso,
                          float maxlod,
                          float minlod,
                          bool enable) {
    TRACE(TextureControl0, index, alphaKill, maxaniso, maxlod, minlod, enable);
    // ignore maxaniso
    _context->fragmentTextureSamplers[index].fragmentAlphaKill = alphaKill;
    _context->fragmentTextureSamplers[index].enable = enable;
    _context->fragmentTextureSamplers[index].minlod = minlod;
    _context->fragmentTextureSamplers[index].maxlod = maxlod;
}

void Rsx::SetReference(uint32_t ref) {
    waitForIdle();
    *_ref = fast_endian_reverse(ref);
}

// Surface

void Rsx::SurfaceCompression(uint32_t x) {
    TRACE(SurfaceCompression, x);
}

void Rsx::setSurfaceColorLocation(unsigned index, uint32_t location) {
    TRACE(setSurfaceColorLocation, index, location);
    assert(index < 4);
    _context->surface.colorLocation[index] = gcmEnumToLocation(location);
}

void Rsx::SurfaceFormat(GcmSurfaceColor colorFormat,
                        SurfaceDepthFormat depthFormat,
                        uint8_t antialias,
                        uint8_t type,
                        uint8_t width,
                        uint8_t height,
                        uint32_t pitchA,
                        uint32_t offsetA,
                        uint32_t offsetZ,
                        uint32_t offsetB,
                        uint32_t pitchB) {
    TRACE(SurfaceFormat,
          colorFormat,
          depthFormat,
          antialias,
          type,
          width,
          height,
          pitchA,
          offsetA,
          offsetZ,
          offsetB,
          pitchB);
    //assert(colorFormat == CELL_GCM_SURFACE_A8R8G8B8);
    _context->surface.depthFormat = depthFormat;
    //assert(antialias == CELL_GCM_SURFACE_CENTER_1);
    assert(type == CELL_GCM_SURFACE_PITCH);
    _context->surface.colorFormat = colorFormat;
    _context->surface.width = 1 << (width + 1);
    _context->surface.height = 1 << (height + 1);
    _context->surface.colorPitch[0] = pitchA;
    _context->surface.colorPitch[1] = pitchB;
    _context->surface.colorOffset[0] = offsetA;
    _context->surface.colorOffset[1] = offsetB;
    _context->surface.depthOffset = offsetZ;
}

void Rsx::SurfacePitchZ(uint32_t pitch) {
    TRACE(SurfacePitchZ, pitch);
    _context->surface.depthPitch = pitch;
}

void Rsx::SurfacePitchC(uint32_t pitchC, uint32_t pitchD, uint32_t offsetC, uint32_t offsetD) {
    TRACE(SurfacePitchC, pitchC, pitchD, offsetC, offsetD);
    _context->surface.colorPitch[2] = pitchC;
    _context->surface.colorPitch[3] = pitchD;
    _context->surface.colorOffset[2] = offsetC;
    _context->surface.colorOffset[3] = offsetD;
}

void Rsx::SurfaceColorTarget(uint32_t target) {
    TRACE(SurfaceColorTarget, target);
    if (target == CELL_GCM_SURFACE_TARGET_NONE) {
        _context->surface.colorTarget = { 0, 0, 0, 0 };
    } else {
        _context->surface.colorTarget = {
            target != CELL_GCM_SURFACE_TARGET_1,
            target != CELL_GCM_SURFACE_TARGET_0,
            target == CELL_GCM_SURFACE_TARGET_MRT2 || target == CELL_GCM_SURFACE_TARGET_MRT3,
            target == CELL_GCM_SURFACE_TARGET_MRT3
        };
    }
}

void Rsx::WindowOffset(uint16_t x, uint16_t y) {
    TRACE(WindowOffset, x, y);
    _context->surface.windowOriginX = x;
    _context->surface.windowOriginY = y;
}

void Rsx::SurfaceClipHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    TRACE(SurfaceClipHorizontal, x, w, y, h);
    assert(x == 0);
    assert(y == 0);
    _context->surfaceClipWidth = w;
    _context->surfaceClipHeight = h;
}

// assuming cellGcmSetSurface is always used and not its subcommands
// then this command is always set last
void Rsx::ShaderWindow(uint16_t height, uint8_t origin, uint16_t pixelCenters) {
    TRACE(ShaderWindow, height, origin, pixelCenters);
    if (height == 0)
        return;
    assert(height == _context->surfaceClipHeight);
    assert(origin == CELL_GCM_WINDOW_ORIGIN_BOTTOM);
    assert(pixelCenters == CELL_GCM_WINDOW_PIXEL_CENTER_HALF);
    waitForIdle();
    _context->framebuffer->setSurface(
        _context->surface, _context->surfaceClipWidth, _context->surfaceClipHeight);
    updateViewPort();
}

void Rsx::Control0(uint32_t format) {
    TRACE(Control0, format);
    if (format & 0x00100000) {
        _context->surface.depthType =
            enum_cast<SurfaceDepthType>((format & ~0x00100000) >> 12);
    } else {
        INFO(rsx) << "unknown control0";
    }
}

void Rsx::ContextDmaColorA(uint32_t context) {
    TRACE(ContextDmaColorA, context);
    setSurfaceColorLocation(0, context);
}

void Rsx::ContextDmaColorB(uint32_t context) {
    TRACE(ContextDmaColorB, context);
    setSurfaceColorLocation(1, context);
}

void Rsx::ContextDmaColorC_2(uint32_t contextC, uint32_t contextD) {
    TRACE(ContextDmaColorC_2, contextC, contextD);
    setSurfaceColorLocation(2, contextC);
    setSurfaceColorLocation(3, contextD);
}

void Rsx::ContextDmaColorD(uint32_t context) {
    TRACE(ContextDmaColorD, context);
    setSurfaceColorLocation(3, context);
}

void Rsx::ContextDmaColorC_1(uint32_t contextC) {
    TRACE(ContextDmaColorC_1, contextC);
    setSurfaceColorLocation(2, contextC);
}

void Rsx::ContextDmaZeta(uint32_t context) {
    TRACE(ContextDmaZeta, context);
    _context->surface.depthLocation = gcmEnumToLocation(context);
}

void Rsx::setDisplayBuffer(uint8_t id, uint32_t offset, uint32_t height) {
    TRACE(setDisplayBuffer, id, offset, height);
    auto& buffer = _context->displayBuffers[id];
    buffer.offset = offset;
    buffer.width = height * 16 / 9;
    buffer.pitch = 4 * buffer.width;
    buffer.height = height;
}

inline void glDebugCallbackFunction(GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar *message,
            const void *userParam) {
    auto sourceStr = source == GL_DEBUG_SOURCE_API ? "Api"
                   : source == GL_DEBUG_SOURCE_WINDOW_SYSTEM ? "WindowSystem"
                   : source == GL_DEBUG_SOURCE_SHADER_COMPILER ? "ShaderCompiler"
                   : source == GL_DEBUG_SOURCE_THIRD_PARTY ? "ThirdParty"
                   : source == GL_DEBUG_SOURCE_APPLICATION ? "Application"
                   : "Other";
    INFO(rsx) << sformat("gl callback [source={}]: {}", sourceStr, message);
    if (severity == GL_DEBUG_SEVERITY_HIGH)
        exit(1);
}

void Rsx::initGcm() {
    INFO(rsx) << "initializing rsx";

    _window.init();

#ifdef GL_DEBUG_ENABLED
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(&glDebugCallbackFunction, nullptr);
#endif

    _context.reset(new RsxContext());
    _context->pipeline.bind();
    _context->pipeline.useDefaultShaders();

    if (_mode == RsxOperationMode::RunCapture) {
        _context->tracer.enable(true);
    }

    const auto ringSize = 10;

    _context->drawRingBuffer.reset(new RingBuffer(ringSize, {
        {VertexShaderConstantCount * sizeof(float) * 4, true},
        {sizeof(VertexShaderSamplerUniform), true},
        {sizeof(VertexShaderViewportUniform), true},
        {sizeof(FragmentShaderSamplerUniform), true},
        {FragmentProgramSize / 2, true}
    }));

    _context->localMemoryBuffer = GLPersistentCpuBuffer(256u << 20, true, false);
    _context->mainMemoryBuffer = GLPersistentCpuBuffer(256u << 20, true);
    _context->feedbackBuffer = GLPersistentCpuBuffer(48u << 20);

    g_state.mm->provideMemory(RsxFbBaseAddr,
                              GcmLocalMemorySize,
                              _context->localMemoryBuffer.mapped());

    _context->elementArrayIndexBuffer = GLPersistentCpuBuffer(10 * (1u << 20));

    _context->framebuffer.reset(new GLFramebuffer());
    _context->textureRenderer.reset(new TextureRenderer());
    _textureReader = new RsxTextureReader();
    _textureReader->init();
    //glDisableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);

    resetContext();

    boost::lock_guard<boost::mutex> lock(_initMutex);
    INFO(rsx) << "rsx initialized";
    _initialized = true;
    _initCv.notify_all();
}

void Rsx::shutdownGcm() {
    _context.reset();
    _window.shutdown();
}

void Rsx::updateViewPort() {
    auto f = _context->viewPort.zmax;
    auto n = _context->viewPort.zmin;

    if (n == 0 && f == 0) {
        f = 1;
    }

    auto glDepth = (n == 0 && f == 1) ? GL_ZERO_TO_ONE
                 : (n == -1 && f == 1) ? GL_NEGATIVE_ONE_TO_ONE
                 : 0;
    assert(glDepth);

    glClipControl(GL_UPPER_LEFT, glDepth);
    auto w = _context->viewPort.width;
    auto h = _context->viewPort.height;
    auto x = _context->viewPort.x;
    auto y = _context->viewPort.y;
    glViewport(x, _context->surfaceClipHeight - (y + h), w, h);
    glDepthRange(n, f);

    float s, b;
    if (glDepth == GL_NEGATIVE_ONE_TO_ONE) {
        s = (f - n) / 2;
        b = (n + f) / 2;
    } else {
        s = f - n;
        b = n;
    }

    glm::mat4 gl;
    gl[0] = glm::vec4(w / 2, 0, 0, 0);
    gl[1] = glm::vec4(0, h / 2, 0, 0);
    gl[2] = glm::vec4(0, 0, s, 0);
    gl[3] = glm::vec4(x + w / 2, y + h / 2, b, 1);

    glm::mat4 gcm;
    gcm[0] = glm::vec4(_context->viewPort.scale[0], 0, 0, 0);
    gcm[1] = glm::vec4(0, _context->viewPort.scale[1], 0, 0);
    gcm[2] = glm::vec4(0, 0, _context->viewPort.scale[2], 0);
    gcm[3] = glm::vec4(_context->viewPort.offset[0],
                       _context->viewPort.offset[1],
                       _context->viewPort.offset[2],
                       1);

    auto viewportUniform = (VertexShaderViewportUniform*)_context->drawRingBuffer->current(vertexViewportBuffer);
    viewportUniform->glInverseGcm = glm::inverse(gl) * gcm;
}

GLPersistentCpuBuffer* Rsx::getBuffer(MemoryLocation location) {
    return location == MemoryLocation::Local ? &_context->localMemoryBuffer
                                             : &_context->mainMemoryBuffer;
}

void Rsx::updateVertexDataArrays(unsigned first, unsigned count) {
    RangeCloser r(&_vdaCounter);
    auto uniform = (VertexShaderSamplerUniform*)_context->drawRingBuffer->current(vertexSamplersBuffer);
    for (auto i = 0u; i < _context->vertexDataArrays.size(); ++i) {
        auto& format = _context->vertexDataArrays[i];
        auto& input = _context->vertexInputs[i];

        uniform->enabledInputs[i][0] = input.rank > 0;

        if (input.rank == 0)
            continue;

        auto bufferOffset = format.offset + first * format.stride;
        auto buffer = getBuffer(format.location);

        uniform->inputBufferBases[i][0] = bufferOffset;
        uniform->inputBufferStrides[i][0] = format.stride;
        uniform->inputBufferOps[i][0] =
            _context->frequencyDividerOperation & (1u << i);
        uniform->inputBufferFrequencies[i][0] = format.frequency;
        uniform->inputBuffers[i][0] = buffer->gpuPointer();
        if (_mode != RsxOperationMode::Replay) {
            UpdateBufferCache(format.location, bufferOffset, count * format.stride);
        }
    }
}

void Rsx::waitForIdle() {
    TRACE(waitForIdle);
    RangeCloser r(&_waitingForIdleCounter);
    glFinish();
}

void Rsx::BackEndWriteSemaphoreRelease(uint32_t value) {
    INFO(rsx) << sformat("BackEndWriteSemaphoreRelease({:x})", value);
    waitForIdle();
    g_state.mm->store32(_context->semaphoreOffset + GcmLabelBaseOffset, value, g_state.granule);
}

void Rsx::SemaphoreOffset(uint32_t offset) {
    INFO(rsx) << sformat("SemaphoreOffset({:x})", offset);
    _context->semaphoreOffset = offset;
}

void Rsx::invalidateCaches(uint32_t va, uint32_t size) {
    _context->textureCache.invalidate(va, size);
}

uint32_t Rsx::getGet() {
    return fast_endian_reverse(*_get);
}

uint32_t Rsx::getPut() {
    return fast_endian_reverse(*_put);
}

uint32_t Rsx::getRet() {
    return _ret;
}

void Rsx::watchTextureCache() {
    RangeCloser r(&_textureCacheCounter);
    _context->textureCache.watch([&](auto va, auto size) {
        return g_state.mm->modificationMap()->marked(va, size);
    });
}

void Rsx::resetCacheWatch() {
    RangeCloser r(&_resetCacheCounter);
    _context->textureCache.watch([&](auto va, auto size) {
        g_state.mm->modificationMap()->reset(va, size);
        //assert(!g_state.mm->modificationMap()->marked(va, size));
        return false;
    });
}

void Rsx::OffsetDestin(uint32_t offset) {
    TRACE(OffsetDestin, offset);
    _context->surface2d.destOffset = offset;
}

void Rsx::ContextDmaImage(uint32_t location) {
    TRACE(ContextDmaImage, location);
    _context->swizzle2d.location = gcmEnumToLocation(location);
}

void colorFormat(RsxContext* context, uint32_t format, uint16_t pitch) {
    assert(format == CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 ||
           format == CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8 ||
           format == CELL_GCM_TRANSFER_SURFACE_FORMAT_Y32);
    context->surface2d.format = format == CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5
                   ? ScaleSettingsFormat::r5g6b5
                   : format == CELL_GCM_TRANSFER_SURFACE_FORMAT_Y32
                         ? ScaleSettingsFormat::y32
                         : ScaleSettingsFormat::a8r8g8b8;
    context->surface2d.pitch = pitch;
}

void Rsx::ColorFormat_3(uint32_t format, uint16_t pitch, uint32_t offset) {
    TRACE(ColorFormat_3, format, pitch, offset);
    colorFormat(_context.get(), format, pitch);
    _context->surface2d.destOffset = offset;
}

void Rsx::ColorFormat_2(uint32_t format, uint16_t pitch) {
    TRACE(ColorFormat_2, format, pitch);
    colorFormat(_context.get(), format, pitch);
}

void Rsx::Point(uint16_t pointX,
                uint16_t pointY,
                uint16_t outSizeX,
                uint16_t outSizeY,
                uint16_t inSizeX,
                uint16_t inSizeY)
{
    TRACE(Point, pointX, pointY, outSizeX, outSizeY, inSizeX, inSizeY);

    InlineSettings& i = _context->inline2d;
    i.pointX = pointX;
    i.pointY = pointY;
    i.destSizeX = outSizeX;
    i.destSizeY = outSizeY;
    i.srcSizeX = inSizeX;
    i.srcSizeY = inSizeY;
}

void Rsx::Color(uint32_t ptr, uint32_t count) {
    TRACE(Color, ptr, count);

    if (_mode == RsxOperationMode::Replay)
        return;

    assert(_context->inline2d.pointY == 0);
    assert(_context->inline2d.destSizeY == 1);
    assert(_context->inline2d.srcSizeY == 1);

    assert(_context->surface2d.format == ScaleSettingsFormat::y32);
    auto dest = rsxOffsetToEa(_context->surface2d.destLocation,
                              _context->surface2d.destOffset +
                              _context->inline2d.pointX * 4);

    assert(~(ptr & 0xfffff) > count && "mid page writes aren't supported");
    static std::vector<uint8_t> vec;
    vec.resize(count * 4);
    g_state.mm->readMemory(ptr, &vec[0], count * 4);
    g_state.mm->writeMemory(dest, &vec[0], count * 4);
    invalidateCaches(dest, count * 4);
}

void Rsx::ContextDmaImageDestin(uint32_t location) {
    TRACE(ContextDmaImageDestin, location);
    _context->surface2d.destLocation = gcmEnumToLocation(location);
}

void Rsx::OffsetIn_1(uint32_t offset) {
    TRACE(OffsetIn_1, offset);
    _context->copy2d.srcOffset = offset;
}

void Rsx::OffsetOut(uint32_t offset) {
    TRACE(OffsetOut, offset);
    _context->copy2d.destOffset = offset;
}

void Rsx::PitchIn(int32_t inPitch,
                  int32_t outPitch,
                  uint32_t lineLength,
                  uint32_t lineCount,
                  uint8_t inFormat,
                  uint8_t outFormat) {
    TRACE(PitchIn, inPitch, outPitch, lineLength, lineCount, inFormat, outFormat);

    CopySettings& c = _context->copy2d;
    c.srcPitch = inPitch;
    c.destPitch = outPitch;
    c.lineLength = lineLength;
    c.lineCount = lineCount;
    c.srcFormat = inFormat;
    c.destFormat = outFormat;
}

void Rsx::DmaBufferIn(uint32_t sourceLocation, uint32_t dstLocation) {
    TRACE(DmaBufferIn, sourceLocation, dstLocation);
    _context->copy2d.srcLocation = gcmEnumToLocation(sourceLocation);
    _context->copy2d.destLocation = gcmEnumToLocation(dstLocation);
}

void Rsx::startCopy2d() {
    CopySettings& c = _context->copy2d;
    auto sourceEa = rsxOffsetToEa(c.srcLocation, c.srcOffset);
    auto destEa = rsxOffsetToEa(c.destLocation, c.destOffset);

    assert(c.srcFormat == 1 || c.srcFormat == 2 || c.srcFormat == 4);
    assert(c.destFormat == 1 || c.destFormat == 2 || c.destFormat == 4);

    uint32_t srcLine = sourceEa;
    uint32_t destLine = destEa;
    for (auto line = 0u; line < c.lineCount; ++line) {
        uint8_t* src = g_state.mm->getMemoryPointer(srcLine, c.lineLength);
        uint8_t* dest = g_state.mm->getMemoryPointer(destLine, c.lineLength);
        auto srcLineEnd = src + c.lineLength;
        while (src < srcLineEnd) {
            *dest = *src;
            src += c.srcFormat;
            dest += c.destFormat;
        }
        srcLine += c.srcPitch;
        destLine += c.destPitch;
        invalidateCaches(destLine, c.lineLength);
    }
}

void Rsx::OffsetIn_9(uint32_t inOffset,
                     uint32_t outOffset,
                     int32_t inPitch,
                     int32_t outPitch,
                     uint32_t lineLength,
                     uint32_t lineCount,
                     uint8_t inFormat,
                     uint8_t outFormat,
                     uint32_t notify) {
    if (_mode == RsxOperationMode::RunCapture) {
        updateOffsetTableForReplay();
    }
    TRACE(OffsetIn_9,
          inOffset,
          outOffset,
          inPitch,
          outPitch,
          lineLength,
          lineCount,
          inFormat,
          outFormat,
          notify);
    assert(inFormat == 1 || inFormat == 2 || inFormat == 4);
    assert(outFormat == 1 || outFormat == 2 || outFormat == 4);
    assert(notify == 0);

    CopySettings& c = _context->copy2d;
    c.srcOffset = inOffset;
    c.destOffset = outOffset;
    c.srcPitch = inPitch;
    c.destPitch = outPitch;
    c.lineLength = lineLength;
    c.lineCount = lineCount;
    c.srcFormat = inFormat;
    c.destFormat = outFormat;

    startCopy2d();
}

void Rsx::BufferNotify(uint32_t notify) {
    TRACE(BufferNotify, notify);
    assert(notify == 0);
    startCopy2d();
}

void Rsx::Nv3089ContextDmaImage(uint32_t location) {
    TRACE(Nv3089ContextDmaImage, location);
    _context->scale2d.location = gcmEnumToLocation(location);
}

void Rsx::Nv3089ContextSurface(uint32_t surfaceType) {
    TRACE(Nv3089ContextSurface, surfaceType);
    assert(surfaceType == CELL_GCM_CONTEXT_SURFACE2D ||
           surfaceType == CELL_GCM_CONTEXT_SWIZZLE2D);
    _context->scale2d.type = surfaceType == CELL_GCM_CONTEXT_SURFACE2D
                                 ? ScaleSettingsSurfaceType::Linear
                                 : ScaleSettingsSurfaceType::Swizzle;
}

void Rsx::Nv3089SetColorConversion(uint32_t conv,
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
                                   float dtdy) {
    TRACE(Nv3089SetColorConversion, conv, fmt, op, x, y, w, h, outX, outY, outW, outH, dsdx, dtdy);
    assert(conv == CELL_GCM_TRANSFER_CONVERSION_TRUNCATE);
    assert(op == CELL_GCM_TRANSFER_OPERATION_SRCCOPY);
    assert(fmt == CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5 ||
           fmt == CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8);
    ScaleSettings& s = _context->scale2d;
    s.format = fmt == CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5
                   ? ScaleSettingsFormat::r5g6b5
                   : ScaleSettingsFormat::a8r8g8b8;
    s.clipX = x;
    s.clipY = y;
    s.clipW = w;
    s.clipH = h;
    s.outX = outX;
    s.outY = outY;
    s.outW = outW;
    s.outH = outH;
    s.dsdx = dsdx;
    s.dtdy = dtdy;
}

struct r6g6b5_t {
    uint32_t value;
    BIT_FIELD(r, 0, 5)
    BIT_FIELD(g, 5, 11)
    BIT_FIELD(b, 11, 16)
};

void Rsx::transferImage() {
    ScaleSettings& scale = _context->scale2d;
    SurfaceSettings& surface = _context->surface2d;
    SwizzleSettings& swizzle = _context->swizzle2d;

    bool isSwizzle = scale.type == ScaleSettingsSurfaceType::Swizzle;

    auto sourceEa = rsxOffsetToEa(scale.location, scale.offset);
    auto destEa = isSwizzle
                      ? rsxOffsetToEa(swizzle.location, swizzle.offset)
                      : rsxOffsetToEa(surface.destLocation, surface.destOffset);
    auto src = g_state.mm->getMemoryPointer(sourceEa, 1);
    auto dest = g_state.mm->getMemoryPointer(destEa, 1);

    if (scale.clipW == 1024 && scale.clipH == 720)
        return;

    FramebufferTextureKey key{sourceEa};
    auto res = _context->framebuffer->findTexture(key);
    if (res.texture) {
        auto it = br::find_if(_context->displayBuffers, [&](auto& buf) {
            return buf.offset == surface.destOffset;
        });
        if (it != end(_context->displayBuffers)) {
            _context->surfaceLinks.insert({sourceEa, destEa});
            return;
        }
    }

    if (br::find_if(_context->displayBuffers, [&](auto& buf) {
        return buf.offset <= surface.destOffset &&
               surface.destOffset < buf.offset + buf.pitch * buf.height;
    }) != end(_context->displayBuffers))
        return;

    if (!isSwizzle && surface.format == ScaleSettingsFormat::a8r8g8b8 &&
        scale.format == ScaleSettingsFormat::a8r8g8b8 &&
        surface.pitch == scale.pitch && scale.inH == scale.outH &&
        scale.clipX == 0 && scale.clipY == 0 && scale.outX == 0 && scale.outY == 0)
    {
        auto size = scale.pitch * scale.inH;
        if (scale.location == MemoryLocation::Local && surface.destLocation == MemoryLocation::Local) {
            glCopyNamedBufferSubData(_context->localMemoryBuffer.handle(),
                                     _context->localMemoryBuffer.handle(),
                                     scale.offset,
                                     surface.destOffset,
                                     size);
            waitForIdle();
        } else {
            memmove(dest, src, size);
        }
        invalidateCaches(destEa, 1 << 20);
        return;
    }

    auto sourcePixelSize = scale.format == ScaleSettingsFormat::r5g6b5 ? 2 : 4;
    auto destPixelFormat = isSwizzle ? swizzle.format : surface.format;
    auto destPixelSize = destPixelFormat == ScaleSettingsFormat::r5g6b5 ? 2 : 4;

    auto destX0 = std::max<int16_t>(scale.outX, scale.clipX);
    auto destXn = std::min<int16_t>(scale.outX + scale.outW, scale.clipX + scale.clipW);
    auto destY0 = std::max<int16_t>(scale.outY, scale.clipY);
    auto destYn = std::min<int16_t>(scale.outY + scale.outH, scale.clipY + scale.clipH);

    assert(destX0 >= 0);
    assert(destY0 >= 0);

    auto clipDiffX = scale.clipX > scale.outX ? scale.clipX - scale.outX : 0;
    auto clipDiffY = scale.clipY > scale.outY ? scale.clipY - scale.outY : 0;

    SwizzledTextureIterator swizzleIter(dest, swizzle.logWidth, swizzle.logHeight, destPixelSize);

    for (auto destY = destY0; destY < destYn; ++destY) {
        for (auto destX = destX0; destX < destXn; ++destX) {
            int srcX = clamp(scale.inX + (destX - destX0 + clipDiffX) * scale.dsdx,
                             .0f, scale.inW - 1);
            int srcY = clamp(scale.inY + (destY - destY0 + clipDiffY) * scale.dtdy,
                             .0f, scale.inH - 1);
            auto srcPixelPtr = src + srcY * scale.pitch + srcX * sourcePixelSize;
            auto destPixelPtr = dest +
                (isSwizzle ? swizzleIter.swizzleAddress(destX, destY, 0) * destPixelSize
                           : (destY * surface.pitch + destX * destPixelSize));
            uint32_t srcPixel = fast_endian_reverse(*(uint32_t*)srcPixelPtr);
            uint32_t destPixel = srcPixel;
            if (sourcePixelSize != destPixelSize) {
                if (sourcePixelSize == 2) {
                    r6g6b5_t in { srcPixel };
                    destPixel = (0xff << 24)
                              | (ext8(in.r()) << 16)
                              | (ext8(in.g()) << 8)
                              | ext8(in.b());
                } else {
                    r6g6b5_t out { 0 };
                    out.r_set(((srcPixel >> 16) & 0xff) * 32 / 255);
                    out.g_set(((srcPixel >> 8) & 0xff) * 64 / 255);
                    out.b_set((srcPixel & 0xff) * 32 / 255);
                    destPixel = out.value;
                }
            }
            if (destPixelSize == 2) {
                *(uint16_t*)destPixelPtr = fast_endian_reverse(uint16_t(destPixel >> 16));
            } else {
                *(uint32_t*)destPixelPtr = fast_endian_reverse(destPixel);
            }
        }
    }

    invalidateCaches(destEa, 1 << 20);
}

void Rsx::ImageInSize(uint16_t inW,
                      uint16_t inH,
                      uint16_t pitch,
                      uint8_t origin,
                      uint8_t interpolator,
                      uint32_t offset,
                      float inX,
                      float inY) {
    TRACE(ImageInSize, inW, inH, pitch, origin, interpolator, offset, inX, inY);

    if (_mode == RsxOperationMode::Replay)
        return;

    ScaleSettings& s = _context->scale2d;
    s.inW = inW;
    s.inH = inH;
    s.pitch = pitch;
    s.offset = offset;
    s.inX = inX;
    s.inY = inY;

    if (s.dsdx == 0 || s.dtdy == 0)
        return;

    if (s.inW == 0 || s.inH == 0)
        return;

    if (s.outW == 0 || s.outH == 0)
        return;

    transferImage();
}

void Rsx::Nv309eSetFormat(uint16_t format,
                          uint8_t width,
                          uint8_t height,
                          uint32_t offset) {
    TRACE(Nv309eSetFormat, format, width, height, offset);
    assert(format == CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 ||
           format == CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8);
    SwizzleSettings& s = _context->swizzle2d;
    s.format = format == CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5
                   ? ScaleSettingsFormat::r5g6b5
                   : ScaleSettingsFormat::a8r8g8b8;
    s.logWidth = width;
    s.logHeight = height;
    s.offset = offset;
}

void Rsx::BlendEnable(bool enable) {
    TRACE(BlendEnable, enable);
    _context->fragmentOps.blend = enable;
    glEnableb(GL_BLEND, enable);
}

GLenum gcmBlendFuncToOpengl(GcmBlendFunc func) {
#define X(x) case GcmBlendFunc:: x: return GL_##x;
    switch (func) {
        X(ZERO)
        X(ONE)
        X(SRC_COLOR)
        X(ONE_MINUS_SRC_COLOR)
        X(DST_COLOR)
        X(ONE_MINUS_DST_COLOR)
        X(SRC_ALPHA)
        X(ONE_MINUS_SRC_ALPHA)
        X(DST_ALPHA)
        X(ONE_MINUS_DST_ALPHA)
        X(SRC_ALPHA_SATURATE)
        X(CONSTANT_COLOR)
        X(ONE_MINUS_CONSTANT_COLOR)
        X(CONSTANT_ALPHA)
        X(ONE_MINUS_CONSTANT_ALPHA)
        default: assert(false); return 0;
    }
#undef X
}

void Rsx::BlendFuncSFactor(GcmBlendFunc sfcolor,
                           GcmBlendFunc sfalpha,
                           GcmBlendFunc dfcolor,
                           GcmBlendFunc dfalpha) {
    TRACE(BlendFuncSFactor, sfcolor, sfalpha, dfcolor, dfalpha);
    _context->fragmentOps.sfcolor = sfcolor;
    _context->fragmentOps.sfalpha = sfalpha;
    _context->fragmentOps.dfcolor = dfcolor;
    _context->fragmentOps.dfalpha = dfalpha;
    glBlendFuncSeparate(
        gcmBlendFuncToOpengl(sfcolor),
        gcmBlendFuncToOpengl(dfcolor),
        gcmBlendFuncToOpengl(sfalpha),
        gcmBlendFuncToOpengl(dfalpha)
    );
}

void Rsx::LogicOpEnable(bool enable) {
    TRACE(LogicOpEnable, enable);
    _context->fragmentOps.logic = enable;
    glEnableb(GL_COLOR_LOGIC_OP, enable);
}

GLenum gcmBlendEquationToOpengl(GcmBlendEquation eq) {
    switch (eq) {
        case GcmBlendEquation::FUNC_ADD: return GL_FUNC_ADD;
        case GcmBlendEquation::MIN: return GL_MIN;
        case GcmBlendEquation::MAX: return GL_MAX;
        case GcmBlendEquation::FUNC_SUBTRACT: return GL_FUNC_SUBTRACT;
        case GcmBlendEquation::FUNC_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
        default: throw std::runtime_error("unsupported blend equation");
    }
}

void Rsx::BlendEquation(GcmBlendEquation color, GcmBlendEquation alpha) {
    TRACE(BlendEquation, color, alpha);
    _context->fragmentOps.blendColor = color;
    _context->fragmentOps.blendAlpha = alpha;
    glBlendEquationSeparate(
        gcmBlendEquationToOpengl(color),
        gcmBlendEquationToOpengl(alpha));
}

void Rsx::ZStencilClearValue(uint32_t value) {
    TRACE(ZStencilClearValue, value);
    glClearStencil(value);
}

void Rsx::VertexData4fM(unsigned index, float x, float y, float z, float w) {
    TRACE(VertexData4fM, index, x, y, z, w);
    auto uniform = (VertexShaderSamplerUniform*)_context->drawRingBuffer->current(vertexSamplersBuffer);
    uniform->disabledInputValues[index] = { x, y, z, w };
}

GLenum gcmCullFaceToOpengl(GcmCullFace cfm) {
    switch (cfm) {
        case GcmCullFace::FRONT: return GL_BACK;
        case GcmCullFace::BACK: return GL_FRONT;
        case GcmCullFace::FRONT_AND_BACK: return GL_FRONT_AND_BACK;
    }
    throw std::runtime_error("unsupported cull face");
}

void Rsx::CullFace(GcmCullFace cfm) {
    TRACE(CullFace, cfm);
    _context->cullFace = cfm;
    glCullFace(gcmCullFaceToOpengl(cfm));
}

GLenum gcmPolygonModeToOpengl(uint32_t mode) {
    switch (mode) {
        case CELL_GCM_POLYGON_MODE_POINT: return GL_POINT;
        case CELL_GCM_POLYGON_MODE_LINE: return GL_LINE;
        case CELL_GCM_POLYGON_MODE_FILL: return GL_FILL;
    }
    throw std::runtime_error("unsupported polygon mode");
}

void Rsx::FrontPolygonMode(uint32_t mode) {
    TRACE(FrontPolygonMode, mode);
    glPolygonMode(GL_FRONT, gcmPolygonModeToOpengl(mode));
}

void Rsx::BackPolygonMode(uint32_t mode) {
    TRACE(BackPolygonMode, mode);
    glPolygonMode(GL_BACK, gcmPolygonModeToOpengl(mode));
}

void Rsx::StencilTestEnable(bool enable) {
    TRACE(StencilTestEnable, enable);
    glEnableb(GL_STENCIL_TEST, enable);
}

void Rsx::StencilMask(uint32_t sm) {
    TRACE(StencilMask, sm);
    glStencilMask(sm);
}

void Rsx::StencilFunc(GcmOperator func, int32_t ref, uint32_t mask) {
    TRACE(StencilFunc, func, ref, mask);
    _context->fragmentOps.stencilFunc = func;
    glStencilFunc(gcmOperatorToOpengl(func), ref, mask);
}

GLenum gcmStencilOpToOpengl(uint32_t mode) {
#define X(x) case CELL_GCM_##x: return GL_##x;
    switch (mode) {
        X(KEEP)
        X(ZERO)
        X(REPLACE)
        X(INCR)
        X(INCR_WRAP)
        X(DECR)
        X(DECR_WRAP)
        X(INVERT)
        default: throw std::runtime_error("bad stencil op");
    }
#undef X
}

void Rsx::StencilOpFail(uint32_t fail, uint32_t depthFail, uint32_t depthPass) {
    TRACE(StencilOpFail, fail, depthFail, depthPass);
    glStencilOp(gcmStencilOpToOpengl(fail),
                gcmStencilOpToOpengl(depthFail),
                gcmStencilOpToOpengl(depthPass));
}

void Rsx::ContextDmaReport(uint32_t handle) {
    TRACE(ContextDmaReport, handle);
    _context->reportLocation = handle == CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN
        ? MemoryLocation::Main : MemoryLocation::Local;
    waitForIdle();
}

void Rsx::GetReport(uint8_t type, uint32_t offset) {
    TRACE(GetReport, type, offset);

    if (_mode == RsxOperationMode::Replay)
        return;

    // TODO: report zpass/zcull
    auto ea = getReportDataAddressLocation(offset / 16, _context->reportLocation);
    g_state.mm->store64(ea, g_state.proc->getTimeBaseNanoseconds().count(), g_state.granule);
    g_state.mm->store64(ea + 8 + 4, 0, g_state.granule);
}

void Rsx::updateScissor() {
//    auto w = _context->surface.scissor.width;
//    auto h = _context->surface.scissor.height;
//    auto x = _context->surface.scissor.x;
//    auto y = _context->surface.scissor.y;
//    glEnableb(GL_SCISSOR_TEST, w != 4096 || h != 4096);
//    glScissor(x, _context->surfaceClipHeight - (y + h), w, h);
}

void Rsx::ScissorHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    TRACE(ScissorHorizontal, x, w, y, h);
    _context->surface.scissor.x = x;
    _context->surface.scissor.width = w;
    _context->surface.scissor.y = y;
    _context->surface.scissor.height = h;
}

void Rsx::TransformProgramStart(uint32_t startSlot) {
    TRACE(TransformProgramStart, startSlot);
    assert(startSlot == 0);
}

void Rsx::LineWidth(float width) {
    TRACE(LineWidth, width);
    glLineWidth(width);
}

void Rsx::LineSmoothEnable(bool enable) {
    TRACE(LineSmoothEnable, enable);
    glEnableb(GL_LINE_SMOOTH, enable);
}

GLenum gcmLogicOpToOpengl(GcmLogicOp op) {
#define X(x) case GcmLogicOp:: x: return GL_##x;
    switch (op) {
        X(CLEAR)
        X(AND)
        X(AND_REVERSE)
        X(COPY)
        X(AND_INVERTED)
        X(NOOP)
        X(XOR)
        X(OR)
        X(NOR)
        X(EQUIV)
        X(INVERT)
        X(OR_REVERSE)
        X(COPY_INVERTED)
        X(OR_INVERTED)
        X(NAND)
        X(SET)
        default: throw std::runtime_error("bad logic op");
    }
#undef X
}

void Rsx::LogicOp(GcmLogicOp op) {
    TRACE(LogicOp, op);
    _context->fragmentOps.logicOp = op;
    glLogicOp(gcmLogicOpToOpengl(op));
}

void Rsx::PolySmoothEnable(bool enable) {
    TRACE(PolySmoothEnable, enable);
    glEnableb(GL_POLYGON_SMOOTH, enable);
}

void Rsx::PolyOffsetLineEnable(bool enable) {
    TRACE(PolyOffsetLineEnable, enable);
    glEnableb(GL_POLYGON_OFFSET_LINE, enable);
}

void Rsx::PolyOffsetFillEnable(bool enable) {
    TRACE(PolyOffsetFillEnable, enable);
    glEnableb(GL_POLYGON_OFFSET_FILL, enable);
}

void Rsx::setOperationMode(RsxOperationMode mode) {
    _mode = mode;
}

RsxOperationMode Rsx::_mode = RsxOperationMode::Run;

void Rsx::UpdateBufferCache(MemoryLocation location, uint32_t offset, uint32_t size) {
    if (_mode == RsxOperationMode::Run)
        return;

    auto ea = rsxOffsetToEa(location, offset);
    if (_mode == RsxOperationMode::RunCapture) {
        updateOffsetTableForReplay();
        std::vector<uint8_t> vec(size);
        g_state.mm->readMemory(ea, &vec[0], size);
        _context->tracer.pushBlob(&vec[0], vec.size());
        TRACE(UpdateBufferCache, location, offset, size);
    } else if (_mode == RsxOperationMode::Replay) {
        assert(size == _currentReplayBlob.size());
        g_state.mm->writeMemory(ea, &_currentReplayBlob[0], size);
    }
}

RsxContext* Rsx::context() {
    assert(_context);
    return _context.get();
}

void Rsx::updateOffsetTableForReplay() {
    if (_mode == RsxOperationMode::Run)
        return;

    if (_mode == RsxOperationMode::RunCapture) {
        auto table = serializeOffsetTable();
        _context->tracer.pushBlob(&table[0], table.size() * 2);
        TRACE(updateOffsetTableForReplay);
    } else {
        std::vector<uint16_t> vec(_currentReplayBlob.size() / 2);
        memcpy(&vec[0], &_currentReplayBlob[0], _currentReplayBlob.size());
        deserializeOffsetTable(vec);
    }
}

uint32_t getReportDataAddressLocation(uint32_t index, MemoryLocation location) {
    if (location == MemoryLocation::Local) {
        assert(index < 2048);
    } else {
        assert(index < 1024 * 1024);
    }
    auto offset = 0x0e000000 + 16 * index;
    return rsxOffsetToEa(location, offset);
}

void Rsx::captureFrames() {
    _frameCapturePending = true;
    _shortTrace = true;
}

void Rsx::DriverInterrupt(uint32_t cause) {
    invokeHandler(RSX_HANDLER_USER, cause);
}

void Rsx::PointSize(float size) {
    TRACE(PointSize, size);
    _context->pointSize = size;
    glPointSize(size);
}

void Rsx::PointParamsEnable(bool enable) {
    TRACE(PointParamsEnable, enable);
    _context->pointSpriteControl.enabled = enable;
}

void Rsx::PointSpriteControl(bool enable, uint16_t rmode, PointSpriteTex tex) {
    TRACE(PointSpriteControl, enable, rmode, tex);
    _context->pointSpriteControl.enabled = enable;
    _context->pointSpriteControl.rmode = rmode;
    _context->pointSpriteControl.tex = tex;
}

RsxContext::RsxContext() : fragmentBytecode(FragmentProgramSize), statText(14) {}
