#include "Rsx.h"

#include "GLBuffer.h"
#include "Cache.h"
#include "GLFramebuffer.h"
#include "GLTexture.h"
#include "../PPU.h"
#include "../shaders/ShaderGenerator.h"
#include "../utils.h"
#include <atomic>
#include <vector>
#include <fstream>
#include <boost/log/trivial.hpp>
#include "../../libs/graphics/graphics.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct VertexDataArrayFormatInfo {
    uint16_t frequency;
    uint8_t stride;
    uint8_t size;
    uint8_t type;
    MemoryLocation location;
    uint32_t offset;
    GLuint binding = 0;
};

typedef std::array<float, 4> glvec4_t;

void check_shader(GLuint shader) {
    GLint param;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &param);
    if (param != GL_TRUE) {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &param);
        std::string s(param + 1, 0);
        GLsizei len;
        glGetShaderInfoLog(shader, param, &len, &s[0]);
        throw std::runtime_error(ssnprintf("shader error: %s\n", s.c_str()));
    }
}

enum class GLShaderType {
    vertex, fragment
};

void deleteShader(GLuint handle) {
    glDeleteShader(handle);
}

class GLShader : public HandleWrapper<GLuint, deleteShader> {
public:
    GLShader() : HandleWrapper(0) { }
    GLShader(GLShaderType type) : HandleWrapper(init(type)) { }
    GLuint init(GLShaderType type) {
        auto glType = type == GLShaderType::fragment ? 
            GL_FRAGMENT_SHADER : GL_VERTEX_SHADER;
        return glCreateShader(glType);
    }
};

void deleteProgram(GLuint handle) {
    glDeleteProgram(handle);
}

class GLProgram : public HandleWrapper<GLuint, deleteProgram> {
public:
    GLProgram(GLuint handle) : HandleWrapper(handle) { }
    GLProgram() : HandleWrapper(glCreateProgram()) { }
};

void deleteSampler(GLuint handle) {
    glDeleteSamplers(1, &handle);
}

class GLSampler : public HandleWrapper<GLuint, deleteSampler> {
public:
    GLSampler(GLuint handle) : HandleWrapper(handle) { }
    GLSampler() : HandleWrapper(init()) { }
    GLuint init() {
        GLuint handle;
        glcall(glCreateSamplers(1, &handle));
        return handle;
    }
};

struct TextureSamplerInfo {
    bool enable;
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

struct TransferInfo {
    uint32_t offsetDestin;
    uint16_t destPitch;
    uint16_t sourcePitch;
    uint16_t pointX; 
    uint16_t pointY; 
    uint16_t outSizeX;
    uint16_t outSizeY; 
    uint16_t inSizeX;
    uint16_t inSizeY;
};

struct BufferCacheKey {
    uint32_t va;
    uint32_t size;
    inline bool operator<(BufferCacheKey const& other) const {
        return std::tie(va, size)
             < std::tie(other.va, other.size);
    }
};

struct TextureCacheKey {
    uint32_t va;
    uint32_t width;
    uint32_t height;
    uint8_t internalType;
    inline bool operator<(TextureCacheKey const& other) const {
        return std::tie(va, width, height, internalType)
             < std::tie(other.va, other.width, other.height, other.internalType);
    }
};

class GLFramebuffer;
class TextureRenderer;
class RsxContext {
public:
    uint32_t gcmIoSize;
    ps3_uintptr_t gcmIoAddress;
    Window window;
    SurfaceInfo surface;
    ViewPortInfo viewPort;
    float clipMin;
    float clipMax;
    float viewPortScale[4];
    float viewPortOffset[4];
    uint32_t colorMask;
    bool isDepthTestEnabled;
    uint32_t depthFunc;
    bool isCullFaceEnabled;
    bool isFlatShadeMode;
    glm::vec4 colorClearValue;
    GLuint glClearSurfaceMask;
    bool isFlipInProgress = false;
    std::array<VertexDataArrayFormatInfo, 16> vertexDataArrays;
    GLuint glVertexArrayMode;
    GLShader vertexShader;
    GLShader fragmentShader;
    GLuint vertexConstBuffer;
    GLuint vertexSamplersBuffer;
    GLuint fragmentSamplersBuffer;
    GLProgram shaderProgram = GLProgram(0);
    bool vertexShaderDirty = false;
    bool fragmentShaderDirty = false;
    uint32_t fragmentOffset = 0;
    std::vector<uint8_t> fragmentBytecode;
    std::vector<uint8_t> lastFrame;
    std::array<VertexShaderInputFormat, 16> vertexInputs;
    std::array<uint8_t, 512 * 16> vertexInstructions;
    uint32_t vertexLoadOffset = 0;
    uint32_t vertexIndexArrayOffset = 0;
    GLuint vertexIndexArrayGlType = 0;
    TextureSamplerInfo vertexTextureSamplers[4];
    TextureSamplerInfo fragmentTextureSamplers[16];
    DisplayBufferInfo displayBuffers[8];
    std::unique_ptr<GLFramebuffer> framebuffer;
    std::unique_ptr<TextureRenderer> textureRenderer;
    std::unique_ptr<uint8_t[]> localMemory;
    uint32_t semaphoreOffset = 0;
    TransferInfo transfer;
    Cache<BufferCacheKey, GLBuffer, 256 * (2 >> 20)> bufferCache;
    Cache<TextureCacheKey, GLTexture, 256 * (2 >> 20)> textureCache;
};

Rsx::Rsx(PPU* ppu) : _ppu(ppu), _context(new RsxContext()) { }

Rsx::~Rsx() {
    shutdown();
}

class TextureRenderer {
    GLProgram _program;
    GLSampler _sampler;
    GLBuffer _buffer;
public:
    TextureRenderer() {
        GLShader fragmentShader(GLShaderType::fragment);
        GLShader vertexShader(GLShaderType::vertex);
        
        auto fragmentCode =
            "#version 450 core\n"
            "out vec4 color;\n"
            "layout (binding = 20) uniform sampler2D tex;\n"
            "void main(void) {\n"
            "    vec2 size = textureSize(tex, 0);\n"
            "    color = texture(tex, gl_FragCoord.xy / size, 0);\n"
            "}";

        auto vertexCode =
            "#version 450 core\n"
            "layout (location = 0) in vec2 pos;\n"
            "void main(void) {\n"
            "    gl_Position = vec4(pos, 0, 1);\n"
            "}\n";
        
        auto fs = fragmentShader.handle();
        auto vs = vertexShader.handle();
        
        glShaderSource(fs, 1, &fragmentCode, NULL);
        glShaderSource(vs, 1, &vertexCode, NULL);
        glCompileShader(fs);
        glCompileShader(vs);
        check_shader(fs);
        check_shader(vs);
        
        glAttachShader(_program.handle(), fs);
        glAttachShader(_program.handle(), vs);
        glcall(glLinkProgram(_program.handle()));
        
        glm::vec2 vertices[] {
            { -1.f, 1.f }, { -1.f, -1.f }, { 1.f, -1.f },
            { -1.f, 1.f }, { 1.f, 1.f }, { 1.f, -1.f }
        };
        
        _buffer = GLBuffer(GLBufferType::Static, sizeof(vertices), vertices);
    }
    
    void render(GLSimpleTexture* tex) {
        glcall(glEnableVertexAttribArray(0));
        glcall(glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0));
        glcall(glVertexAttribBinding(0, 0));
        glcall(glBindVertexBuffer(0, _buffer.handle(), 0, sizeof(glm::vec2)));
        
        auto samplerIndex = 20;
        glcall(glBindTextureUnit(samplerIndex, tex->handle()));
        glcall(glBindSampler(samplerIndex, _sampler.handle()));
        glSamplerParameteri(_sampler.handle(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glSamplerParameteri(_sampler.handle(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glcall(glUseProgram(_program.handle()));
        
        glcall(glDrawArrays(GL_TRIANGLES, 0, 6));
        
        // TODO: restore vertex attrib if required
    }
};

void Rsx::setLabel(int index, uint32_t value) {
    auto offset = index * 0x10;
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("setting rsx label at offset %x", offset);
    auto ptr = _ppu->getMemoryPointer(GcmLabelBaseOffset + offset, sizeof(uint32_t));
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
    auto ptr = _ppu->getMemoryPointer(GcmLabelBaseOffset + offset, sizeof(uint32_t));
    auto atomic = (std::atomic<uint32_t>*)ptr;
    while (boost::endian::big_to_native(atomic->load()) != value) ;
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("acquired");
}

void Rsx::SemaphoreRelease(uint32_t value) {
    auto offset = _semaphores[_activeSemaphoreHandle];
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("releasing semaphore %x at offset %x with value %x",
        _activeSemaphoreHandle, offset, value
    );
    auto ptr = _ppu->getMemoryPointer(GcmLabelBaseOffset + offset, sizeof(uint32_t));
    auto atomic = (std::atomic<uint32_t>*)ptr;
    atomic->store(boost::endian::native_to_big(value));
}

void Rsx::ClearRectHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ClearRectHorizontal(%d, %d, %d, %d)", x, w, y, h);
    //TODO: implement
}

void Rsx::ClipIdTestEnable(uint32_t x) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ClipIdTestEnable(%d)", x);
    //TODO: implement
}

void Rsx::FlatShadeOp(uint32_t x) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("FlatShadeOp(%d)", x);
    //TODO: implement
}

void Rsx::VertexAttribOutputMask(uint32_t mask) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexAttribOutputMask(%x)", mask);
    //TODO: implement
}

void Rsx::FrequencyDividerOperation(uint16_t op) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("FrequencyDividerOperation(%x)", op);
    //TODO: implement
}

void Rsx::TexCoordControl(unsigned int index, uint32_t control) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TexCoordControl(%d, %x)", index, control);
    //TODO: implement
}

void Rsx::ReduceDstColor(bool enable) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ReduceDstColor(%d)", enable);
    //TODO: implement
}

void Rsx::FogMode(uint32_t mode) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("FogMode(%x)", mode);
    //TODO: implement
}

void Rsx::AnisoSpread(unsigned int index,
                      bool reduceSamplesEnable,
                      bool hReduceSamplesEnable,
                      bool vReduceSamplesEnable,
                      uint8_t spacingSelect,
                      uint8_t hSpacingSelect,
                      uint8_t vSpacingSelect) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("AnisoSpread(%d, ...)", index);
    //TODO: implement
}

void Rsx::VertexDataBaseOffset(uint32_t baseOffset, uint32_t baseIndex) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexDataBaseOffset(%x, %x)", baseOffset, baseIndex);
    //TODO: implement
}

void Rsx::AlphaFunc(uint32_t af, uint32_t ref) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("AlphaFunc(%x, %x)", af, ref);
    //TODO: implement
}

void Rsx::AlphaTestEnable(bool enable) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("AlphaTestEnable(%d)", enable);
    //TODO: implement
}

void Rsx::ShaderControl(uint32_t control, uint8_t registerCount) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ShaderControl(%x, %d)", control, registerCount);
    //TODO: implement
}

void Rsx::TransformProgramLoad(uint32_t load, uint32_t start) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TransformProgramLoad(%x, %x)", load, start);
    assert(load == 0);
    assert(start == 0); // TODO: handle one-parameter invocation without resetting start
    _context->vertexLoadOffset = load;
}

void Rsx::TransformProgram(uint32_t locationOffset, unsigned size) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TransformProgram(..., %d)", size);
    auto bytes = size * 4;
    auto src = _ppu->getMemoryPointer(GcmLocalMemoryBase + locationOffset, bytes);
    memcpy(&_context->vertexInstructions[_context->vertexLoadOffset], src, bytes);
    _context->vertexShaderDirty = true;
    _context->vertexLoadOffset += bytes;
}

void Rsx::VertexAttribInputMask(uint32_t mask) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexAttribInputMask(%x)", mask);
}

void Rsx::TransformTimeout(uint16_t count, uint16_t registerCount) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TransformTimeout(%x, %d)", count, registerCount);
}

void Rsx::ShaderProgram(uint32_t locationOffset) {
    // loads fragment program byte code from locationOffset-1 up to the last command
    // (with the "#last command" bit)
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ShaderProgram(%x)", locationOffset);
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("%x", _ppu->load<4>(locationOffset - 1 + GcmLocalMemoryBase));
    _context->fragmentOffset = GcmLocalMemoryBase + locationOffset - 1;
    _context->fragmentShaderDirty = true;
}

void Rsx::ViewportHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ViewportHorizontal(%d, %d, %d, %d)", x, w, y, h);
}

void Rsx::ClipMin(float min, float max) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ClipMin(%e, %e)", min, max);
    _context->clipMin = min;
    _context->clipMax = max;
}

void Rsx::ViewportOffset(float offset0,
                         float offset1,
                         float offset2,
                         float offset3,
                         float scale0,
                         float scale1,
                         float scale2,
                         float scale3) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ViewportOffset(%e, ...)", offset0);
    _context->viewPortOffset[0] = offset0;
    _context->viewPortOffset[1] = offset1;
    _context->viewPortOffset[2] = offset2;
    _context->viewPortOffset[3] = offset3;
    _context->viewPortScale[0] = scale0;
    _context->viewPortScale[1] = scale1;
    _context->viewPortScale[2] = scale2;
    _context->viewPortScale[3] = scale3;
}

void Rsx::ColorMask(uint32_t mask) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ColorMask(%x)", mask);
    _context->colorMask = mask;
    glcall(glColorMask(
        (mask & CELL_GCM_COLOR_MASK_R) ? GL_TRUE : GL_FALSE,
        (mask & CELL_GCM_COLOR_MASK_G) ? GL_TRUE : GL_FALSE,
        (mask & CELL_GCM_COLOR_MASK_B) ? GL_TRUE : GL_FALSE,
        (mask & CELL_GCM_COLOR_MASK_A) ? GL_TRUE : GL_FALSE
    ));
}

void Rsx::DepthTestEnable(bool enable) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("DepthTestEnable(%d)", enable);
    _context->isDepthTestEnabled = enable;
    if (enable) {
        glcall(glEnable(GL_DEPTH_TEST));
    } else {
        glcall(glDisable(GL_DEPTH_TEST));
    }
}

void Rsx::DepthFunc(uint32_t zf) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("DepthFunc(%d)", zf);
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("CullFaceEnable(%d)", enable);
    assert(!enable);
    _context->isCullFaceEnabled = enable;
    if (enable) {
        glcall(glEnable(GL_CULL_FACE));
    } else {
        glcall(glDisable(GL_CULL_FACE));
    }
}

void Rsx::ShadeMode(uint32_t sm) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ShadeMode(%x)", sm);
    _context->isFlatShadeMode = sm == CELL_GCM_FLAT;
    _context->fragmentShaderDirty = true;
    assert(sm == CELL_GCM_SMOOTH || sm == CELL_GCM_FLAT);
}

void Rsx::ColorClearValue(uint32_t color) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ColorClearValue(%x)", color);
    union {
        uint32_t val;
        BitField<0, 8> a;
        BitField<8, 16> r;
        BitField<16, 24> g;
        BitField<24, 32> b;
    } c = { color };
    _context->colorClearValue = { c.r.u() / 255., c.g.u() / 255., c.b.u() / 255., c.a.u() / 255. };
}

void Rsx::ClearSurface(uint32_t mask) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ClearSurface(%x)", mask);
    assert(mask & CELL_GCM_CLEAR_R);
    assert(mask & CELL_GCM_CLEAR_G);
    assert(mask & CELL_GCM_CLEAR_B);
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexDataArrayFormat(%x, %x, %x, %x, %x)",
        index, frequency, stride, size, type);
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexDataArrayOffset(%x, %x, %x)",
        index, location, offset);
    auto& array = _context->vertexDataArrays[index];
    array.location = location == CELL_GCM_LOCATION_LOCAL ?
        MemoryLocation::Local : MemoryLocation::Main;
    array.offset = offset;
    array.binding = index;
    GLint maxBindings;
    assert((GLint)array.binding < (glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &maxBindings), maxBindings));
    assert(location == CELL_GCM_LOCATION_LOCAL);
}

void Rsx::BeginEnd(uint32_t mode) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("BeginEnd(%x)", mode);
    // opengl deprecated primitives
    assert(mode != CELL_GCM_PRIMITIVE_QUADS);
    assert(mode != CELL_GCM_PRIMITIVE_QUAD_STRIP);
    assert(mode != CELL_GCM_PRIMITIVE_POLYGON);
    _context->glVertexArrayMode = 
        mode == CELL_GCM_PRIMITIVE_POINTS ? GL_POINTS :
        mode == CELL_GCM_PRIMITIVE_LINES ? GL_LINES :
        mode == CELL_GCM_PRIMITIVE_LINE_LOOP ? GL_LINE_LOOP :
        mode == CELL_GCM_PRIMITIVE_LINE_STRIP ? GL_LINE_STRIP :
        mode == CELL_GCM_PRIMITIVE_TRIANGLES ? GL_TRIANGLES :
        mode == CELL_GCM_PRIMITIVE_TRIANGLE_STRIP ? GL_TRIANGLE_STRIP :
        GL_TRIANGLE_FAN;
}

void Rsx::DrawArrays(unsigned first, unsigned count) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("DrawArrays(%d, %d)", first, count);
    updateVertexDataArrays(first, count);
    updateTextures();
    updateShaders();
    
    _context->bufferCache.watch([=](uint32_t va, uint32_t size) { _ppu->memoryBreak(va, size); });
    _context->textureCache.watch([=](uint32_t va, uint32_t size) { _ppu->memoryBreak(va, size); });
    
    glcall(glDrawArrays(_context->glVertexArrayMode, 0, count));
}

bool Rsx::isFlipInProgress() const {
    boost::unique_lock<boost::mutex> lock(_mutex);
    return _context->isFlipInProgress;
}

void Rsx::resetFlipStatus() {
    boost::unique_lock<boost::mutex> lock(_mutex);
    _context->isFlipInProgress = true;
}

struct __attribute__ ((__packed__)) VertexShaderSamplerUniform {
    uint32_t wraps[3];
    std::array<float, 4> borderColor;
};

struct __attribute__ ((__packed__)) FragmentShaderSamplerUniform {
    uint32_t flip;
};

void Rsx::setGcmContext(uint32_t ioSize, ps3_uintptr_t ioAddress) {
    _context->gcmIoSize = ioSize;
    _context->gcmIoAddress = ioAddress;
}

void Rsx::EmuFlip(uint32_t buffer, uint32_t label, uint32_t labelValue) {
    auto va = _context->displayBuffers[buffer].offset + GcmLocalMemoryBase;
    auto tex = _context->framebuffer->findTexture(va);
    _context->framebuffer->bindDefault();
    updateFramebuffer();
    _context->textureRenderer->render(tex);
    
    _context->window.SwapBuffers();
    
    auto& vec = _context->lastFrame;
    glReadnPixels(_context->viewPort.x,
                  _context->viewPort.y,
                  _context->viewPort.width,
                  _context->viewPort.height,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  vec.size(),
                  &vec[0]);
    
    std::ofstream f("/tmp/ps3frame.rgba", std::ofstream::binary);
    f.write((const char*)vec.data(), vec.size());
    
    this->setLabel(1, 0);
    if (label != (uint32_t)-1) {
        this->setLabel(label, labelValue);
    }
    //boost::unique_lock<boost::mutex> lock(_mutex);
    _context->isFlipInProgress = false;
}

void Rsx::TransformConstantLoad(uint32_t loadAt, std::vector<uint32_t> const& vals) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TransformConstantLoad(%x, %d)", loadAt, vals.size());
    assert(vals.size() % 4 == 0);
    auto size = vals.size() * sizeof(uint32_t);
    glcall(glNamedBufferSubData(_context->vertexConstBuffer, loadAt * 16, size, vals.data()));
}

bool Rsx::linkShaderProgram() {
    if (!_context->fragmentShader.handle() || !_context->vertexShader.handle())
        return false;
    
    GLProgram program;
    glAttachShader(program.handle(), _context->vertexShader.handle());
    glAttachShader(program.handle(), _context->fragmentShader.handle());
    glcall(glLinkProgram(program.handle()));
    glcall(glUseProgram(program.handle()));
    _context->shaderProgram = std::move(program);
    
    glcall(glBindBufferBase(GL_UNIFORM_BUFFER,
                            VertexShaderConstantBinding,
                            _context->vertexConstBuffer));
    glcall(glBindBufferBase(GL_UNIFORM_BUFFER,
                            VertexShaderSamplesInfoBinding,
                            _context->vertexSamplersBuffer));
    glcall(glBindBufferBase(GL_UNIFORM_BUFFER,
                            FragmentShaderSamplesInfoBinding,
                            _context->fragmentSamplersBuffer));
    return true;
}

void Rsx::RestartIndexEnable(bool enable) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("RestartIndexEnable(%d)", enable);
    if (enable) {
        glcall(glEnable(GL_PRIMITIVE_RESTART));
    } else {
        glcall(glDisable(GL_PRIMITIVE_RESTART));
    }
}

void Rsx::RestartIndex(uint32_t index) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("RestartIndex(%x)", index);
    glcall(glPrimitiveRestartIndex(index));
}

void Rsx::IndexArrayAddress(uint8_t location, uint32_t offset, uint32_t type) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("IndexArrayAddress(%x, %x, %x)", location, offset, type);
    _context->vertexIndexArrayOffset = offset;
    _context->vertexIndexArrayGlType = GL_UNSIGNED_INT; // no difference between 32 and 16 for rsx
}

void Rsx::DrawIndexArray(uint32_t first, uint32_t count) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("DrawIndexArray(%x, %x)", first, count);
    updateVertexDataArrays(first, count);
    updateTextures();
    updateShaders();
    
    _context->bufferCache.watch([=](uint32_t va, uint32_t size) { _ppu->memoryBreak(va, size); });
    _context->textureCache.watch([=](uint32_t va, uint32_t size) { _ppu->memoryBreak(va, size); });
    
    // TODO: proper buffer management
    std::unique_ptr<uint8_t[]> copy(new uint8_t[count * 4]); // TODO: check index format
    auto va = first + _context->vertexIndexArrayOffset + GcmLocalMemoryBase;
    _ppu->readMemory(va, copy.get(), count * 4);
    auto ptr = (uint32_t*)copy.get();
    for (auto i = 0u; i < count; ++i) {
        ptr[i] = boost::endian::endian_reverse(ptr[i]);
    }
    GLuint buffer;
    glcall(glCreateBuffers(1, &buffer));
    glcall(glNamedBufferStorage(buffer, count * 4, copy.get(), 0));
    glcall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer));
    
    auto offset = (void*)(uintptr_t)first;
    glcall(glDrawElements(_context->glVertexArrayMode, count, _context->vertexIndexArrayGlType, offset));
}

void Rsx::updateShaders() {
    if (_context->fragmentShaderDirty) {
        _context->fragmentShaderDirty = false;
        
        _context->fragmentBytecode.resize(1000); // TODO: compute size exactly
        auto& vec = _context->fragmentBytecode;
        _ppu->readMemory(_context->fragmentOffset, &vec[0], vec.size());
        
        // TODO: handle sizes
        std::array<int, 16> sizes = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
        auto text = GenerateFragmentShader(_context->fragmentBytecode, sizes, _context->isFlatShadeMode);
                
        BOOST_LOG_TRIVIAL(trace) << text;
        
        _context->fragmentShader = GLShader(GLShaderType::fragment);
        const char* textptr = text.c_str();
        glShaderSource(_context->fragmentShader.handle(), 1, &textptr, NULL);
        glCompileShader(_context->fragmentShader.handle());
        check_shader(_context->fragmentShader.handle());
    }
    
    if (_context->vertexShaderDirty) {
        _context->vertexShaderDirty = false;
        std::array<int, 4> samplerSizes = { 
            _context->vertexTextureSamplers[0].texture.dimension,
            _context->vertexTextureSamplers[1].texture.dimension,
            _context->vertexTextureSamplers[2].texture.dimension,
            _context->vertexTextureSamplers[3].texture.dimension
        };
        auto text = GenerateVertexShader(_context->vertexInstructions.data(),
                                         _context->vertexInputs,
                                         samplerSizes, 0); // TODO: loadAt

        BOOST_LOG_TRIVIAL(trace) << text;
        
        GLShader vertexShader(GLShaderType::vertex);
        const char* textptr = text.c_str();
        glShaderSource(vertexShader.handle(), 1, &textptr, NULL);
        glCompileShader(vertexShader.handle());
        check_shader(vertexShader.handle());
        _context->vertexShader = std::move(vertexShader);
    }
    
    linkShaderProgram();
}

void Rsx::VertexTextureOffset(unsigned index, 
                              uint32_t offset, 
                              uint8_t mipmap,
                              uint8_t format,
                              uint8_t dimension,
                              uint8_t location)
{
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexTextureOffset(%d, %x, ...)", index, offset);
    auto& t = _context->vertexTextureSamplers[index].texture;
    t.offset = offset;
    t.mipmap = mipmap;
    t.format = format;
    t.dimension = dimension;
    assert(location == CELL_GCM_LOCATION_LOCAL || location == CELL_GCM_LOCATION_MAIN);
    t.location = location == CELL_GCM_LOCATION_MAIN ? MemoryLocation::Main : MemoryLocation::Local;
}

void Rsx::VertexTextureControl3(unsigned index, uint32_t pitch) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexTextureControl3(%d, %x)", index, pitch);
    _context->vertexTextureSamplers[index].texture.pitch = pitch;
}

void Rsx::VertexTextureImageRect(unsigned index, uint16_t width, uint16_t height) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexTextureImageRect(%d, %d, %d)", index, width, height);
    _context->vertexTextureSamplers[index].texture.width = width;
    _context->vertexTextureSamplers[index].texture.height = height;
}

void Rsx::VertexTextureControl0(unsigned index, bool enable, float minlod, float maxlod) {
    BOOST_LOG_TRIVIAL(trace) << 
        ssnprintf("VertexTextureControl0(%d, %d, %x, %x) [vertex]", index, enable, minlod, maxlod);
    _context->vertexTextureSamplers[index].enable = enable;
    _context->vertexTextureSamplers[index].minlod = minlod;
    _context->vertexTextureSamplers[index].maxlod = maxlod;
}

void Rsx::VertexTextureAddress(unsigned index, uint8_t wraps, uint8_t wrapt) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexTextureAddress(%d, %x, %x)", index, wraps, wrapt);
    _context->vertexTextureSamplers[index].wraps = wraps;
    _context->vertexTextureSamplers[index].wrapt = wrapt;
}

void Rsx::VertexTextureFilter(unsigned int index, float bias) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexTextureFilter(%d, %x)", index, bias);
    _context->vertexTextureSamplers[index].bias = bias;
}

void Rsx::updateTextures() {
    int index = 0;
    for (auto& sampler : _context->vertexTextureSamplers) {
        if (sampler.enable) {
            auto& info = sampler.texture;
            TextureCacheKey key { 
                addressToMainMemory(info.location, info.offset),
                info.width,
                info.height,
                info.format
            };
            auto texture = _context->textureCache.retrieve(key);
            if (!texture) {
                CacheItemUpdater<GLTexture> updater { 
                    key.va, info.height * info.pitch,
                    [=](auto t) {
                        t->update(_ppu);
                    }
                };
                texture = new GLTexture(_ppu, sampler.texture);
                _context->textureCache.insert(key, texture, updater);
            }
            
            texture->bind(index);
            auto handle = sampler.glSampler.handle();
            if (!handle) {
                sampler.glSampler = GLSampler();
                handle = sampler.glSampler.handle();
                glcall(glBindSampler(index, handle));
            }
            glSamplerParameterf(handle, GL_TEXTURE_MIN_LOD, sampler.minlod);
            glSamplerParameterf(handle, GL_TEXTURE_MAX_LOD, sampler.maxlod);
            glSamplerParameterf(handle, GL_TEXTURE_LOD_BIAS, sampler.bias);
            glSamplerParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glSamplerParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glSamplerParameteri(handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glSamplerParameteri(handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            VertexShaderSamplerUniform uniform = { 
                sampler.wraps, sampler.wrapt, 0, sampler.borderColor
            };
            auto uniformSize = sizeof(uniform);
            glcall(glNamedBufferSubData(_context->vertexSamplersBuffer, 
                                        index * uniformSize, 
                                        uniformSize, 
                                        &uniform));
        }
        index++;
    }
    int fragmentIndex = 0;
    for (auto& sampler : _context->fragmentTextureSamplers) {
        if (sampler.enable) {
            auto va = addressToMainMemory(sampler.texture.location, sampler.texture.offset);
            auto surfaceTex = _context->framebuffer->findTexture(va);
            if (surfaceTex) {
                glcall(glBindTextureUnit(index, surfaceTex->handle()));
            } else {
                auto& info = sampler.texture;
                TextureCacheKey key { 
                    addressToMainMemory(info.location, info.offset),
                    info.width,
                    info.height,
                    info.format
                };
                auto texture = _context->textureCache.retrieve(key);
                if (!texture) {
                    CacheItemUpdater<GLTexture> updater { 
                        key.va, info.height * info.pitch,
                        [=](auto t) {
                            t->update(_ppu);
                        }
                    };
                    texture = new GLTexture(_ppu, sampler.texture);
                    _context->textureCache.insert(key, texture, updater);
                }
                texture->bind(index);
            }
            
            auto handle = sampler.glSampler.handle();
            if (!handle) {
                sampler.glSampler = GLSampler();
                handle = sampler.glSampler.handle();
                glcall(glBindSampler(index, handle));
            }
            glSamplerParameterf(handle, GL_TEXTURE_MIN_LOD, sampler.minlod);
            glSamplerParameterf(handle, GL_TEXTURE_MAX_LOD, sampler.maxlod);
            glSamplerParameterf(handle, GL_TEXTURE_LOD_BIAS, sampler.bias);
            glSamplerParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glSamplerParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glSamplerParameteri(handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glSamplerParameteri(handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            // TODO: handle viewport CELL_GCM_WINDOW_ORIGIN_TOP
            FragmentShaderSamplerUniform info = { 
                surfaceTex != nullptr
            };
            auto infoSize = sizeof(info);
            glcall(glNamedBufferSubData(_context->fragmentSamplersBuffer, fragmentIndex * infoSize, infoSize, &info));
        }
        fragmentIndex++;
        index++;
    }
}

void Rsx::init() {
    BOOST_LOG_TRIVIAL(trace) << "waiting for rsx loop to initialize";
    
    _thread.reset(new boost::thread([=]{ loop(); }));
    
    // lock the thread until Rsx has initialized the buffer
    boost::unique_lock<boost::mutex> lock(_initMutex);
    _initCv.wait(lock, [=] { return _initialized; });
    
    BOOST_LOG_TRIVIAL(trace) << "rsx loop completed initialization";
}

void Rsx::VertexTextureBorderColor(unsigned int index, std::array<float, int(4)> argb) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexTextureBorderColor(%d, ...)", index);
    _context->vertexTextureSamplers[index].borderColor = argb;
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureAddress(%d, ...)", index);
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

void Rsx::TextureBorderColor(unsigned index, std::array<float, int(4)> argb) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureBorderColor(%d, ...)", index);
    _context->vertexTextureSamplers[index].borderColor = argb;
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureFilter(%d, ...)", index);
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureOffset(...)");
    auto& t = _context->fragmentTextureSamplers[index].texture;
    t.offset = offset;
    t.mipmap = mipmap;
    t.format = format;
    t.dimension = dimension;
    t.fragmentBorder = border;
    t.fragmentCubemap = cubemap;
    assert(location == CELL_GCM_LOCATION_LOCAL || location == CELL_GCM_LOCATION_MAIN);
    t.location = location == CELL_GCM_LOCATION_MAIN ? MemoryLocation::Main : MemoryLocation::Local;
}

void Rsx::TextureImageRect(unsigned int index, uint16_t width, uint16_t height) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureImageRect(%d, %d, %d)", index, width, height);
    _context->fragmentTextureSamplers[index].texture.width = width;
    _context->fragmentTextureSamplers[index].texture.height = height;
}

void Rsx::TextureControl3(unsigned int index, uint16_t depth, uint32_t pitch) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureControl3(%d, %d, %d)", index, depth, pitch);
    _context->fragmentTextureSamplers[index].texture.pitch = pitch;
    _context->fragmentTextureSamplers[index].texture.fragmentDepth = depth;
}

void Rsx::TextureControl1(unsigned int index, uint32_t remap) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureControl1(%d, %x)", index, remap);
    _context->fragmentTextureSamplers[index].texture.fragmentRemapCrossbarSelect = remap;
}

void Rsx::TextureControl2(unsigned int index, uint8_t slope, bool iso, bool aniso) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureControl2(%d, %d, %d, %d)", index, slope, iso, aniso);
    // ignore these optimizations
}

void Rsx::TextureControl0(unsigned index,
                          uint8_t alphaKill,
                          uint8_t maxaniso,
                          float maxlod,
                          float minlod,
                          bool enable) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureControl0(%d, ...)", index);
    // ignore maxaniso
    _context->fragmentTextureSamplers[index].fragmentAlphaKill = alphaKill;
    _context->fragmentTextureSamplers[index].enable = enable;
    _context->fragmentTextureSamplers[index].minlod = minlod;
    _context->fragmentTextureSamplers[index].maxlod = maxlod;
}

void Rsx::SetReference(uint32_t ref) {
    _ref = ref;
}

// Surface

void Rsx::SurfaceCompression(uint32_t x) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfaceCompression(%d)", x);
}

void Rsx::setSurfaceColorLocation(unsigned index, uint32_t location) {
    assert(index < 4);
    location -= CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
    if (location == CELL_GCM_LOCATION_LOCAL) {
        _context->surface.colorLocation[index] = MemoryLocation::Local;   
    } else if (location == CELL_GCM_LOCATION_MAIN) {
        _context->surface.colorLocation[index] = MemoryLocation::Main;
    } else {
        assert(false);
    }
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfaceFormat(%x, ...)", colorFormat);
    assert(colorFormat == CELL_GCM_SURFACE_A8R8G8B8);
    if (depthFormat == CELL_GCM_SURFACE_Z16) {
        _context->surface.depthFormat = SurfaceDepthFormat::z16;
    } else if (depthFormat == CELL_GCM_SURFACE_Z24S8) {
        _context->surface.depthFormat = SurfaceDepthFormat::z24s8;
    } else {
        assert(false);
    }
    assert(antialias == CELL_GCM_SURFACE_CENTER_1);
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfacePitchZ(%d)", pitch);
    _context->surface.depthPitch = pitch;
}

void Rsx::SurfacePitchC(uint32_t pitchC, uint32_t pitchD, uint32_t offsetC, uint32_t offsetD) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfacePitchC(%d, %d, %d, %d)",
                                          pitchC, pitchD, offsetC, offsetD);
    _context->surface.colorPitch[2] = pitchC;
    _context->surface.colorPitch[3] = pitchD;
    _context->surface.colorOffset[2] = offsetC;
    _context->surface.colorOffset[3] = offsetD;
}

void Rsx::SurfaceColorTarget(uint32_t target) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfaceColorTarget(%x)", target);
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("WindowOffset(%d, %d)", x, y);
    _context->surface.windowOriginX = x;
    _context->surface.windowOriginY = y;
}

void Rsx::SurfaceClipHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfaceClipHorizontal(%d, %d, %d, %d)", x, w, y, h);
    _context->viewPort.x = x;
    _context->viewPort.y = y;
    _context->viewPort.width = w;
    _context->viewPort.height = h;
    _context->lastFrame.resize(w * h * 4);
}

// assume cellGcmSetSurface is always used and not its subcommands
// then this command is always set last
void Rsx::ShaderWindow(uint16_t height, uint8_t origin, uint16_t pixelCenters) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ShaderWindow(%d, %x, %x)", height, origin, pixelCenters);
    assert(origin == CELL_GCM_WINDOW_ORIGIN_BOTTOM);
    assert(pixelCenters == CELL_GCM_WINDOW_PIXEL_CENTER_HALF);
    waitForIdle();
    _context->framebuffer->setSurface(_context->surface, _context->viewPort);
    updateFramebuffer();
}

void Rsx::Control0(uint32_t format) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("Control0(%x)", format);
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ContextDmaColorA(%x)", context);
    setSurfaceColorLocation(0, context);
}

void Rsx::ContextDmaColorB(uint32_t context) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ContextDmaColorB(%x)", context);
    setSurfaceColorLocation(1, context);
}

void Rsx::ContextDmaColorC(uint32_t contextC, uint32_t contextD) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ContextDmaColorC(%x, %x)", contextC, contextD);
    setSurfaceColorLocation(2, contextC);
    setSurfaceColorLocation(3, contextD);
}

void Rsx::ContextDmaColorD(uint32_t context) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ContextDmaColorD(%x)", context);
    setSurfaceColorLocation(3, context);
}

void Rsx::ContextDmaColorC(uint32_t contextC) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ContextDmaColorC(%x)", contextC);
    setSurfaceColorLocation(2, contextC);
}

void Rsx::ContextDmaZeta(uint32_t context) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ContextDmaZeta(%x)", context);
    auto local = context - CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER == CELL_GCM_LOCATION_LOCAL;
    _context->surface.depthLocation = local ? MemoryLocation::Local : MemoryLocation::Main;
}

void Rsx::setDisplayBuffer(uint8_t id, uint32_t offset, uint32_t pitch, uint32_t width, uint32_t height) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("setDisplayBuffer(%x, ...)", id);
    auto& buffer = _context->displayBuffers[id];
    buffer.offset = offset;
    buffer.pitch = pitch;
    buffer.width = width;
    buffer.height = height;
}

void Rsx::initGcm() {
    BOOST_LOG_TRIVIAL(trace) << "initializing rsx";
    
    _ppu->memoryBreakHandler([=](uint32_t va, uint32_t size) { memoryBreakHandler(va, size); });
    _context->window.Init();
    _context->localMemory.reset(new uint8_t[GcmLocalMemorySize]);
    _ppu->provideMemory(GcmLocalMemoryBase, GcmLocalMemorySize, _context->localMemory.get());
    
    // remap io to point to the buffer as well
    _ppu->map(_context->gcmIoAddress, GcmLocalMemoryBase, _context->gcmIoSize);
    
    glcall(glCreateBuffers(1, &_context->vertexConstBuffer));
    size_t constBufferSize = VertexShaderConstantCount * sizeof(float) * 4;
    glcall(glNamedBufferStorage(_context->vertexConstBuffer, constBufferSize, NULL, 
                                GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT));
    
    glcall(glCreateBuffers(1, &_context->vertexSamplersBuffer));
    auto uniformSize = sizeof(VertexShaderSamplerUniform) * 4;
    glcall(glNamedBufferStorage(_context->vertexSamplersBuffer, uniformSize, NULL, 
                                GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT));
    
    glcall(glCreateBuffers(1, &_context->fragmentSamplersBuffer));
    uniformSize = sizeof(FragmentShaderSamplerUniform) * 16;
    glcall(glNamedBufferStorage(_context->fragmentSamplersBuffer, uniformSize, NULL, 
                                GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT));
    
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

void Rsx::updateFramebuffer() {
    glcall(glViewport(_context->viewPort.x,
                      _context->viewPort.y,
                      _context->viewPort.width,
                      _context->viewPort.height));
    auto& c = _context->colorClearValue;
    glcall(glClearColor(c.r, c.g, c.b, c.a));
    glcall(glClear(_context->glClearSurfaceMask));
}

void Rsx::updateVertexDataArrays(unsigned first, unsigned count) {
    for (auto i = 0u; i < _context->vertexDataArrays.size(); ++i) {
        auto& format = _context->vertexDataArrays[i];
        auto& input = _context->vertexInputs[i];
        if (!input.enabled)
            continue;
   
        auto va = addressToMainMemory(MemoryLocation::Local, format.offset);
        auto bufferVa = va + first * format.stride;
        auto bufferSize = count * format.stride;
        
        BufferCacheKey key { bufferVa, bufferSize };
        GLBuffer* buffer = _context->bufferCache.retrieve(key);
        if (!buffer) {
            CacheItemUpdater<GLBuffer> updater{ bufferVa, bufferSize, [=](auto b) {
                auto mapped = b->map();
                _ppu->readMemory(bufferVa, mapped, bufferSize);
                b->unmap();
            }};
            buffer = new GLBuffer(GLBufferType::MapWrite, bufferSize);
            _context->bufferCache.insert(key, buffer, updater);
        }
        
        glcall(glVertexAttribBinding(i, format.binding));
        glcall(glBindVertexBuffer(format.binding, buffer->handle(), 0, format.stride));
    }
}

void Rsx::waitForIdle() {
    glcall(glFinish());
}

void Rsx::BackEndWriteSemaphoreRelease(uint32_t value) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("BackEndWriteSemaphoreRelease(%x)", value);
    waitForIdle();
    _ppu->store<4>(_context->semaphoreOffset + GcmLabelBaseOffset, value);
    __sync_synchronize();
}

void Rsx::SemaphoreOffset(uint32_t offset) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SemaphoreOffset(%x)", offset);
    _context->semaphoreOffset = offset;
}

void Rsx::OffsetDestin(uint32_t offset) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("OffsetDestin(%x)", offset);
    _context->transfer.offsetDestin = offset;
}

void Rsx::ColorFormat(uint32_t format, uint16_t dstPitch, uint16_t srcPitch) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ColorFormat(%x, %x, %x)", format, dstPitch, srcPitch);
    _context->transfer.destPitch = dstPitch;
    _context->transfer.sourcePitch = srcPitch;
}

void Rsx::Point(uint16_t pointX, 
                uint16_t pointY, 
                uint16_t outSizeX, 
                uint16_t outSizeY, 
                uint16_t inSizeX, 
                uint16_t inSizeY)
{
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("Point(%d, %d, %d, %d, %d, %d)",
        pointX, pointY, outSizeX, outSizeX, inSizeX, inSizeY
    );
    _context->transfer.pointX = pointX;
    _context->transfer.pointY = pointY;
    _context->transfer.outSizeX = outSizeX;
    _context->transfer.outSizeY = outSizeY;
    _context->transfer.inSizeX = inSizeX;
    _context->transfer.inSizeY = inSizeY;
}

void Rsx::Color(std::vector<uint32_t> const& vec) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("Color(size=%d)", vec.size());
    // TODO: calc image
    auto pos = _context->transfer.pointX * 4;
    for (auto v : vec) {
        _ppu->store<4>(_context->transfer.offsetDestin + GcmLocalMemoryBase + pos, v);
        pos += 4;
    }
}

void Rsx::memoryBreakHandler(uint32_t va, uint32_t size) {
    _context->bufferCache.invalidate(va, size);
    _context->textureCache.invalidate(va, size);
}

bool Rsx::isCallActive() {
    return _ret != 0;
}

uint32_t Rsx::getGet() {
    return _get;
}
