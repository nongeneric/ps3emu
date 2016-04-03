#include "Rsx.h"

#include "GLBuffer.h"
#include "Cache.h"
#include "GLFramebuffer.h"
#include "GLTexture.h"
#include "../Process.h"
#include "../ppu/CallbackThread.h"
#include "../MainMemory.h"
#include "../ELFLoader.h"
#include "../shaders/ShaderGenerator.h"
#include "../shaders/shader_dasm.h"
#include "../utils.h"
#include <atomic>
#include <vector>
#include <fstream>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/clamp.hpp>
#include "../../libs/graphics/graphics.h"

#include "FragmentShaderUpdateFunctor.h"
#include "RsxContext.h"
#include "TextureRenderer.h"
#include "GLShader.h"
#include "GLProgramPipeline.h"
#include "GLSampler.h"
#include "Tracer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace boost::algorithm;

typedef std::array<float, 4> glvec4_t;

Rsx::Rsx() = default;

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

void Rsx::setLabel(int index, uint32_t value) {
    if (_mode == RsxOperationMode::Replay)
        return;
    auto offset = index * 0x10;
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("setting rsx label at offset %x", offset);
    auto ptr = _mm->getMemoryPointer(GcmLabelBaseOffset + offset, sizeof(uint32_t));
    auto atomic = (std::atomic<uint32_t>*)ptr;
    atomic->store(boost::endian::native_to_big(value));
}

void Rsx::ChannelSetContextDmaSemaphore(uint32_t handle) {
    _activeSemaphoreHandle = handle;
}

void Rsx::ChannelSemaphoreOffset(uint32_t offset) {
    _semaphores[_activeSemaphoreHandle] = offset;
}

void Rsx::ChannelSemaphoreAcquire(uint32_t value) {
    auto offset = _semaphores[_activeSemaphoreHandle];
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("acquiring semaphore %x at offset %x with value %x",
        _activeSemaphoreHandle, offset, value
    );
    auto ptr = _mm->getMemoryPointer(GcmLabelBaseOffset + offset, sizeof(uint32_t));
    auto atomic = (std::atomic<uint32_t>*)ptr;
    while (boost::endian::big_to_native(atomic->load()) != value) ;
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("acquired");
}

void Rsx::SemaphoreRelease(uint32_t value) {
    auto offset = _semaphores[_activeSemaphoreHandle];
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("releasing semaphore %x at offset %x with value %x",
        _activeSemaphoreHandle, offset, value
    );
    auto ptr = _mm->getMemoryPointer(GcmLabelBaseOffset + offset, sizeof(uint32_t));
    auto atomic = (std::atomic<uint32_t>*)ptr;
    atomic->store(boost::endian::native_to_big(value));
}

void Rsx::ClearRectHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    TRACE4(ClearRectHorizontal, x, w, y, h);
    //TODO: implement
}

void Rsx::ClipIdTestEnable(uint32_t x) {
    TRACE1(ClipIdTestEnable, x);
    //TODO: implement
}

void Rsx::FlatShadeOp(uint32_t x) {
    TRACE1(FlatShadeOp, x);
    //TODO: implement
}

void Rsx::VertexAttribOutputMask(uint32_t mask) {
    TRACE1(VertexAttribOutputMask, mask);
    //TODO: implement
}

void Rsx::FrequencyDividerOperation(uint16_t op) {
    TRACE1(FrequencyDividerOperation, op);
    //TODO: implement
}

void Rsx::TexCoordControl(unsigned int index, uint32_t control) {
    TRACE2(TexCoordControl, index, control);
    //TODO: implement
}

void Rsx::ReduceDstColor(bool enable) {
    TRACE1(ReduceDstColor, enable);
    //TODO: implement
}

void Rsx::FogMode(uint32_t mode) {
    TRACE1(FogMode, mode);
    //TODO: implement
}

void Rsx::AnisoSpread(unsigned int index,
                      bool reduceSamplesEnable,
                      bool hReduceSamplesEnable,
                      bool vReduceSamplesEnable,
                      uint8_t spacingSelect,
                      uint8_t hSpacingSelect,
                      uint8_t vSpacingSelect) {
    TRACE7(AnisoSpread, 
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
    TRACE2(VertexDataBaseOffset, baseOffset, baseIndex);
    //TODO: implement
}

void Rsx::AlphaFunc(uint32_t af, uint32_t ref) {
    TRACE2(AlphaFunc, af, ref);
    auto glFunc = af == CELL_GCM_NEVER ? GL_NEVER
                : af == CELL_GCM_LESS ? GL_LESS
                : af == CELL_GCM_EQUAL ? GL_EQUAL
                : af == CELL_GCM_LEQUAL ? GL_LEQUAL
                : af == CELL_GCM_GREATER ? GL_GREATER
                : af == CELL_GCM_NOTEQUAL ? GL_NOTEQUAL
                : af == CELL_GCM_GEQUAL ? GL_GEQUAL
                : af == CELL_GCM_ALWAYS ? GL_ALWAYS
                : 0;
    glcall(glAlphaFunc(glFunc, ref));
}

void Rsx::AlphaTestEnable(bool enable) {
    TRACE1(AlphaTestEnable, enable);
    glEnableb(GL_ALPHA_TEST, enable);
}

void Rsx::ShaderControl(uint32_t control, uint8_t registerCount) {
    TRACE2(ShaderControl, control, registerCount);
    //TODO: implement
}

void Rsx::TransformProgramLoad(uint32_t load, uint32_t start) {
    TRACE2(TransformProgramLoad, load, start);
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
        source = _mm->getMemoryPointer(rsxOffsetToEa(MemoryLocation::Main, locationOffset), bytes);
    }
    
    _context->tracer.pushBlob(source, bytes);
    TRACE2(TransformProgram, locationOffset, size);
    
    memcpy(&_context->vertexInstructions[_context->vertexLoadOffset], source, bytes);
    _context->vertexShaderDirty = true;
    _context->vertexLoadOffset += bytes;
}

void Rsx::VertexAttribInputMask(uint16_t mask) {
    TRACE1(VertexAttribInputMask, mask);
    for (auto i = 0u; i < 16; ++i) {
        if (!(mask & 1)) {
            glDisableVertexAttribArray(i);
            glVertexAttrib4f(i, 0, 0, 0, 1);
        }
        mask >>= 1;
    }
}

void Rsx::TransformTimeout(uint16_t count, uint16_t registerCount) {
    TRACE2(TransformTimeout, count, registerCount);
}

void Rsx::ShaderProgram(uint32_t offset, uint32_t location) {
    TRACE2(ShaderProgram, offset, location);
    auto ea = rsxOffsetToEa(gcmEnumToLocation(location), offset);
    _context->fragmentVa = ea;
    _context->fragmentShaderDirty = true;
}

void Rsx::ViewportHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    TRACE4(ViewportHorizontal, x, w, y, h);
    _context->viewPort.x = x;
    _context->viewPort.y = y;
    _context->viewPort.width = w;
    _context->viewPort.height = h;
}

void Rsx::ClipMin(float min, float max) {
    TRACE2(ClipMin, min, max);
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
    TRACE8(ViewportOffset,
           offset0,
           offset1,
           offset2,
           offset3,
           scale0,
           scale1,
           scale2,
           scale3);
    assert(offset3 == 0);
    assert(scale3 == 0);
    _context->viewPort.offset[0] = offset0;
    _context->viewPort.offset[1] = offset1;
    _context->viewPort.offset[2] = offset2;
    _context->viewPort.scale[0] = scale0;
    _context->viewPort.scale[1] = scale1;
    _context->viewPort.scale[2] = scale2;
    updateViewPort();
}

void Rsx::ColorMask(uint32_t mask) {
    TRACE1(ColorMask, mask);
    _context->colorMask = mask;
    glcall(glColorMask(
        (mask & CELL_GCM_COLOR_MASK_R) ? GL_TRUE : GL_FALSE,
        (mask & CELL_GCM_COLOR_MASK_G) ? GL_TRUE : GL_FALSE,
        (mask & CELL_GCM_COLOR_MASK_B) ? GL_TRUE : GL_FALSE,
        (mask & CELL_GCM_COLOR_MASK_A) ? GL_TRUE : GL_FALSE
    ));
}

void Rsx::DepthTestEnable(bool enable) {
    TRACE1(DepthTestEnable, enable);
    _context->isDepthTestEnabled = enable;
    glEnableb(GL_DEPTH_TEST, enable);
}

void Rsx::DepthFunc(uint32_t zf) {
    TRACE1(DepthFunc, zf);
    _context->depthFunc = zf;
    auto glfunc = zf == CELL_GCM_NEVER ? GL_NEVER
                : zf == CELL_GCM_LESS ? GL_LESS
                : zf == CELL_GCM_EQUAL ? GL_EQUAL
                : zf == CELL_GCM_LEQUAL ? GL_LEQUAL
                : zf == CELL_GCM_GREATER ? GL_GREATER
                : zf == CELL_GCM_NOTEQUAL ? GL_NOTEQUAL
                : zf == CELL_GCM_GEQUAL ? GL_GEQUAL
                : zf == CELL_GCM_ALWAYS ? GL_ALWAYS
                : 0;
    glcall(glDepthFunc(glfunc));
}

void Rsx::CullFaceEnable(bool enable) {
    TRACE1(CullFaceEnable, enable);
    _context->isCullFaceEnabled = enable;
    glEnableb(GL_CULL_FACE, enable);
}

void Rsx::ShadeMode(uint32_t sm) {
    TRACE1(ShadeMode, sm);
    _context->isFlatShadeMode = sm == CELL_GCM_FLAT;
    _context->fragmentShaderDirty = true;
    assert(sm == CELL_GCM_SMOOTH || sm == CELL_GCM_FLAT);
}

void Rsx::ColorClearValue(uint32_t color) {
    TRACE1(ColorClearValue, color);
    union {
        uint32_t val;
        BitField<0, 8> a;
        BitField<8, 16> r;
        BitField<16, 24> g;
        BitField<24, 32> b;
    } c = { color };
    _context->colorClearValue = {
        c.r.u() / 255.,
        c.g.u() / 255.,
        c.b.u() / 255.,
        c.a.u() / 255.
    };
}

void Rsx::ClearSurface(uint32_t mask) {
    TRACE1(ClearSurface, mask);
    assert(mask & CELL_GCM_CLEAR_R);
    assert(mask & CELL_GCM_CLEAR_G);
    assert(mask & CELL_GCM_CLEAR_B);
    assert(mask & CELL_GCM_CLEAR_A);
    auto glmask = GL_COLOR_BUFFER_BIT;
    if (mask & CELL_GCM_CLEAR_Z)
        glmask |= GL_DEPTH_BUFFER_BIT;
    if (mask & CELL_GCM_CLEAR_S)
        glmask |= GL_STENCIL_BUFFER_BIT;
    _context->glClearSurfaceMask = glmask;
    auto& c = _context->colorClearValue;
    glcall(glClearColor(c.r, c.g, c.b, c.a));
    glcall(glClear(glmask));
}

void Rsx::VertexDataArrayFormat(uint8_t index,
                                uint16_t frequency,
                                uint8_t stride,
                                uint8_t size,
                                uint8_t type) {
    TRACE5(VertexDataArrayFormat, index, frequency, stride, size, type);
    auto& format = _context->vertexDataArrays[index];
    format.frequency = frequency;
    format.stride = stride;
    format.size = size;
    format.type = type;
    assert(type == CELL_GCM_VERTEX_UB || type == CELL_GCM_VERTEX_F);
    auto gltype = type == CELL_GCM_VERTEX_UB ? GL_UNSIGNED_BYTE
                : type == CELL_GCM_VERTEX_F ? GL_FLOAT
                : 0;
    auto normalize = gltype == GL_UNSIGNED_BYTE;
    auto& vinput = _context->vertexInputs[index];
    vinput.typeSize = gltype == GL_FLOAT ? 4 : 1; // TODO: other types
    vinput.rank = size;
    vinput.enabled = size != 0;
    if (vinput.enabled) {
        glcall(glEnableVertexAttribArray(index));
        glcall(glVertexAttribFormat(index, size, gltype, normalize, 0));
    } else {
        glcall(glDisableVertexAttribArray(index));
    }
}

void Rsx::VertexDataArrayOffset(unsigned index, uint8_t location, uint32_t offset) {
    TRACE3(VertexDataArrayOffset, index, location, offset);
    auto& array = _context->vertexDataArrays[index];
    array.location = gcmEnumToLocation(location);
    array.offset = offset;
    array.binding = index;
    GLint maxBindings;
    (void)maxBindings;
    assert((GLint)array.binding < (glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &maxBindings), maxBindings));
}

void Rsx::BeginEnd(uint32_t mode) {
    TRACE1(BeginEnd, mode);
    _context->glVertexArrayMode = 
        mode == CELL_GCM_PRIMITIVE_QUADS ? GL_QUADS :
        mode == CELL_GCM_PRIMITIVE_QUAD_STRIP ? GL_QUAD_STRIP :
        mode == CELL_GCM_PRIMITIVE_POLYGON ? GL_POLYGON :
        mode == CELL_GCM_PRIMITIVE_POINTS ? GL_POINTS :
        mode == CELL_GCM_PRIMITIVE_LINES ? GL_LINES :
        mode == CELL_GCM_PRIMITIVE_LINE_LOOP ? GL_LINE_LOOP :
        mode == CELL_GCM_PRIMITIVE_LINE_STRIP ? GL_LINE_STRIP :
        mode == CELL_GCM_PRIMITIVE_TRIANGLES ? GL_TRIANGLES :
        mode == CELL_GCM_PRIMITIVE_TRIANGLE_STRIP ? GL_TRIANGLE_STRIP :
        GL_TRIANGLE_FAN;
}

void Rsx::DrawArrays(unsigned first, unsigned count) {
    updateVertexDataArrays(first, count);
    updateShaders();
    updateTextures();
    watchCaches();
    linkShaderProgram();
    glcall(glDrawArrays(_context->glVertexArrayMode, 0, count));
    
    // Right after a gcm draw command completes, the user might immediately
    // update buffers or shader constants. OpenGL draw commands are asynchronous
    // and as such need to be waited.
    // OpenGL guarantees that all buffers are immediately available to change
    // after a draw command, but this isn't true for persistent buffers.
    // It is possible to use a finer grained synchronization, only waiting inside
    // gcm commands that might modify buffers/constants and possibly textures, but
    // for now a less error-prone approach with glFinish is used.
    glFinish();
    
    TRACE2(DrawArrays, first, count);
}

bool Rsx::isFlipInProgress() const {
    return _isFlipInProgress;
}

void Rsx::resetFlipStatus() {
    _isFlipInProgress = true;
}

struct __attribute__ ((__packed__)) VertexShaderSamplerUniform {
    std::array<uint32_t, 4> wraps[4];
    std::array<float, 4> borderColor[4];
};

struct __attribute__ ((__packed__)) FragmentShaderSamplerUniform {
    uint32_t flip[16];
};

struct VertexShaderViewportUniform {
    glm::mat4 glInverseGcm;
};

static_assert(sizeof(VertexShaderViewportUniform) == 4 * 16, "must be packed");

void Rsx::setGcmContext(uint32_t ioSize, ps3_uintptr_t ioAddress) {
    _gcmIoSize = ioSize;
    _gcmIoAddress = ioAddress;
}

void Rsx::EmuFlip(uint32_t buffer, uint32_t label, uint32_t labelValue) {
    TRACE3(EmuFlip, buffer, label, labelValue);
    auto va = _context->displayBuffers[buffer].offset + RsxFbBaseAddr;
    auto tex = _context->framebuffer->findTexture(va);
    _context->framebuffer->bindDefault();
    _context->textureRenderer->render(tex);
    _context->pipeline.bind();
    _window.swapBuffers();
    _context->frame++;
    _context->commandNum = 0;
    
#if TESTS
    static int framenum = 0;
    if (framenum < 10 && _mode != RsxOperationMode::Replay) {
        auto& vec = _context->lastFrame;
        vec.resize(_window.width() * _window.height() * 3);
        glGetTextureImage(tex->handle(), 
                          0,
                          GL_RGB,
                          GL_UNSIGNED_BYTE, 
                          vec.size(), 
                          &vec[0]);
        std::ofstream f(ssnprintf("/tmp/ps3frame%d.rgb", framenum));
        assert(f.is_open());
        f.write((const char*)vec.data(), vec.size());
        framenum++;
    }
#endif
    
    if (_context->vBlankHandlerDescr) {
        fdescr descr;
        _mm->readMemory(_context->vBlankHandlerDescr, &descr, sizeof(descr));
        auto future = _proc->getCallbackThread()->schedule({1}, descr.tocBase, descr.va);
        future.get();
    }
    
    this->setLabel(1, 0);
    if (label != (uint32_t)-1) {
        this->setLabel(label, labelValue);
    }
    
    _isFlipInProgress = false;
    
    // PlatformDevice.cpp of jsgcm says so
    _context->reportLocation = MemoryLocation::Local;
}

void Rsx::TransformConstantLoad(uint32_t loadAt, uint32_t offset, uint32_t count) {
    assert(count % 4 == 0);
    auto size = count * sizeof(uint32_t);
    
    std::vector<uint8_t> source;
    if (_mode == RsxOperationMode::Replay) {
        source = _currentReplayBlob;
    } else {
        source.resize(size);
        _mm->readMemory(
            rsxOffsetToEa(MemoryLocation::Main, offset), &source[0], source.size());
    }
    
    _context->tracer.pushBlob(&source[0], source.size());
    TRACE3(TransformConstantLoad, loadAt, offset, count);
    
    for (auto i = 0u; i < count; ++i) {
        auto u = (uint32_t*)&source[i * 4];
        boost::endian::endian_reverse_inplace(*u);
    }
    
    auto mapped = (uint8_t*)_context->vertexConstBuffer.mapped() + loadAt * 16;
    memcpy(mapped, source.data(), size);
}

bool Rsx::linkShaderProgram() {
    if (!_context->fragmentShader || !_context->vertexShader)
        return false;
    
    _context->pipeline.useShader(*_context->vertexShader);
    _context->pipeline.useShader(*_context->fragmentShader);
    _context->pipeline.validate();
    
    glcall(glBindBufferBase(GL_UNIFORM_BUFFER,
                            VertexShaderConstantBinding,
                            _context->vertexConstBuffer.handle()));
    glcall(glBindBufferBase(GL_UNIFORM_BUFFER,
                            VertexShaderSamplesInfoBinding,
                            _context->vertexSamplersBuffer.handle()));
    glcall(glBindBufferBase(GL_UNIFORM_BUFFER,
                            VertexShaderViewportMatrixBinding,
                            _context->vertexViewportBuffer.handle()));
    glcall(glBindBufferBase(GL_UNIFORM_BUFFER,
                            FragmentShaderSamplesInfoBinding,
                            _context->fragmentSamplersBuffer.handle()));
    return true;
}

void Rsx::RestartIndexEnable(bool enable) {
    TRACE1(RestartIndexEnable, enable);
    glEnableb(GL_PRIMITIVE_RESTART, enable);
}

void Rsx::RestartIndex(uint32_t index) {
    TRACE1(RestartIndex, index);
    glcall(glPrimitiveRestartIndex(index));
}

void Rsx::IndexArrayAddress(uint8_t location, uint32_t offset, uint32_t type) {
    TRACE3(IndexArrayAddress, location, offset, type);
    _context->indexArray.location = gcmEnumToLocation(location);
    _context->indexArray.offset = offset;
    _context->indexArray.glType = GL_UNSIGNED_INT; // no difference between 32 and 16 for rsx
}

void Rsx::DrawIndexArray(uint32_t first, uint32_t count) {
    assert(first == 0);
    auto destBuffer = &_context->elementArrayIndexBuffer;
    auto sourceBuffer = getBuffer(_context->indexArray.location);
    
    assert(count * 4 <= destBuffer->size());
    
    auto source = (big_uint32_t*)((uintptr_t)sourceBuffer->mapped() +
                                  _context->indexArray.offset);
    auto dest = (uint32_t*)destBuffer->mapped();
    for (auto i = first; i < count; ++i) {
        dest[i] = source[i];
    }
    
    if (_mode != RsxOperationMode::Replay) {
        UpdateBufferCache(_context->indexArray.location,
                        _context->indexArray.offset + first * 4,
                        count * 4);
    }

    updateVertexDataArrays(first, count);
    updateTextures();
    updateShaders();
    watchCaches();
    linkShaderProgram();
    
    glcall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destBuffer->handle()));
    
    auto offset = (void*)(uintptr_t)(first * 4);
    glcall(glDrawElements(_context->glVertexArrayMode, count, _context->indexArray.glType, offset));
    
    // see DrawArrays for rationale
    glFinish();
    
    TRACE2(DrawIndexArray, first, count);
}

FragmentShader* Rsx::addFragmentShaderToCache(uint32_t va, uint32_t size) {
    TRACE2(addFragmentShaderToCache, va, size);
    FragmentShaderCacheKey key { va, size };
    auto shader = new FragmentShader();
    auto updater = new FragmentShaderUpdateFunctor(
        _context->fragmentVa,
        size,
        _context.get(),
        _mm
    );
    _context->fragmentShaderCache.insert(key, shader, updater);
    return shader;
}

FragmentShader* Rsx::getFragmentShaderFromCache(uint32_t va, uint32_t size) {
    FragmentShaderCacheKey key { va, size };
    FragmentShaderUpdateFunctor* updater;
    FragmentShader* shader;
    std::tie(shader, updater) = _context->fragmentShaderCache.retrieveWithUpdater(key);
    if (!shader) {
        shader = addFragmentShaderToCache(va, size);
        std::tie(shader, updater) = _context->fragmentShaderCache.retrieveWithUpdater(key);
    }
    updater->bindConstBuffer();
    return shader;
}

void Rsx::updateShaders() {    
    if (_context->fragmentShaderDirty) {
        _context->fragmentShaderDirty = false;
        _context->fragmentShader = getFragmentShaderFromCache(_context->fragmentVa, FragmentProgramSize);
    }
    
    if (_context->vertexShaderDirty) {
        _context->vertexShaderDirty = false;
        std::array<int, 4> samplerSizes = { 
            _context->vertexTextureSamplers[0].texture.dimension,
            _context->vertexTextureSamplers[1].texture.dimension,
            _context->vertexTextureSamplers[2].texture.dimension,
            _context->vertexTextureSamplers[3].texture.dimension
        };
        
        auto size = CalcVertexBytecodeSize(_context->vertexInstructions.data());
        
        std::array<uint8_t, 16> arraySizes;
        assert(_context->vertexInputs.size() == arraySizes.size());
        for (auto i = 0u; i < arraySizes.size(); ++i) {
            arraySizes[i] = _context->vertexInputs[i].rank;
        }
        VertexShaderCacheKey key{
            std::vector<uint8_t>(_context->vertexInstructions.data(),
                                 _context->vertexInstructions.data() + size),
            arraySizes};
        auto shader = _context->vertexShaderCache.retrieve(key);
        if (!shader) {
            auto text = GenerateVertexShader(_context->vertexInstructions.data(),
                                             _context->vertexInputs,
                                             samplerSizes,
                                             0); // TODO: loadAt
            shader = new VertexShader(text.c_str());
            BOOST_LOG_TRIVIAL(trace) << "vertex shader log:\n" << shader->log();
            auto updater = new SimpleCacheItemUpdater<VertexShader> {
                uint32_t(), (uint32_t)key.bytecode.size(), [](auto){}
            };
            _context->vertexShaderCache.insert(key, shader, updater);
        }
        _context->vertexShader = shader;
    }
}

void Rsx::VertexTextureOffset(unsigned index, 
                              uint32_t offset, 
                              uint8_t mipmap,
                              uint8_t format,
                              uint8_t dimension,
                              uint8_t location)
{
    TRACE6(VertexTextureOffset, index, offset, mipmap, format, dimension, location);
    auto& t = _context->vertexTextureSamplers[index].texture;
    t.offset = offset;
    t.mipmap = mipmap;
    t.format = format;
    t.dimension = dimension;
    t.location = gcmEnumToLocation(location);
}

void Rsx::VertexTextureControl3(unsigned index, uint32_t pitch) {
    TRACE2(VertexTextureControl3, index, pitch);
    _context->vertexTextureSamplers[index].texture.pitch = pitch;
}

void Rsx::VertexTextureImageRect(unsigned index, uint16_t width, uint16_t height) {
    TRACE3(VertexTextureImageRect, index, width, height);
    _context->vertexTextureSamplers[index].texture.width = width;
    _context->vertexTextureSamplers[index].texture.height = height;
}

void Rsx::VertexTextureControl0(unsigned index, bool enable, float minlod, float maxlod) {
    TRACE4(VertexTextureControl0, index, enable, minlod, maxlod);
    _context->vertexTextureSamplers[index].enable = enable;
    _context->vertexTextureSamplers[index].minlod = minlod;
    _context->vertexTextureSamplers[index].maxlod = maxlod;
}

void Rsx::VertexTextureAddress(unsigned index, uint8_t wraps, uint8_t wrapt) {
    TRACE3(VertexTextureAddress, index, wraps, wrapt);
    _context->vertexTextureSamplers[index].wraps = wraps;
    _context->vertexTextureSamplers[index].wrapt = wrapt;
}

void Rsx::VertexTextureFilter(unsigned int index, float bias) {
    TRACE2(VertexTextureFilter, index, bias);
    _context->vertexTextureSamplers[index].bias = bias;
}

GLTexture* Rsx::addTextureToCache(uint32_t samplerId, bool isFragment) {
    TRACE2(addTextureToCache, samplerId, isFragment);
    auto& info = isFragment ? _context->fragmentTextureSamplers[samplerId].texture
                            : _context->vertexTextureSamplers[samplerId].texture;
    TextureCacheKey key { info.offset, (uint32_t)info.location, info.width, info.height, info.format };
    uint32_t va = 0;
    if (_mode != RsxOperationMode::Replay) {
        va = rsxOffsetToEa(info.location, key.offset);
    }
    auto size = (info.pitch == 0 ? info.width : info.pitch) * info.height;
    auto updater = new SimpleCacheItemUpdater<GLTexture> {
        va, size,
        [=](auto t) {
            std::vector<uint8_t> buf(size);
            _mm->readMemory(va, &buf[0], size);
            t->update(buf);
            
            _context->tracer.pushBlob(&buf[0], buf.size());
            TRACE5(UpdateTextureCache, 
                   key.offset, 
                   key.location,
                   key.width,
                   key.height,
                   key.internalType);
        }, [=](auto t, auto& blob) {
            t->update(blob);
        }
    };
    auto texture = new GLTexture(info);
    _context->textureCache.insert(key, texture, updater);
    return texture;
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
    auto texture = _context->textureCache.retrieve(key);
    if (!texture) {
        texture = addTextureToCache(samplerId, isFragment);
    }
    return texture;
}

void Rsx::updateTextures() {
    int i = 0;
    auto vertexSamplerUniform = (VertexShaderSamplerUniform*)_context->vertexSamplersBuffer.mapped();
    for (auto& sampler : _context->vertexTextureSamplers) {
        if (sampler.enable) {
            auto textureUnit = i + VertexTextureUnit;
            auto texture = getTextureFromCache(i, false);
            texture->bind(textureUnit);
            auto handle = sampler.glSampler.handle();
            if (!handle) {
                sampler.glSampler = GLSampler();
                handle = sampler.glSampler.handle();
                glcall(glBindSampler(textureUnit, handle));
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
    auto fragmentSamplerUniform = (FragmentShaderSamplerUniform*)_context->fragmentSamplersBuffer.mapped();
    for (auto& sampler : _context->fragmentTextureSamplers) {
        if (sampler.enable && sampler.texture.width && sampler.texture.height) {
            auto textureUnit = i + FragmentTextureUnit;
            auto va = rsxOffsetToEa(sampler.texture.location, sampler.texture.offset);
            auto surfaceTex = _context->framebuffer->findTexture(va);
            if (surfaceTex) {
                glcall(glBindTextureUnit(textureUnit, surfaceTex->handle()));
            } else {
                auto texture = getTextureFromCache(i, true);
                texture->bind(textureUnit);
            }
            
            auto handle = sampler.glSampler.handle();
            if (!handle) {
                sampler.glSampler = GLSampler();
                handle = sampler.glSampler.handle();
                glcall(glBindSampler(textureUnit, handle));
            }
            glSamplerParameterf(handle, GL_TEXTURE_MIN_LOD, sampler.minlod);
            glSamplerParameterf(handle, GL_TEXTURE_MAX_LOD, sampler.maxlod);
            glSamplerParameterf(handle, GL_TEXTURE_LOD_BIAS, sampler.bias);
            glSamplerParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glSamplerParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glSamplerParameteri(handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glSamplerParameteri(handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            // TODO: handle viewport CELL_GCM_WINDOW_ORIGIN_TOP
            fragmentSamplerUniform->flip[i] = surfaceTex != nullptr;
        }
        i++;
    }
}

void Rsx::init(Process* proc) {
    _proc = proc;
    _mm = proc->mm();
    
    BOOST_LOG_TRIVIAL(trace) << "waiting for rsx loop to initialize";
    
    _thread.reset(new boost::thread([=]{ loop(); }));
    
    // lock the thread until Rsx has initialized the buffer
    boost::unique_lock<boost::mutex> lock(_initMutex);
    _initCv.wait(lock, [=] { return _initialized; });
    
    BOOST_LOG_TRIVIAL(trace) << "rsx loop completed initialization";
}

void Rsx::VertexTextureBorderColor(unsigned int index, float a, float r, float g, float b) {
    TRACE5(VertexTextureBorderColor, index, a, r, g, b);
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
    TRACE9(TextureAddress, 
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
    TRACE5(TextureBorderColor, index, a, r, g, b);
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
    TRACE9(TextureFilter, index, bias, min, mag, conv, as, rs, gs, bs);
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
                        uint8_t format, 
                        uint8_t dimension,
                        bool border, 
                        bool cubemap, 
                        uint8_t location)
{
    TRACE8(TextureOffset, index, offset, mipmap, format, dimension, border, cubemap, location);
    auto& t = _context->fragmentTextureSamplers[index].texture;
    t.offset = offset;
    t.mipmap = mipmap;
    t.format = format;
    t.dimension = dimension;
    t.fragmentBorder = border;
    t.fragmentCubemap = cubemap;
    t.location = gcmEnumToLocation(location);
}

void Rsx::TextureImageRect(unsigned int index, uint16_t width, uint16_t height) {
    TRACE3(TextureImageRect, index, width, height);
    _context->fragmentTextureSamplers[index].texture.width = width;
    _context->fragmentTextureSamplers[index].texture.height = height;
}

void Rsx::TextureControl3(unsigned int index, uint16_t depth, uint32_t pitch) {
    TRACE3(TextureControl3, index, depth, pitch);
    _context->fragmentTextureSamplers[index].texture.pitch = pitch;
    _context->fragmentTextureSamplers[index].texture.fragmentDepth = depth;
}

void Rsx::TextureControl1(unsigned int index, uint32_t remap) {
    TRACE2(TextureControl1, index, remap);
    _context->fragmentTextureSamplers[index].texture.fragmentRemapCrossbarSelect = remap;
}

void Rsx::TextureControl2(unsigned int index, uint8_t slope, bool iso, bool aniso) {
    TRACE4(TextureControl2, index, slope, iso, aniso);
    // ignore these optimizations
}

void Rsx::TextureControl0(unsigned index,
                          uint8_t alphaKill,
                          uint8_t maxaniso,
                          float maxlod,
                          float minlod,
                          bool enable) {
    TRACE6(TextureControl0, index, alphaKill, maxaniso, maxlod, minlod, enable);
    // ignore maxaniso
    _context->fragmentTextureSamplers[index].fragmentAlphaKill = alphaKill;
    _context->fragmentTextureSamplers[index].enable = enable;
    _context->fragmentTextureSamplers[index].minlod = minlod;
    _context->fragmentTextureSamplers[index].maxlod = maxlod;
}

void Rsx::SetReference(uint32_t ref) {
    waitForIdle();
    _ref = ref;
}

// Surface

void Rsx::SurfaceCompression(uint32_t x) {
    TRACE1(SurfaceCompression, x);
}

void Rsx::setSurfaceColorLocation(unsigned index, uint32_t location) {
    TRACE2(setSurfaceColorLocation, index, location);
    assert(index < 4);
    _context->surface.colorLocation[index] = gcmEnumToLocation(location);
}

void Rsx::SurfaceFormat(uint8_t colorFormat,
                        uint8_t depthFormat,
                        uint8_t antialias,
                        uint8_t type,
                        uint8_t width,
                        uint8_t height,
                        uint32_t pitchA,
                        uint32_t offsetA,
                        uint32_t offsetZ,
                        uint32_t offsetB,
                        uint32_t pitchB) {
    TRACE11(SurfaceFormat,
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
            pitchB
    );
    assert(colorFormat == CELL_GCM_SURFACE_A8R8G8B8);
    if (depthFormat == CELL_GCM_SURFACE_Z16) {
        _context->surface.depthFormat = SurfaceDepthFormat::z16;
    } else if (depthFormat == CELL_GCM_SURFACE_Z24S8) {
        _context->surface.depthFormat = SurfaceDepthFormat::z24s8;
    } else {
        assert(false);
    }
    //assert(antialias == CELL_GCM_SURFACE_CENTER_1);
    assert(type == CELL_GCM_SURFACE_PITCH);
    _context->surface.width = 1 << (width + 1);
    _context->surface.height = 1 << (height + 1);
    assert(_context->surface.width == 2048);
    assert(_context->surface.height == 1024);
    _context->surface.colorPitch[0] = pitchA;
    _context->surface.colorPitch[1] = pitchB;
    _context->surface.colorOffset[0] = offsetA;
    _context->surface.colorOffset[1] = offsetB;
    _context->surface.depthOffset = offsetZ;
}

void Rsx::SurfacePitchZ(uint32_t pitch) {
    TRACE1(SurfacePitchZ, pitch);
    _context->surface.depthPitch = pitch;
}

void Rsx::SurfacePitchC(uint32_t pitchC, uint32_t pitchD, uint32_t offsetC, uint32_t offsetD) {
    TRACE4(SurfacePitchC, pitchC, pitchD, offsetC, offsetD);
    _context->surface.colorPitch[2] = pitchC;
    _context->surface.colorPitch[3] = pitchD;
    _context->surface.colorOffset[2] = offsetC;
    _context->surface.colorOffset[3] = offsetD;
}

void Rsx::SurfaceColorTarget(uint32_t target) {
    TRACE1(SurfaceColorTarget, target);
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
    TRACE2(WindowOffset, x, y);
    _context->surface.windowOriginX = x;
    _context->surface.windowOriginY = y;
}

void Rsx::SurfaceClipHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    TRACE4(SurfaceClipHorizontal, x, w, y, h);
}

// assume cellGcmSetSurface is always used and not its subcommands
// then this command is always set last
void Rsx::ShaderWindow(uint16_t height, uint8_t origin, uint16_t pixelCenters) {
    TRACE3(ShaderWindow, height, origin, pixelCenters);
    assert(origin == CELL_GCM_WINDOW_ORIGIN_BOTTOM);
    assert(pixelCenters == CELL_GCM_WINDOW_PIXEL_CENTER_HALF);
    waitForIdle();
    assert(_context->surface.width >= _window.width());
    assert(_context->surface.height >= _window.height());
    _context->framebuffer->setSurface(_context->surface, _window.width(), _window.height());
}

void Rsx::Control0(uint32_t format) {
    TRACE1(Control0, format);
    if (format & 0x00100000) {
        auto depthFormat = (format & ~0x00100000) >> 12;
        if (depthFormat == CELL_GCM_DEPTH_FORMAT_FIXED) {
            _context->surface.depthType = SurfaceDepthType::Fixed;
        } else if (depthFormat == CELL_GCM_DEPTH_FORMAT_FLOAT) {
            _context->surface.depthType = SurfaceDepthType::Float;
        } else {
            assert(false);
        }
    } else {
        BOOST_LOG_TRIVIAL(error) << "unknown control0";
    }
}

void Rsx::ContextDmaColorA(uint32_t context) {
    TRACE1(ContextDmaColorA, context);
    setSurfaceColorLocation(0, context);
}

void Rsx::ContextDmaColorB(uint32_t context) {
    TRACE1(ContextDmaColorB, context);
    setSurfaceColorLocation(1, context);
}

void Rsx::ContextDmaColorC_2(uint32_t contextC, uint32_t contextD) {
    TRACE2(ContextDmaColorC_2, contextC, contextD);
    setSurfaceColorLocation(2, contextC);
    setSurfaceColorLocation(3, contextD);
}

void Rsx::ContextDmaColorD(uint32_t context) {
    TRACE1(ContextDmaColorD, context);
    setSurfaceColorLocation(3, context);
}

void Rsx::ContextDmaColorC_1(uint32_t contextC) {
    TRACE1(ContextDmaColorC_1, contextC);
    setSurfaceColorLocation(2, contextC);
}

void Rsx::ContextDmaZeta(uint32_t context) {
    TRACE1(ContextDmaZeta, context);
    _context->surface.depthLocation = gcmEnumToLocation(context);
}

void Rsx::setDisplayBuffer(uint8_t id, uint32_t offset, uint32_t pitch, uint32_t width, uint32_t height) {
    TRACE5(setDisplayBuffer, id, offset, pitch, width, height);
    auto& buffer = _context->displayBuffers[id];
    buffer.offset = offset;
    buffer.pitch = pitch;
    buffer.width = width;
    buffer.height = height;
}

void glDebugCallbackFunction(GLenum source,
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
    //if (type == GL_DEBUG_TYPE_ERROR) {
        auto text = ssnprintf("gl callback [source=%s]: %s", sourceStr, message);
        BOOST_LOG_TRIVIAL(error) << text;
    //}
    if (severity == GL_DEBUG_SEVERITY_HIGH)
        exit(1);
}

void Rsx::initGcm() {
    BOOST_LOG_TRIVIAL(trace) << "initializing rsx";
    
    _window.init();
    
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(&glDebugCallbackFunction, nullptr);
    
    _context.reset(new RsxContext());
    
    if (_mode == RsxOperationMode::RunCapture) {
        _context->tracer.enable();
    }
    
    _mm->memoryBreakHandler([=](uint32_t va, uint32_t size) { memoryBreakHandler(va, size); });
    _context->pipeline.bind();
    
    size_t constBufferSize = VertexShaderConstantCount * sizeof(float) * 4;
    _context->vertexConstBuffer = GLPersistentCpuBuffer(constBufferSize);
    
    auto uniformSize = sizeof(VertexShaderSamplerUniform);
    _context->vertexSamplersBuffer = GLPersistentCpuBuffer(uniformSize);
    
    uniformSize = sizeof(VertexShaderViewportUniform);
    _context->vertexViewportBuffer = GLPersistentCpuBuffer(uniformSize);
    
    uniformSize = sizeof(FragmentShaderSamplerUniform);
    _context->fragmentSamplersBuffer = GLPersistentCpuBuffer(uniformSize);
    
    _context->localMemoryBuffer = GLPersistentCpuBuffer(256 * (1u << 20));
    _context->mainMemoryBuffer = GLPersistentCpuBuffer(256 * (1u << 20));
    
    _mm->provideMemory(
        RsxFbBaseAddr, GcmLocalMemorySize, _context->localMemoryBuffer.mapped());
    
    _context->elementArrayIndexBuffer = GLPersistentCpuBuffer(10 * (1u << 20));
    
    for (auto& s : _context->vertexTextureSamplers) {
        s.enable = false;
    }
    for (auto& s : _context->fragmentTextureSamplers) {
        s.enable = false;
    }
    
    _context->framebuffer.reset(new GLFramebuffer());
    _context->textureRenderer.reset(new TextureRenderer());
    
    boost::lock_guard<boost::mutex> lock(_initMutex);
    BOOST_LOG_TRIVIAL(trace) << "rsx initialized";
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
    
    if (n == 0 && f == 0)
        return;
    
    auto glDepth = (n == 0 && f == 1) ? GL_ZERO_TO_ONE
                 : (n == -1 && f == 1) ? GL_NEGATIVE_ONE_TO_ONE
                 : 0;
    assert(glDepth);
    
    glcall(glClipControl(GL_UPPER_LEFT, glDepth));
    auto w = _context->viewPort.width;
    auto h = _context->viewPort.height;
    auto x = _context->viewPort.x;
    auto y = _context->viewPort.y;
    glcall(glViewport(x, _window.height() - (y + h), w, h));
    glcall(glDepthRange(n, f));
    
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
    
    auto viewportUniform = (VertexShaderViewportUniform*)_context->vertexViewportBuffer.mapped();
    viewportUniform->glInverseGcm = glm::inverse(gl) * gcm;
}

GLPersistentCpuBuffer* Rsx::getBuffer(MemoryLocation location) {
    return location == MemoryLocation::Local ? &_context->localMemoryBuffer
                                             : &_context->mainMemoryBuffer;
}

void Rsx::updateVertexDataArrays(unsigned first, unsigned count) {
    for (auto i = 0u; i < _context->vertexDataArrays.size(); ++i) {
        auto& format = _context->vertexDataArrays[i];
        auto& input = _context->vertexInputs[i];
        if (!input.enabled)
            continue;
   
        auto bufferOffset = format.offset + first * format.stride;
        auto handle = getBuffer(format.location)->handle();
        
        glcall(glVertexAttribBinding(i, format.binding));
        glcall(glBindVertexBuffer(format.binding, handle, bufferOffset, format.stride));
        
        if (_mode != RsxOperationMode::Replay) {
            UpdateBufferCache(format.location, bufferOffset, count * format.stride);
        }
    }
}

void Rsx::waitForIdle() {
    TRACE0(waitForIdle);
    glcall(glFinish());
}

void Rsx::BackEndWriteSemaphoreRelease(uint32_t value) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("BackEndWriteSemaphoreRelease(%x)", value);
    waitForIdle();
    _mm->store<4>(_context->semaphoreOffset + GcmLabelBaseOffset, value);
    __sync_synchronize();
}

void Rsx::SemaphoreOffset(uint32_t offset) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SemaphoreOffset(%x)", offset);
    _context->semaphoreOffset = offset;
}

void Rsx::memoryBreakHandler(uint32_t va, uint32_t size) {
    _context->textureCache.invalidate(va, size);
    _context->fragmentShaderCache.invalidate(va, size);
}

bool Rsx::isCallActive() {
    return _ret != 0;
}

uint32_t Rsx::getGet() {
    return _get;
}

void Rsx::watchCaches() {
    if (_mode == RsxOperationMode::Replay)
        return;
    auto setMemoryBreak = [=](uint32_t va, uint32_t size) { _mm->memoryBreak(va, size); };
    _context->textureCache.syncAll();
    _context->textureCache.watch(setMemoryBreak);
    _context->fragmentShaderCache.syncAll();
    _context->fragmentShaderCache.watch(setMemoryBreak);
}

void Rsx::setVBlankHandler(uint32_t descrEa) {
    _context->vBlankHandlerDescr = descrEa;
}

void Rsx::OffsetDestin(uint32_t offset) {
    TRACE1(OffsetDestin, offset);
    _context->surface2d.destOffset = offset;
}

void Rsx::ContextDmaImage(uint32_t location) {
    TRACE1(ContextDmaImage, location);
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
    TRACE3(ColorFormat_3, format, pitch, offset);
    colorFormat(_context.get(), format, pitch);
    _context->surface2d.offset = offset;
}

void Rsx::ColorFormat_2(uint32_t format, uint16_t pitch) {
    TRACE2(ColorFormat_2, format, pitch);
    colorFormat(_context.get(), format, pitch);
}

void Rsx::Point(uint16_t pointX, 
                uint16_t pointY, 
                uint16_t outSizeX, 
                uint16_t outSizeY, 
                uint16_t inSizeX, 
                uint16_t inSizeY)
{
    TRACE6(Point, pointX, pointY, outSizeX, outSizeY, inSizeX, inSizeY);
    
    InlineSettings& i = _context->inline2d;
    i.pointX = pointX;
    i.pointY = pointY;
    i.destSizeX = outSizeX;
    i.destSizeY = outSizeY;
    i.srcSizeX = inSizeX;
    i.srcSizeY = inSizeY;
}

void Rsx::Color(uint32_t ptr, uint32_t count) {
    TRACE2(Color, ptr, count);
    
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
    _mm->readMemory(ptr, &vec[0], count * 4);
    _mm->writeMemory(dest, &vec[0], count * 4);
}

void Rsx::ContextDmaImageDestin(uint32_t location) {
    TRACE1(ContextDmaImageDestin, location);
    _context->surface2d.destLocation = gcmEnumToLocation(location);
}

void Rsx::OffsetIn_1(uint32_t offset) {
    TRACE1(OffsetIn_1, offset);
    _context->copy2d.srcOffset = offset;
}

void Rsx::OffsetOut(uint32_t offset) {
    TRACE1(OffsetOut, offset);
    _context->copy2d.destOffset = offset;
}

void Rsx::PitchIn(int32_t inPitch,
                  int32_t outPitch,
                  uint32_t lineLength,
                  uint32_t lineCount,
                  uint8_t inFormat,
                  uint8_t outFormat) {
    TRACE6(PitchIn, inPitch, outPitch, lineLength, lineCount, inFormat, outFormat);
    
    CopySettings& c = _context->copy2d;
    c.srcPitch = inPitch;
    c.destPitch = outPitch;
    c.lineLength = lineLength;
    c.lineCount = lineCount;
    c.srcFormat = inFormat;
    c.destFormat = outFormat;
}

void Rsx::DmaBufferIn(uint32_t sourceLocation, uint32_t dstLocation) {
    TRACE2(DmaBufferIn, sourceLocation, dstLocation);
    _context->copy2d.srcLocation = gcmEnumToLocation(sourceLocation);
    _context->copy2d.destLocation = gcmEnumToLocation(dstLocation);
}

void startCopy2d(RsxContext* context, MainMemory* mm) {
    CopySettings& c = context->copy2d;
    auto sourceEa = rsxOffsetToEa(c.srcLocation, c.srcOffset);
    auto destEa = rsxOffsetToEa(c.destLocation, c.destOffset);

    assert(c.srcFormat == 1 || c.srcFormat == 2 || c.srcFormat == 4);
    assert(c.destFormat == 1 || c.destFormat == 2 || c.destFormat == 4);
    
    uint32_t srcLine = sourceEa;
    uint32_t destLine = destEa;
    for (auto line = 0u; line < c.lineCount; ++line) {
        uint8_t* src = mm->getMemoryPointer(srcLine, c.lineLength);
        uint8_t* dest = mm->getMemoryPointer(destLine, c.lineLength);
        auto srcLineEnd = src + c.lineLength;
        while (src < srcLineEnd) {
            *dest = *src;
            src += c.srcFormat;
            dest += c.destFormat;
        }
        srcLine += c.srcPitch;
        destLine += c.destPitch;
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
    TRACE9(OffsetIn_9,
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
    
    startCopy2d(_context.get(), _mm);
}

void Rsx::BufferNotify(uint32_t notify) {
    TRACE1(BufferNotify, notify);
    assert(notify == 0);
    startCopy2d(_context.get(), _mm);
}

void Rsx::Nv3089ContextDmaImage(uint32_t location) {
    TRACE1(Nv3089ContextDmaImage, location);
    _context->scale2d.location = gcmEnumToLocation(location);
}

void Rsx::Nv3089ContextSurface(uint32_t surfaceType) {
    TRACE1(Nv3089ContextSurface, surfaceType);
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
    TRACE13(Nv3089SetColorConversion, conv, fmt, op, x, y, w, h, outX, outY, outW, outH, dsdx, dtdy);
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

union r6g6b5_t {
    uint32_t val;
    BitField<0, 5> r;
    BitField<5, 11> g;
    BitField<11, 16> b;
};

void transferImage(RsxContext* context, MainMemory* mm) {
    ScaleSettings& scale = context->scale2d;
    SurfaceSettings& surface = context->surface2d;
    SwizzleSettings& swizzle = context->swizzle2d;
    
    bool isSwizzle = scale.type == ScaleSettingsSurfaceType::Swizzle;
    
    auto sourceEa = rsxOffsetToEa(scale.location, scale.offset);
    auto destEa = isSwizzle
                      ? rsxOffsetToEa(swizzle.location, swizzle.offset)
                      : rsxOffsetToEa(surface.destLocation, surface.destOffset);
    auto src = mm->getMemoryPointer(sourceEa, 1);
    auto dest = mm->getMemoryPointer(destEa, 1);
    
    auto sourcePixelSize = scale.format == ScaleSettingsFormat::r5g6b5 ? 2 : 4;
    auto destPixelSize = surface.format == ScaleSettingsFormat::r5g6b5 ? 2 : 4;
    
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
            uint32_t srcPixel = *(big_uint32_t*)srcPixelPtr;
            uint32_t destPixel = srcPixel;
            if (sourcePixelSize != destPixelSize) {
                if (sourcePixelSize == 2) {
                    r6g6b5_t in { srcPixel };
                    destPixel = (0xff << 24)
                              | (ext8(in.r) << 16)
                              | (ext8(in.g) << 8)
                              | ext8(in.b);
                } else {
                    r6g6b5_t out { 0 };
                    out.r.set(((srcPixel >> 16) & 0xff) * 32 / 255);
                    out.g.set(((srcPixel >> 8) & 0xff) * 64 / 255);
                    out.b.set((srcPixel & 0xff) * 32 / 255);
                    destPixel = out.val;
                }
            }
            if (destPixelSize == 2) {
                *(big_uint16_t*)destPixelPtr = destPixel >> 16;
            } else {
                *(big_uint32_t*)destPixelPtr = destPixel;
            }
        }
    }
}

void Rsx::ImageInSize(uint16_t inW,
                      uint16_t inH,
                      uint16_t pitch,
                      uint8_t origin,
                      uint8_t interpolator,
                      uint32_t offset,
                      float inX,
                      float inY) {
    TRACE8(ImageInSize, inW, inH, pitch, origin, interpolator, offset, inX, inY);
    
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
    
    transferImage(_context.get(), _mm);
}

void Rsx::Nv309eSetFormat(uint16_t format,
                          uint8_t width,
                          uint8_t height,
                          uint32_t offset) {
    TRACE4(Nv309eSetFormat, format, width, height, offset);
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
    TRACE1(BlendEnable, enable);
    glEnableb(GL_BLEND, enable);
}

GLenum gcmBlendFuncToOpengl(uint16_t func) {
#define X(x) case CELL_GCM_##x: return GL_##x;
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

void Rsx::BlendFuncSFactor(uint16_t sfcolor, 
                           uint16_t sfalpha,
                           uint16_t dfcolor,
                           uint16_t dfalpha) {
    TRACE4(BlendFuncSFactor, sfcolor, sfalpha, dfcolor, dfalpha);
    glBlendFuncSeparate(
        gcmBlendFuncToOpengl(sfcolor),
        gcmBlendFuncToOpengl(dfcolor),
        gcmBlendFuncToOpengl(sfalpha),
        gcmBlendFuncToOpengl(dfalpha)
    );
}

void Rsx::LogicOpEnable(bool enable) {
    TRACE1(LogicOpEnable, enable);
    glEnableb(GL_COLOR_LOGIC_OP, enable);
}

GLenum gcmBlendEquationToOpengl(uint16_t eq) {
    switch (eq) {
        case CELL_GCM_FUNC_ADD: return GL_FUNC_ADD;
        case CELL_GCM_MIN: return GL_MIN;
        case CELL_GCM_MAX: return GL_MAX;
        case CELL_GCM_FUNC_SUBTRACT: return GL_FUNC_SUBTRACT;
        case CELL_GCM_FUNC_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
    }
    throw std::runtime_error("unsupported blend equation");
}

void Rsx::BlendEquation(uint16_t color, uint16_t alpha) {
    TRACE2(BlendEquation, color, alpha);
    glBlendEquationSeparate(
        gcmBlendEquationToOpengl(color),
        gcmBlendEquationToOpengl(alpha));
}

void Rsx::ZStencilClearValue(uint32_t value) {
    TRACE1(ZStencilClearValue, value);
    glClearStencil(value);
}

void Rsx::VertexData4fM(unsigned index, float x, float y, float z, float w) {
    TRACE5(VertexData4fM, index, x, y, z, w);
    glVertexAttrib4f(index, x, y, z, w);
}

void Rsx::setOperationMode(RsxOperationMode mode) {
    _mode = mode;
}

RsxOperationMode Rsx::_mode = RsxOperationMode::Run;

void Rsx::UpdateBufferCache(MemoryLocation location, uint32_t offset, uint32_t size) {
    if (_mode == RsxOperationMode::Run)
        return;
    
    auto buffer = getBuffer(location);
    auto mapped = buffer->mapped() + offset;
    if (_mode == RsxOperationMode::RunCapture) {
        _context->tracer.pushBlob(mapped, size);
        TRACE3(UpdateBufferCache, location, offset, size);
    } else if (_mode == RsxOperationMode::Replay) {
        assert(size == _currentReplayBlob.size());
        memcpy(mapped, &_currentReplayBlob[0], size);
    }
}

void Rsx::UpdateTextureCache(uint32_t offset, uint32_t location, uint32_t width, uint32_t height, uint8_t format) {
    TextureCacheKey key { offset, location, width, height, format };
    auto tuple = _context->textureCache.retrieveWithUpdater(key);
    auto texture = std::get<0>(tuple);
    auto updater = std::get<1>(tuple);
    updater->updateWithBlob(texture, _currentReplayBlob);
}

void Rsx::UpdateFragmentCache(uint32_t va, uint32_t size) {
    auto tuple = _context->fragmentShaderCache.retrieveWithUpdater( { va, size });
    auto program = std::get<0>(tuple);
    auto updater = std::get<1>(tuple);
    updater->updateWithBlob(program, _currentReplayBlob);
}

RsxContext* Rsx::context() {
    return _context.get();
}

void Rsx::updateOffsetTableForReplay() {
    if (_mode == RsxOperationMode::Run)
        return;
    
    if (_mode == RsxOperationMode::RunCapture) {
        auto table = serializeOffsetTable();
        _context->tracer.pushBlob(&table[0], table.size() * 2);
        TRACE0(updateOffsetTableForReplay);
    } else {
        std::vector<uint16_t> vec(_currentReplayBlob.size() / 2);
        memcpy(&vec[0], &_currentReplayBlob[0], _currentReplayBlob.size());
        deserializeOffsetTable(vec);
    }
}
