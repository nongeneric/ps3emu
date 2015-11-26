#include "GLTexture.h"
#include "../PPU.h"

#include <boost/log/trivial.hpp>

using namespace glm;

// rgba

template <typename BF>
uint8_t ext8(BF bf) {
    static_assert(BF::W < 8, "");
    uint8_t val = bf.u();
    return (val << (8 - BF::W)) | (val >> (2 * BF::W - 8));
}

void read_A8R8G8B8(const uint8_t* raw, u8vec4& texel) {
    texel = { raw[1], raw[2], raw[3], raw[0] };
}

void read_D8R8G8B8(const uint8_t* raw, u8vec4& texel) {
    texel = { raw[0], raw[1], raw[2], 0xff };
}

void read_A1R5G5B5(const uint8_t* raw, u8vec4& texel) {
    union {
        uint32_t val;
        BitField<0, 1> a;
        BitField<1, 6> r;
        BitField<6, 11> g;
        BitField<11, 16> b;
    } t = { (uint32_t)*(const uint16_t*)raw << 16 };
    texel = { 
        ext8(t.r),
        ext8(t.g),
        ext8(t.b),
        t.a.u() ? 0xff : 0
    };
}

void read_R5G5B5A1(const uint8_t* raw, u8vec4& texel) {
    union {
        uint32_t val;
        BitField<0, 5> r;
        BitField<5, 10> g;
        BitField<10, 15> b;
        BitField<15, 16> a;
    } t = { (uint32_t)*(const uint16_t*)raw << 16 };
    texel = { 
        ext8(t.r),
        ext8(t.g),
        ext8(t.b),
        t.a.u() ? 0xff : 0
    };
}

void read_A4R4G4B4(const uint8_t* raw, u8vec4& texel) {
    union {
        uint32_t val;
        BitField<0, 4> a;
        BitField<4, 8> r;
        BitField<8, 12> g;
        BitField<12, 16> b;
    } t = { (uint32_t)*(const uint16_t*)raw << 16 };
    texel = { 
        ext8(t.r),
        ext8(t.g),
        ext8(t.b),
        ext8(t.a)
    };
}

void read_R5G6B5(const uint8_t* raw, u8vec4& texel) {
    union {
        uint32_t val;
        BitField<0, 5> r;
        BitField<5, 11> g;
        BitField<11, 16> b;
    } t = { (uint32_t)*(const uint16_t*)raw << 16 };
    texel = { 
        ext8(t.r),
        ext8(t.g),
        ext8(t.b),
        0xff
    };
}

void read_D1R5G5B5(const uint8_t* raw, u8vec4& texel) {
    union {
        uint32_t val;
        BitField<1, 6> r;
        BitField<6, 11> g;
        BitField<11, 16> b;
    } t = { (uint32_t)*(const uint16_t*)raw << 16 };
    texel = { 
        ext8(t.r),
        ext8(t.g),
        ext8(t.b),
        0xff
    };
}

void read_R6G5B5(const uint8_t* raw, u8vec4& texel) {
    union {
        uint32_t val;
        BitField<0, 6> r;
        BitField<6, 11> g;
        BitField<11, 16> b;
    } t = { (uint32_t)*(const uint16_t*)raw << 16 };
    texel = { 
        ext8(t.r),
        ext8(t.g),
        ext8(t.b),
        0xff
    };
}

void read_B8(const uint8_t* raw, u8vec4& texel) {
    texel = { 
        *raw,
        *raw,
        *raw,
        0xff
    };
}

enum class FormatType {
    u8x4, u16x2, f16x2, u32, f32
};

struct FormatInfo {
    FormatType type;
    std::function<void(const uint8_t*, u8vec4&)> u8read;
};

unsigned getTexelSize(uint32_t format) {
    switch (format) {
        case CELL_GCM_TEXTURE_A8R8G8B8:
        case CELL_GCM_TEXTURE_D8R8G8B8:
        case CELL_GCM_TEXTURE_DEPTH24_D8:
        case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
        case CELL_GCM_TEXTURE_Y16_X16:
        case CELL_GCM_TEXTURE_X32_FLOAT:
        case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
            return 4;
        case CELL_GCM_TEXTURE_A1R5G5B5:
        case CELL_GCM_TEXTURE_R5G5B5A1:
        case CELL_GCM_TEXTURE_A4R4G4B4:
        case CELL_GCM_TEXTURE_R5G6B5:
        case CELL_GCM_TEXTURE_D1R5G5B5:
        case CELL_GCM_TEXTURE_R6G5B5:
        case CELL_GCM_TEXTURE_G8B8:        
        case CELL_GCM_TEXTURE_DEPTH16:
        case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
        case CELL_GCM_TEXTURE_X16:
        case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
        case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
            return 2;
        case CELL_GCM_TEXTURE_B8:
            return 1;
        case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
        case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
            return 8;
        case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
        case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
        case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
            return 16;
        default: assert(false);
    }
    return {};
}

FormatInfo getFormat(uint32_t format) {
    switch (format) {
        case CELL_GCM_TEXTURE_A8R8G8B8:
            return { FormatType::u8x4, read_A8R8G8B8 };
        case CELL_GCM_TEXTURE_D8R8G8B8:
            return { FormatType::u8x4, read_D8R8G8B8 };
        case CELL_GCM_TEXTURE_A1R5G5B5:
            return { FormatType::u8x4, read_A1R5G5B5 };
        case CELL_GCM_TEXTURE_R5G5B5A1:
            return { FormatType::u8x4, read_R5G5B5A1 };
        case CELL_GCM_TEXTURE_A4R4G4B4:
            return { FormatType::u8x4, read_A4R4G4B4 };
        case CELL_GCM_TEXTURE_R5G6B5:
            return { FormatType::u8x4, read_R5G6B5 };
        case CELL_GCM_TEXTURE_D1R5G5B5:
            return { FormatType::u8x4, read_D1R5G5B5 };
        case CELL_GCM_TEXTURE_R6G5B5:
            return { FormatType::u8x4, read_R6G5B5 };
        case CELL_GCM_TEXTURE_B8:
            return { FormatType::u8x4, read_B8 };
        
        case CELL_GCM_TEXTURE_G8B8:        
        case CELL_GCM_TEXTURE_DEPTH16:
        case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
        case CELL_GCM_TEXTURE_X16:
        case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
        case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
        case CELL_GCM_TEXTURE_DEPTH24_D8:
        case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
        case CELL_GCM_TEXTURE_Y16_X16:
        case CELL_GCM_TEXTURE_X32_FLOAT:
        case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
        case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
        case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
        case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
        case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
        case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
        default: assert(false);
    }
    return {};
}

float remap(uint8_t f, uint32_t signedRemap, uint32_t unsignedRemap, bool sign, bool gamma) {
    float r = f;
    if (sign) {
        r = (int8_t)f;
        if (signedRemap == CELL_GCM_TEXTURE_SIGNED_REMAP_NORMAL) {
            r /= 127;
        } else {
            assert(signedRemap == CELL_GCM_TEXTURE_SIGNED_REMAP_CLAMPED);
            r = (r + 128.f) / 127.5f - 1.f;
        }
    } else if (gamma) {
        r = (r + 128.f) / 255.f;
        r = r <= 0.03928f ? r / 12.92f : glm::pow((r + 0.055f) / 1.055f, 2.4f);
    } else {
        if (unsignedRemap == CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL) {
            r /= 255.f;
        } else {
            assert(unsignedRemap == CELL_GCM_TEXTURE_UNSIGNED_REMAP_BIASED);
            r = (r - 128.f) / 255.f;
        }
    }
    return r;
}

float crossbar(vec4 const& vec, uint32_t input) {
    switch (input) {
        case CELL_GCM_TEXTURE_REMAP_FROM_R: return vec.r;
        case CELL_GCM_TEXTURE_REMAP_FROM_G: return vec.g;
        case CELL_GCM_TEXTURE_REMAP_FROM_B: return vec.b;
        case CELL_GCM_TEXTURE_REMAP_FROM_A: return vec.a;
        default: throw std::runtime_error("incorrect remap value");
    }
}

float select(float f, uint32_t output) {
    switch (output) {
        case CELL_GCM_TEXTURE_REMAP_ZERO: return 0.f;
        case CELL_GCM_TEXTURE_REMAP_ONE: return 1.f;
        case CELL_GCM_TEXTURE_REMAP_REMAP: return f;
        default: throw std::runtime_error("incorrect select value");
    }
}

class SurfaceWriter {
    std::function<void(uint8_t*, vec4 const&)> _write;
public:
    SurfaceWriter(uint32_t format) : _write(make_write(format)) { }
    
    std::function<void(uint8_t*, vec4 const&)> make_write(uint32_t texelFormat) {
        switch (texelFormat) {
            case 0: return [](uint8_t* ptr, vec4 const& v) { };
            case CELL_GCM_SURFACE_A8B8G8R8: return [](uint8_t* ptr, vec4 const& v) {
                auto typed = (uint32_t*)ptr;
                union {
                    uint32_t val;
                    BitField<0, 8> a;
                    BitField<8, 16> r;
                    BitField<16, 24> g;
                    BitField<24, 32> b;
                } t = { *typed };
                t.a.set(v[3] * 255);
                t.r.set(v[0] * 255);
                t.g.set(v[1] * 255);
                t.b.set(v[2] * 255);
                *typed = t.val;
            };
            case CELL_GCM_SURFACE_F_W32Z32Y32X32: return [](uint8_t* ptr, vec4 const& v) {
                auto f = (float*)ptr;
                f[0] = v[3];
                f[1] = v[2];
                f[2] = v[1];
                f[3] = v[0];
            };
            default: throw std::runtime_error("unknown format");
        }
    }
};

GLTexture::GLTexture(PPU* ppu, const RsxTextureInfo& info): _info(info) {
    auto size = info.pitch * info.height;
    std::unique_ptr<uint8_t[]> buf(new uint8_t[size]);
    ppu->readMemory(GcmLocalMemoryBase + info.offset, buf.get(), size);
    
    std::unique_ptr<vec4[]> conv(new vec4[info.width * info.height]);

    assert(info.format & CELL_GCM_TEXTURE_LN);
    
    auto texelFormat = info.format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
    
    TextureReader reader(texelFormat, info);
    TextureIterator it(&buf[0], info.pitch, info.width, getTexelSize(texelFormat));
    for (auto i = 0; i < info.width * info.height; ++i) {
        reader.read(*it, conv[i]);
        ++it;
    }
    
    glcall(glCreateTextures(GL_TEXTURE_2D, 1, &_handle));
    glcall(glTextureStorage2D(_handle, info.mipmap, GL_RGBA32F, info.width, info.height));
    glcall(glTextureSubImage2D(_handle,
        0, 0, 0, info.width, info.height, GL_RGBA, GL_FLOAT, conv.get()
    ));
}

GLTexture::~GLTexture() {
    //glcall(glDeleteTextures(1, &_handle));
    glDeleteTextures(1, &_handle);
}

const RsxTextureInfo& GLTexture::info() const {
    return _info;
}

void GLTexture::bind(GLuint samplerIndex) {
    glcall(glBindTextureUnit(samplerIndex, _handle));
}

void glcheck(int line, const char* call) {
    BOOST_LOG_TRIVIAL(trace) << "glcall: " << call;
    auto err = glGetError();
    if (err) {
        auto msg = err == GL_INVALID_ENUM ? "GL_INVALID_ENUM"
                   : err == GL_INVALID_VALUE ? "GL_INVALID_VALUE"
                   : err == GL_INVALID_OPERATION ? "GL_INVALID_OPERATION"
                   : err == GL_INVALID_FRAMEBUFFER_OPERATION ? "GL_INVALID_FRAMEBUFFER_OPERATION"
                   : err == GL_OUT_OF_MEMORY ? "GL_OUT_OF_MEMORY"
                   : "unknown";
        throw std::runtime_error(ssnprintf("[%d] error: %x (%s)\n", line, err, msg));
    }
}

TextureReader::TextureReader(uint32_t format, const RsxTextureInfo& info)
    : _read(make_read(format, info)) { }
    
std::function<glm::vec4(uint8_t*)> TextureReader::make_read(uint32_t texelFormat, 
                                                            RsxTextureInfo const& info)
{
    if (texelFormat == CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT) {
        return [](uint8_t* p) {
            auto fp = (float*)p;
            return vec4(fp[0], fp[1], fp[2], fp[3]);
        };
    } else {
        auto format = getFormat(texelFormat);
        if (format.type == FormatType::u8x4) {
            union {
                uint32_t v;
                BitField<0, 16> order;
                BitField<16, 18> outB;
                BitField<18, 20> outG;
                BitField<20, 22> outR;
                BitField<22, 24> outA;
                BitField<24, 26> inB;
                BitField<26, 28> inG;
                BitField<28, 30> inR;
                BitField<30, 32> inA;
            } remapParams = { info.fragmentRemapCrossbarSelect };
            return [=](uint8_t* p) {
                u8vec4 readout;
                format.u8read(p, readout);
                vec4 tex = {
                    remap(readout.r,
                    info.fragmentSignedRemap,
                    info.fragmentUnsignedRemap,
                    info.fragmentRs,
                    info.fragmentGamma & CELL_GCM_TEXTURE_GAMMA_R),
                    remap(readout.g,
                    info.fragmentSignedRemap,
                    info.fragmentUnsignedRemap,
                    info.fragmentGs,
                    info.fragmentGamma & CELL_GCM_TEXTURE_GAMMA_G),
                    remap(readout.b,
                    info.fragmentSignedRemap,
                    info.fragmentUnsignedRemap,
                    info.fragmentBs,
                    info.fragmentGamma & CELL_GCM_TEXTURE_GAMMA_B),
                    remap(readout.a,
                    info.fragmentSignedRemap,
                    info.fragmentUnsignedRemap,
                    info.fragmentAs,
                    info.fragmentGamma & CELL_GCM_TEXTURE_GAMMA_A)
                };
                tex = { crossbar(tex, remapParams.inR.u()),
                        crossbar(tex, remapParams.inG.u()),
                        crossbar(tex, remapParams.inB.u()),
                        crossbar(tex, remapParams.inA.u())
                      };
                tex = { select(tex.r, remapParams.outR.u()),
                        select(tex.g, remapParams.outG.u()),
                        select(tex.b, remapParams.outB.u()),
                        select(tex.a, remapParams.outA.u())
                      };
                return tex;
            };
        }
    }
    throw std::runtime_error("");
}

void TextureReader::read(uint8_t* ptr, vec4& tex) {
    tex = _read(ptr);
}

TextureIterator::TextureIterator(uint8_t* buf, unsigned int pitch, unsigned width, unsigned size)
    : _ptr(buf), _pitch(pitch), _width(width), _size(size) { }
    
TextureIterator& TextureIterator::operator++() {
    _pos += _size;
    if (_pos >= _pitch) {
        _pos = 0;
        _ptr += _pitch;
    }
    return *this;
}

bool TextureIterator::operator==(const TextureIterator& other) {
    return _ptr == other._ptr;
}

uint8_t* TextureIterator::operator*() {
    return _ptr + _pos;
}

GLSimpleTexture::GLSimpleTexture ( unsigned int width, unsigned int height, GLuint format )
    : _width (width), _height (height), _format (format)
{
    glcall(glCreateTextures(GL_TEXTURE_2D, 1, &_handle));
    glcall(glTextureStorage2D(_handle, 1, format, width, height));
}

GLSimpleTexture::~GLSimpleTexture() {
    glDeleteTextures(1, &_handle);
}

GLuint GLSimpleTexture::handle() {
    return _handle;
}

unsigned GLSimpleTexture::width() {
    return _width;
}

unsigned GLSimpleTexture::height() {
    return _height;
}

GLuint GLSimpleTexture::format() {
    return _format;
}
