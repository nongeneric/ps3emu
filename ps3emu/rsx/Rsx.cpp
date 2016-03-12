#include "Rsx.h"

#include "GLBuffer.h"
#include "Cache.h"
#include "GLFramebuffer.h"
#include "GLTexture.h"
#include "../gcmviz/GcmDatabase.h"
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
#include "../../libs/graphics/graphics.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

template<typename T> struct GcmType { static constexpr uint32_t type = (uint32_t)GcmArgType::None; };
template<> struct GcmType<uint8_t> { static constexpr uint32_t type = (uint32_t)GcmArgType::UInt8; };
template<> struct GcmType<uint16_t> { static constexpr uint32_t type = (uint32_t)GcmArgType::UInt16; };
template<> struct GcmType<uint32_t> { static constexpr uint32_t type = (uint32_t)GcmArgType::UInt32; };
template<> struct GcmType<float> { static constexpr uint32_t type = (uint32_t)GcmArgType::Float; };
template<> struct GcmType<bool> { static constexpr uint32_t type = (uint32_t)GcmArgType::Bool; };
template<> struct GcmType<int32_t> { static constexpr uint32_t type = (uint32_t)GcmArgType::Int32; };
template<> struct GcmType<int16_t> { static constexpr uint32_t type = (uint32_t)GcmArgType::Int16; };

template <typename T> struct DbColumnType {
    static uint32_t convert(T value) {
        return value;
    }
};

template <> struct DbColumnType<float> {
    static uint32_t convert(float value) {
        return union_cast<float, uint32_t>(value);
    }
};

/*
def a(i):
     r = "#define TRACE" + str(i) + "(id"
     for n in range(0, i): r += ", a" + str(n)
     r += ") trace(CommandId::id, { ARG(a0)"
     for n in range(1, i): r += ", ARG(a" + str(n) + ")"
     r += " });"
     print(r)
for z in range(1, 14): a(z) 
*/

#define ARG(a) GcmCommandArg { DbColumnType<decltype(a)>::convert(a), \
                               #a, GcmType<decltype(a)>::type }
#define TRACE0(id) trace(CommandId::id);
#define TRACE1(id, a0) trace(CommandId::id, { ARG(a0) });
#define TRACE2(id, a0, a1) trace(CommandId::id, { ARG(a0), ARG(a1) });
#define TRACE3(id, a0, a1, a2) trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2) });
#define TRACE4(id, a0, a1, a2, a3) trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3) });
#define TRACE5(id, a0, a1, a2, a3, a4) trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4) });
#define TRACE6(id, a0, a1, a2, a3, a4, a5) trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5) });
#define TRACE7(id, a0, a1, a2, a3, a4, a5, a6) trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6) });
#define TRACE8(id, a0, a1, a2, a3, a4, a5, a6, a7) trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7) });
#define TRACE9(id, a0, a1, a2, a3, a4, a5, a6, a7, a8) trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7), ARG(a8) });
#define TRACE10(id, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7), ARG(a8), ARG(a9) });
#define TRACE11(id, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7), ARG(a8), ARG(a9), ARG(a10) });
#define TRACE12(id, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7), ARG(a8), ARG(a9), ARG(a10), ARG(a11) });
#define TRACE13(id, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) trace(CommandId::id, { ARG(a0), ARG(a1), ARG(a2), ARG(a3), ARG(a4), ARG(a5), ARG(a6), ARG(a7), ARG(a8), ARG(a9), ARG(a10), ARG(a11), ARG(a12) });

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

void deleteProgramPipeline(GLuint handle) {
    glDeleteProgramPipelines(1, &handle);
}

template <GLuint Type>
class Shader {
    GLProgram _program;
    GLuint create(const char* text) {
        return glCreateShaderProgramv(Type, 1, &text);
    }
public:
    Shader() : _program(0) { }
    Shader(const char* text) : _program(create(text)) { }
    GLuint handle() {
        return _program.handle();
    }
    std::string log() {
        GLint len;
        glcall(glGetProgramiv(_program.handle(), GL_INFO_LOG_LENGTH, &len));
        std::string str;
        str.resize(len);
        glcall(glGetProgramInfoLog(_program.handle(), len, nullptr, &str[0]));
        return str;   
    }
};

class VertexShader : public Shader<GL_VERTEX_SHADER> {
public:
    VertexShader() { }
    VertexShader(const char* text) : Shader(text) { }
};

class FragmentShader : public Shader<GL_FRAGMENT_SHADER> {
public:
    FragmentShader() { }
    FragmentShader(const char* text) : Shader(text) { }
};

class GLProgramPipeline : public HandleWrapper<GLuint, deleteProgramPipeline> {
    GLuint create() {
        GLuint handle = 0;
        glcall(glGenProgramPipelines(1, &handle));
        return handle;
    }
public:
    GLProgramPipeline() : HandleWrapper(create()) { }
    void bind() {
        glcall(glBindProgramPipeline(handle()));
    }
    void useShader(VertexShader& shader) {
        glcall(glUseProgramStages(handle(), GL_VERTEX_SHADER_BIT, shader.handle()));
    }
    void useShader(FragmentShader& shader) {
        glcall(glUseProgramStages(handle(), GL_FRAGMENT_SHADER_BIT, shader.handle()));
    }
    void validate() {
        glcall(glValidateProgramPipeline(handle()));
    }
};

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

struct VertexShaderCacheKey {
    std::vector<uint8_t> bytecode;
    inline bool operator<(VertexShaderCacheKey const& other) const {
        auto size = bytecode.size();
        auto othersize = other.bytecode.size();
        return std::tie(size, bytecode)
             < std::tie(othersize, other.bytecode);
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

class Tracer {
    bool _enabled = false;
    GcmDatabase _db;
public:
    void enable() {
        _enabled = true;
        system("rm -rf /tmp/ps3emu.trace");
        _db.createOrOpen("/tmp/ps3emu.trace");
    }
    
    template <typename... Args>
    void trace(uint32_t frame,
               uint32_t num,
               CommandId command,
               std::vector<GcmCommandArg> args) {
        if (!_enabled)
            return;
        GcmCommand gcmCommand;
        gcmCommand.frame = frame;
        gcmCommand.num = num;
        gcmCommand.id = (uint32_t)command;
        gcmCommand.args = args;
        _db.insertCommand(gcmCommand);
    }
    
    template <typename... Args>
    void traceWithBlob(CommandId command,
                       std::vector<uint8_t> const& blob,
                       Args... args) {
        
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
    bool isFlipInProgress = false;
    std::array<VertexDataArrayFormatInfo, 16> vertexDataArrays;
    GLuint glVertexArrayMode;
    VertexShader* vertexShader = nullptr;
    FragmentShader* fragmentShader = nullptr;
    GLBuffer vertexConstBuffer;
    GLBuffer vertexSamplersBuffer;
    GLBuffer vertexViewportBuffer;
    GLBuffer fragmentSamplersBuffer;
    bool vertexShaderDirty = false;
    bool fragmentShaderDirty = false;
    uint32_t fragmentVa = 0;
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
    GLProgramPipeline pipeline;
    Cache<BufferCacheKey, GLBuffer, 256 * (2 >> 20)> bufferCache;
    Cache<TextureCacheKey, GLTexture, 256 * (2 >> 20)> textureCache;
    Cache<VertexShaderCacheKey, VertexShader, 10 * (2 >> 20)> vertexShaderCache;
    Cache<FragmentShaderCacheKey, FragmentShader, 10 * (2 >> 20), FragmentShaderUpdateFunctor> fragmentShaderCache;
    uint32_t vBlankHandlerDescr = 0;
    MemoryLocation reportLocation = MemoryLocation::Local;
};

Rsx::Rsx() = default;

Rsx::~Rsx() {
    shutdown();
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

class TextureRenderer {
    GLProgramPipeline _pipeline;
    VertexShader _vertexShader;
    FragmentShader _fragmentShader;
    GLSampler _sampler;
    GLBuffer _buffer;
public:
    TextureRenderer() {
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
            "out gl_PerVertex {\n"
            "    vec4 gl_Position;\n"
            "};\n"
            "void main(void) {\n"
            "    gl_Position = vec4(pos, 0, 1);\n"
            "}\n";
        
        _fragmentShader = FragmentShader(fragmentCode);
        _vertexShader = VertexShader(vertexCode);
        _pipeline.useShader(_fragmentShader);
        _pipeline.useShader(_vertexShader);
        
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
        _pipeline.bind();
        
        glcall(glViewport(0, 0, tex->width(), tex->height()));
        glcall(glDepthRange(0, 1));
        glcall(glDrawArrays(GL_TRIANGLES, 0, 6));
        
        // TODO: restore vertex attrib if required
    }
};

void Rsx::setLabel(int index, uint32_t value) {
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
    //TODO: implement
}

void Rsx::AlphaTestEnable(bool enable) {
    TRACE1(AlphaTestEnable, enable);
    //TODO: implement
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
    TRACE2(TransformProgram, locationOffset, size);
    auto bytes = size * 4;
    auto src = _mm->getMemoryPointer(rsxOffsetToEa(MemoryLocation::Main, locationOffset), bytes);
    memcpy(&_context->vertexInstructions[_context->vertexLoadOffset], src, bytes);
    _context->vertexShaderDirty = true;
    _context->vertexLoadOffset += bytes;
}

void Rsx::VertexAttribInputMask(uint32_t mask) {
    TRACE1(VertexAttribInputMask, mask);
}

void Rsx::TransformTimeout(uint16_t count, uint16_t registerCount) {
    TRACE2(TransformTimeout, count, registerCount);
}

void Rsx::ShaderProgram(uint32_t locationOffset) {
    TRACE1(ShaderProgram, locationOffset);
    // loads fragment program byte code from locationOffset-1 up to the last command
    // (with the "#last command" bit)
    locationOffset -= CELL_GCM_LOCATION_MAIN;
    auto ea = rsxOffsetToEa(MemoryLocation::Local, locationOffset);
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
    if (enable) {
        glcall(glEnable(GL_DEPTH_TEST));
    } else {
        glcall(glDisable(GL_DEPTH_TEST));
    }
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
    assert(!enable);
    _context->isCullFaceEnabled = enable;
    if (enable) {
        glcall(glEnable(GL_CULL_FACE));
    } else {
        glcall(glDisable(GL_CULL_FACE));
    }
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
    _context->colorClearValue = { c.r.u() / 255., c.g.u() / 255., c.b.u() / 255., c.a.u() / 255. };
}

void Rsx::ClearSurface(uint32_t mask) {
    TRACE1(ClearSurface, mask);
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
    TRACE2(DrawArrays, first, count);
    updateVertexDataArrays(first, count);
    updateShaders();
    updateTextures();
    watchCaches();
    linkShaderProgram();
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
    std::array<uint32_t, 4> wraps[4];
    std::array<float, 4> borderColor[4];
};

struct __attribute__ ((__packed__)) FragmentShaderSamplerUniform {
    uint32_t flip[16];
};

struct __attribute__ ((__packed__)) VertexShaderViewportUniform {
    glm::mat4 glInverseGcm;
};

void Rsx::setGcmContext(uint32_t ioSize, ps3_uintptr_t ioAddress) {
    _gcmIoSize = ioSize;
    _gcmIoAddress = ioAddress;
}

void Rsx::EmuFlip(uint32_t buffer, uint32_t label, uint32_t labelValue) {
    auto va = _context->displayBuffers[buffer].offset + RsxFbBaseAddr;
    auto tex = _context->framebuffer->findTexture(va);
    _context->framebuffer->bindDefault();
    updateFramebuffer();
    _context->textureRenderer->render(tex);
    _context->pipeline.bind();
    _window.SwapBuffers();
    _context->frame++;
    _context->commandNum = 0;
    
    if (_context->vBlankHandlerDescr) {
        fdescr descr;
        _mm->readMemory(_context->vBlankHandlerDescr, &descr, sizeof(descr));
        auto future = _proc->getCallbackThread()->schedule({1}, descr.tocBase, descr.va);
        future.get();
    }

#if TESTS
    static int framenum = 0;
    
    if (framenum < 10) {
        auto& vec = _context->lastFrame;
        vec.resize(_window.width() * _window.height() * 4);
        glcall(glReadPixels(
            0,
            0,
            _window.width(),
            _window.height(),
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            &vec[0]));
        std::ofstream f(ssnprintf("/tmp/ps3frame%d.rgba", framenum));
        assert(f.is_open());
        f.write((const char*)vec.data(), vec.size());
        framenum++;
    }
#endif
    
    this->setLabel(1, 0);
    if (label != (uint32_t)-1) {
        this->setLabel(label, labelValue);
    }
    //boost::unique_lock<boost::mutex> lock(_mutex);
    _context->isFlipInProgress = false;
    
    // PlatformDevice.cpp of jsgcm says so
    _context->reportLocation = MemoryLocation::Local;
}

void Rsx::TransformConstantLoad(uint32_t loadAt, std::vector<uint32_t> const& vals) {
    TRACE1(TransformConstantLoad, loadAt);
    assert(vals.size() % 4 == 0);
    auto size = vals.size() * sizeof(uint32_t);
    glcall(glNamedBufferSubData(_context->vertexConstBuffer.handle(), loadAt * 16, size, vals.data()));
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
    if (enable) {
        glcall(glEnable(GL_PRIMITIVE_RESTART));
    } else {
        glcall(glDisable(GL_PRIMITIVE_RESTART));
    }
}

void Rsx::RestartIndex(uint32_t index) {
    TRACE1(RestartIndex, index);
    glcall(glPrimitiveRestartIndex(index));
}

void Rsx::IndexArrayAddress(uint8_t location, uint32_t offset, uint32_t type) {
    TRACE3(IndexArrayAddress, location, offset, type);
    _context->vertexIndexArrayOffset = offset;
    _context->vertexIndexArrayGlType = GL_UNSIGNED_INT; // no difference between 32 and 16 for rsx
}

void Rsx::DrawIndexArray(uint32_t first, uint32_t count) {
    TRACE2(DrawIndexArray, first, count);
    updateVertexDataArrays(first, count);
    updateTextures();
    updateShaders();
    watchCaches();
    linkShaderProgram();
    
    // TODO: proper buffer management
    std::unique_ptr<uint8_t[]> copy(new uint8_t[count * 4]); // TODO: check index format
    auto va = first + _context->vertexIndexArrayOffset + RsxFbBaseAddr;
    _mm->readMemory(va, copy.get(), count * 4);
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

class FragmentShaderUpdateFunctor {
    GLBuffer _constBuffer;
    std::vector<uint8_t> _bytecode;
    std::vector<uint8_t> _newbytecode;
    FragmentProgramInfo _info;
    RsxContext* _rsxContext;
    MainMemory* _mm;
    
    void updateBytecode(FragmentShader* shader) {
        auto dasm = PrintFragmentProgram(_newbytecode.data());
        BOOST_LOG_TRIVIAL(trace) << "fragment program\n" << dasm;
        // TODO: handle sizes
        std::array<int, 16> sizes = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
        auto text = GenerateFragmentShader(_newbytecode, sizes, _rsxContext->isFlatShadeMode);
        BOOST_LOG_TRIVIAL(trace) << text;
        *shader = FragmentShader(text.c_str());
        BOOST_LOG_TRIVIAL(trace) << "fragment shader log:\n" << shader->log();
        
        _bytecode = _newbytecode;
        _info = get_fragment_bytecode_info(&_bytecode[0]);
        updateConsts();
    }
    
    void updateConsts() {
        auto fconst = (std::array<float, 4>*)_constBuffer.map();
        for (auto i = 0u; i < _info.length; i += 16) {
            if (_info.constMap[i / 16]) {
                *fconst = read_fragment_imm_val(&_newbytecode[i]);
                fconst++;
            }
        }
        _constBuffer.unmap();
    }
    
public:
    uint32_t va;
    uint32_t size;
    
    FragmentShaderUpdateFunctor(uint32_t va, 
                                uint32_t size,
                                std::vector<uint8_t>&& bytecode,
                                RsxContext* rsxContext,
                                MainMemory* mm)
        : _constBuffer(GLBufferType::MapWrite, size / 2),
          _newbytecode(bytecode),
          _rsxContext(rsxContext),
          _mm(mm),
          va(va), size(size)
    {
        assert(size % 16 == 0);
        _info = get_fragment_bytecode_info(&_newbytecode[0]);
    }
    
    void update(FragmentShader* shader) {
        if (_bytecode.empty()) { // first update
            updateBytecode(shader);
            return;
        }
        _mm->readMemory(va, &_newbytecode[0], size);
        bool constsChanged = false;
        bool bytecodeChanged = false;
        for (auto i = 0u; i < size; i += 16) {
            auto equals = memcmp(&_bytecode[i], &_newbytecode[i], 16) == 0;
            if (!equals) {
                if (_info.constMap[i / 16]) {
                    constsChanged = true;
                } else {
                    bytecodeChanged = true;
                }
            }
        }
        if (bytecodeChanged) {
            updateBytecode(shader);
        } else if (constsChanged) {
            updateConsts();
        }
    }
    
    void bindConstBuffer() {
        glcall(glBindBufferBase(GL_UNIFORM_BUFFER,
                                FragmentShaderConstantBinding,
                                _constBuffer.handle()));
    }
};

void Rsx::updateShaders() {
    if (_context->fragmentShaderDirty) {
        _context->fragmentShaderDirty = false;
        
        auto& vec = _context->fragmentBytecode;
        vec.resize(512 * 16);
        _mm->readMemory(_context->fragmentVa, &vec[0], vec.size());
        
        auto len = get_fragment_bytecode_length(&vec[0]);
        FragmentShaderCacheKey key { _context->fragmentVa, len };
        FragmentShaderUpdateFunctor* updater;
        FragmentShader* shader;
        std::tie(shader, updater) = _context->fragmentShaderCache.retrieveWithUpdater(key);
        if (!shader) {
             shader = new FragmentShader();
             updater = new FragmentShaderUpdateFunctor(
                 _context->fragmentVa,
                 len,
                 std::vector<uint8_t>(begin(vec), begin(vec) + len),
                 _context.get(),
                 _mm
             );
             _context->fragmentShaderCache.insert(key, shader, updater);
        }
        updater->bindConstBuffer();
        _context->fragmentShader = shader;
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
        
        VertexShaderCacheKey key { std::vector<uint8_t>(_context->vertexInstructions.data(),
                                                        _context->vertexInstructions.data() + size) };
        auto shader = _context->vertexShaderCache.retrieve(key);
        if (!shader) {
            auto dasm = PrintVertexProgram(_context->vertexInstructions.data());
            BOOST_LOG_TRIVIAL(trace) << "vertex program\n" << dasm;
            auto text = GenerateVertexShader(_context->vertexInstructions.data(),
                                             _context->vertexInputs,
                                             samplerSizes,
                                             0); // TODO: loadAt
            BOOST_LOG_TRIVIAL(trace) << text;
            
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

void Rsx::updateTextures() {
    int i = 0;
    auto vertexSamplerUniform = (VertexShaderSamplerUniform*)_context->vertexSamplersBuffer.map();
    for (auto& sampler : _context->vertexTextureSamplers) {
        if (sampler.enable) {
            auto textureUnit = i + VertexTextureUnit;
            auto& info = sampler.texture;
            TextureCacheKey key { 
                rsxOffsetToEa(info.location, info.offset),
                info.width,
                info.height,
                info.format
            };
            auto texture = _context->textureCache.retrieve(key);
            if (!texture) {
                auto updater = new SimpleCacheItemUpdater<GLTexture> {
                    key.va, info.height * info.pitch,
                    [=](auto t) {
                        t->update(_mm);
                    }
                };
                texture = new GLTexture(_mm, sampler.texture);
                _context->textureCache.insert(key, texture, updater);
            }
            
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
    _context->vertexSamplersBuffer.unmap();
    
    i = 0;
    auto fragmentSamplerUniform = (FragmentShaderSamplerUniform*)_context->fragmentSamplersBuffer.map();
    for (auto& sampler : _context->fragmentTextureSamplers) {
        if (sampler.enable && sampler.texture.width && sampler.texture.height) {
            auto textureUnit = i + FragmentTextureUnit;
            auto va = rsxOffsetToEa(sampler.texture.location, sampler.texture.offset);
            auto surfaceTex = _context->framebuffer->findTexture(va);
            if (surfaceTex) {
                glcall(glBindTextureUnit(textureUnit, surfaceTex->handle()));
            } else {
                auto& info = sampler.texture;
                TextureCacheKey key { 
                    rsxOffsetToEa(info.location, info.offset),
                    info.width,
                    info.height,
                    info.format
                };
                auto texture = _context->textureCache.retrieve(key);
                if (!texture) {
                    auto updater = new SimpleCacheItemUpdater<GLTexture> {
                        key.va, info.height * info.pitch,
                        [=](auto t) {
                            t->update(_mm);
                        }
                    };
                    texture = new GLTexture(_mm, sampler.texture);
                    _context->textureCache.insert(key, texture, std::move(updater));
                }
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
    _context->fragmentSamplersBuffer.unmap();
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
    _ref = ref;
}

// Surface

void Rsx::SurfaceCompression(uint32_t x) {
    TRACE1(SurfaceCompression, x);
}

void Rsx::setSurfaceColorLocation(unsigned index, uint32_t location) {
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

void Rsx::ContextDmaColorC(uint32_t contextC, uint32_t contextD) {
    TRACE2(ContextDmaColorC_2, contextC, contextD);
    setSurfaceColorLocation(2, contextC);
    setSurfaceColorLocation(3, contextD);
}

void Rsx::ContextDmaColorD(uint32_t context) {
    TRACE1(ContextDmaColorD, context);
    setSurfaceColorLocation(3, context);
}

void Rsx::ContextDmaColorC(uint32_t contextC) {
    TRACE1(ContextDmaColorC_1, contextC);
    setSurfaceColorLocation(2, contextC);
}

void Rsx::ContextDmaZeta(uint32_t context) {
    TRACE1(ContextDmaZeta, context);
    _context->surface.depthLocation = gcmEnumToLocation(context);
}

void Rsx::setDisplayBuffer(uint8_t id, uint32_t offset, uint32_t pitch, uint32_t width, uint32_t height) {
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
    auto text = ssnprintf("gl callback [source=%s]: %s", sourceStr, message);
    if (type == GL_DEBUG_TYPE_ERROR) {
        BOOST_LOG_TRIVIAL(error) << text;
    } else {
        BOOST_LOG_TRIVIAL(trace) << text;
    }
    if (severity == GL_DEBUG_SEVERITY_HIGH)
        exit(1);
}

void Rsx::initGcm() {
    BOOST_LOG_TRIVIAL(trace) << "initializing rsx";
    
    _window.Init();
    
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(&glDebugCallbackFunction, nullptr);
    
    _context.reset(new RsxContext());
    
    if (_mode == RsxOperationMode::RunCapture) {
        _context->tracer.enable();
    }
    
    _mm->memoryBreakHandler([=](uint32_t va, uint32_t size) { memoryBreakHandler(va, size); });
    _context->pipeline.bind();
    _context->localMemory.reset(new uint8_t[GcmLocalMemorySize]);
    _mm->provideMemory(RsxFbBaseAddr, GcmLocalMemorySize, _context->localMemory.get());
    
    size_t constBufferSize = VertexShaderConstantCount * sizeof(float) * 4;
    _context->vertexConstBuffer = GLBuffer(GLBufferType::Dynamic, constBufferSize);
    
    auto uniformSize = sizeof(VertexShaderSamplerUniform);
    _context->vertexSamplersBuffer = GLBuffer(GLBufferType::MapWrite, uniformSize);
    
    uniformSize = sizeof(VertexShaderViewportUniform);
    _context->vertexViewportBuffer = GLBuffer(GLBufferType::MapWrite, uniformSize);
    
    uniformSize = sizeof(FragmentShaderSamplerUniform);
    _context->fragmentSamplersBuffer = GLBuffer(GLBufferType::MapWrite, uniformSize);
    
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

void Rsx::updateViewPort() {
    glcall(glClipControl(GL_UPPER_LEFT, GL_NEGATIVE_ONE_TO_ONE));
    glcall(glViewport(_context->viewPort.x,
                      _window.height() - (_context->viewPort.y + _context->viewPort.height),
                      _context->viewPort.width,
                      _context->viewPort.height));
    auto f = _context->viewPort.zmax;
    auto n = _context->viewPort.zmin;
    glcall(glDepthRange(n, f));
    
    glm::mat4 gl;
    gl[0] = glm::vec4(_context->viewPort.width / 2, 0, 0, 0);
    gl[1] = glm::vec4(0, _context->viewPort.height / 2, 0, 0);
    gl[2] = glm::vec4(0, 0, (f - n) / 2, 0);
    gl[3] = glm::vec4(_context->viewPort.x + _context->viewPort.width / 2,
                      _context->viewPort.y + _context->viewPort.height / 2,
                      (f + n) / 2, 1);

    glm::mat4 gcm;
    gcm[0] = glm::vec4(_context->viewPort.scale[0], 0, 0, 0);
    gcm[1] = glm::vec4(0, _context->viewPort.scale[1], 0, 0);
    gcm[2] = glm::vec4(0, 0, _context->viewPort.scale[2], 0);
    gcm[3] = glm::vec4(_context->viewPort.offset[0],
                       _context->viewPort.offset[1],
                       _context->viewPort.offset[2],
                       1);
    
    auto viewportUniform = (VertexShaderViewportUniform*)_context->vertexViewportBuffer.map();
    viewportUniform->glInverseGcm = glm::inverse(gl) * gcm;
    _context->vertexViewportBuffer.unmap();
}

void Rsx::updateFramebuffer() {
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
   
        auto va = rsxOffsetToEa(format.location, format.offset);
        auto bufferVa = va + first * format.stride;
        auto bufferSize = count * format.stride;
        
        BufferCacheKey key { bufferVa, bufferSize };
        GLBuffer* buffer = _context->bufferCache.retrieve(key);
        if (!buffer) {
            auto updater = new SimpleCacheItemUpdater<GLBuffer> {
                bufferVa, bufferSize, [=](auto b) {
                    auto mapped = b->map();
                    _mm->readMemory(bufferVa, mapped, bufferSize);
                    b->unmap();
                }
            };
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
    _mm->store<4>(_context->semaphoreOffset + GcmLabelBaseOffset, value);
    __sync_synchronize();
}

void Rsx::SemaphoreOffset(uint32_t offset) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SemaphoreOffset(%x)", offset);
    _context->semaphoreOffset = offset;
}

void Rsx::memoryBreakHandler(uint32_t va, uint32_t size) {
    _context->bufferCache.invalidate(va, size);
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
    auto setMemoryBreak = [=](uint32_t va, uint32_t size) { _mm->memoryBreak(va, size); };
    _context->bufferCache.watch(setMemoryBreak);
    _context->textureCache.watch(setMemoryBreak);
    _context->fragmentShaderCache.watch(setMemoryBreak);
}

void Rsx::setVBlankHandler(uint32_t descrEa) {
    _context->vBlankHandlerDescr = descrEa;
}

// Data Transfer

void Rsx::OffsetDestin(uint32_t offset) {
    TRACE1(OffsetDestin, offset);
    _context->transfer.surfaceOffsetDestin = offset;
}

void Rsx::ColorFormat(uint32_t format, uint16_t dstPitch, uint16_t srcPitch) {
    TRACE3(ColorFormat, format, dstPitch, srcPitch);
    assert(format == CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 ||
           format == CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8 ||
           format == CELL_GCM_TRANSFER_SURFACE_FORMAT_Y32);
    _context->transfer.format = format;
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
    TRACE6(Point, pointX, pointY, outSizeX, outSizeY, inSizeX, inSizeY);
    _context->transfer.pointX = pointX;
    _context->transfer.pointY = pointY;
    _context->transfer.outSizeX = outSizeX;
    _context->transfer.outSizeY = outSizeY;
    _context->transfer.inSizeX = inSizeX;
    _context->transfer.inSizeY = inSizeY;
    
    assert(outSizeY <= 1);
    assert(inSizeY <= 1);
    assert(outSizeX == inSizeX);
}

void Rsx::Color(uint32_t ptr, uint32_t count) {
    TRACE2(Color, ptr, count);
    assert(_context->transfer.format == CELL_GCM_TRANSFER_SURFACE_FORMAT_Y32);
    auto dest = rsxOffsetToEa(_context->transfer.destTransferLocation,
                              _context->transfer.surfaceOffsetDestin +
                                  _context->transfer.pointX * 4);
    assert(~(ptr & 0xfffff) > count && "mid page writes aren't supported");
    static std::vector<uint8_t> vec;
    vec.resize(count * 4);
    _mm->readMemory(ptr, &vec[0], count * 4);
    _mm->writeMemory(dest, &vec[0], count * 4);
}

void Rsx::ContextDmaImageDestin(uint32_t location) {
    TRACE1(ContextDmaImageDestin, location);
    _context->transfer.destTransferLocation = gcmEnumToLocation(location);
}

void Rsx::OffsetIn(uint32_t offset) {
    TRACE1(OffsetIn_1, offset);
    _context->transfer.sourceOffset = offset;
}

void Rsx::OffsetOut(uint32_t offset) {
    TRACE1(OffsetOut, offset);
    _context->transfer.destOffset = offset;
}

void Rsx::PitchIn(int32_t inPitch,
                  int32_t outPitch,
                  uint32_t lineLength,
                  uint32_t lineCount,
                  uint8_t inFormat,
                  uint8_t outFormat) {
    TRACE6(PitchIn, inPitch, outPitch, lineLength, lineCount, inFormat, outFormat);
    _context->transfer.sourcePitch = inPitch;
    _context->transfer.destPitch = outPitch;
    _context->transfer.lineLength = lineLength;
    _context->transfer.lineCount = lineCount;
    _context->transfer.sourceFormat = inFormat;
    _context->transfer.destFormat = outFormat;
}

void Rsx::DmaBufferIn(uint32_t sourceLocation, uint32_t dstLocation) {
    TRACE2(DmaBufferIn, sourceLocation, dstLocation);
    _context->transfer.sourceDataLocation = gcmEnumToLocation(sourceLocation);
    _context->transfer.destDataLocation = gcmEnumToLocation(dstLocation);
}

void Rsx::OffsetIn(uint32_t inOffset, 
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
    assert(notify == 0);
    auto sourceEa = rsxOffsetToEa(_context->transfer.sourceDataLocation, inOffset);
    auto destEa = rsxOffsetToEa(_context->transfer.destDataLocation, outOffset);
    assert((~(sourceEa & 0xfffff) > inPitch * lineCount) && "mid page writes aren't supported");
    assert((~(destEa & 0xfffff) > outPitch * lineCount) && "mid page writes aren't supported");

    uint32_t srcLine = sourceEa;
    uint32_t destLine = destEa;
    for (auto line = 0u; line < lineCount; ++line) {
        uint8_t* src = _mm->getMemoryPointer(srcLine, lineLength);
        uint8_t* dest = _mm->getMemoryPointer(destLine, lineLength);
        auto srcLineEnd = src + lineLength;
        while (src < srcLineEnd) {
            *dest = *src;
            src += inFormat;
            dest += outFormat;
        }
        srcLine += inPitch;
        destLine += outPitch;
    }
}

void Rsx::BufferNotify(uint32_t notify) {
    TRACE1(BufferNotify, notify);
    OffsetIn(_context->transfer.sourceOffset,
             _context->transfer.destOffset,
             _context->transfer.sourcePitch,
             _context->transfer.destPitch,
             _context->transfer.lineLength,
             _context->transfer.lineCount,
             _context->transfer.sourceFormat,
             _context->transfer.destFormat,
             notify);
}

void Rsx::Nv3089ContextDmaImage(uint32_t location) {
    TRACE1(Nv3089ContextDmaImage, location);
    _context->transfer.sourceImageLocation = gcmEnumToLocation(location);
}

void Rsx::Nv3089ContextSurface(uint32_t surfaceType) {
    TRACE1(Nv3089ContextSurface, surfaceType);
    if (surfaceType != CELL_GCM_CONTEXT_SURFACE2D)
        throw std::runtime_error("swizzled surface is not supported");
    _context->transfer.surfaceType = surfaceType;
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
    _context->transfer.conv = {fmt, x, y, w, h, outX, outY, outW, outH, dsdx, dtdy};
    assert(fmt == CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5 ||
           fmt == CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8);
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
    auto& conv = _context->transfer.conv;
    if (conv.dsdx == 0 || conv.dtdy == 0)
        return;
    //assert(conv.dsdx == 1);
    //assert(conv.dtdy == 1);
    
    auto sourceEa = rsxOffsetToEa(_context->transfer.sourceImageLocation, offset);
    auto destEa = rsxOffsetToEa(_context->transfer.destTransferLocation,
                                _context->transfer.surfaceOffsetDestin);
    auto sourcePitch = pitch;
    auto destPitch = _context->transfer.destPitch;
    auto destFormat = _context->transfer.format;
    
    assert((~(sourceEa & 0xfffff) > sourcePitch * inH) &&
           "mid page writes aren't supported");
    assert((~(destEa & 0xfffff) > destPitch * (conv.outH + conv.outY)) &&
           "mid page writes aren't supported");
    
    auto sourcePixelSize = conv.fmt == CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5 ? 2 : 4;
    auto destPixelSize = destFormat == CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 ? 2 : 4;
    assert(sourcePixelSize == destPixelSize && "not implemented");
    
    auto srcLine = sourceEa;
    auto destLine = destEa + conv.outY * destPitch + conv.outX * destPixelSize;
    for (auto i = 0u; i < conv.clipH; ++i) {
        static std::vector<uint8_t> buf;
        buf.resize(conv.clipW * sourcePixelSize);
        _mm->readMemory(srcLine, &buf[0], buf.size());
        _mm->writeMemory(destLine, &buf[0], buf.size());
        srcLine += sourcePitch;
        destLine += destPitch;
    }
}

void Rsx::setOperationMode(RsxOperationMode mode) {
    _mode = mode;
}

template <typename... Args>
void Rsx::trace(CommandId id, std::vector<GcmCommandArg> const& args) {
    _context->tracer.trace(_context->frame, _context->commandNum++, id, args);
}

RsxOperationMode Rsx::_mode = RsxOperationMode::Run;

#define X(x) #x,
const char* commandNames[] = { CommandIdX };

const char* printCommandId(CommandId id) {
    return commandNames[(int)id];
}