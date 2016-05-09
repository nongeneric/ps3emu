#include "GLTexture.h"
#include "DXT.h"
#include "../MainMemory.h"
#include <boost/log/trivial.hpp>
#include <gcm_tool.h>

using namespace glm;

// return rgba

void read_A8R8G8B8(const uint8_t* raw, u8vec4& texel) {
    texel = { raw[1], raw[2], raw[3], raw[0] };
}

void read_D8R8G8B8(const uint8_t* raw, u8vec4& texel) {
    texel = { raw[1], raw[2], raw[3], 0xff };
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

GLTexture::GLTexture(const RsxTextureInfo& info): _info(info) {
    glcall(glCreateTextures(GL_TEXTURE_2D, 1, &_handle));
    glcall(glTextureStorage2D(_handle, info.mipmap, GL_RGBA32F, info.width, info.height));
}

void GLTexture::update(std::vector<uint8_t>& blob) {
    std::unique_ptr<vec4[]> conv(new vec4[_info.width * _info.height]);
    auto texelFormat = _info.format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
    
    if (texelFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT1 ||
        texelFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT23 ||
        texelFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT45) {
        unsigned blockWidth = _info.width / 4;
        unsigned blockHeight = _info.height / 4;
        std::array<glm::vec4, 16> decoded;
        for (auto by = 0u; by < blockHeight; ++by) {
            for (auto bx = 0u; bx < blockWidth; ++bx) {
                auto block = &blob[(bx + by * blockWidth) * 16];
                decodeDXT23(block, &decoded[0]);
                auto x = bx * 4;
                auto line = by * 4;
                for (int i = 0; i < 4; ++i) {
                    memcpy(&conv[line * _info.width + x],
                           &decoded[4 * i],
                           4 * 4 * sizeof(float));
                    line++;
                }
            }
        }
    } else {
        TextureReader reader(texelFormat, _info);
        auto texelSize = getTexelSize(texelFormat);
        if (_info.format & CELL_GCM_TEXTURE_LN) {
            TextureIterator it(&blob[0], _info.pitch, texelSize);
            for (auto i = 0; i < _info.width * _info.height; ++i) {
                reader.read(*it, conv[i]);
                ++it;
            }
        } else {
            SwizzledTextureIterator it(
                &blob[0], _info.width, _info.height, 1, texelSize);
            auto i = 0u;
            for (auto v = 0; v < _info.height; ++v) {
                for (auto u = 0; u < _info.width; ++u) {
                    reader.read(it.at(u, v, 0), conv[i]);
                    i++;
                }
            }
        }
    }
    
    glcall(glTextureSubImage2D(_handle,
        0, 0, 0, _info.width, _info.height, GL_RGBA, GL_FLOAT, conv.get()
    ));
}

GLTexture::~GLTexture() {
    //glcall(glDeleteTextures(1, &_handle));
    glDeleteTextures(1, &_handle);
}

const RsxTextureInfo& GLTexture::info() const {
    return _info;
}

void GLTexture::bind(GLuint textureUnit) {
    glcall(glBindTextureUnit(textureUnit, _handle));
}

void glcheck(int line, const char* call) {
//     BOOST_LOG_TRIVIAL(trace) << "glcall: " << call;
//     auto err = glGetError();
//     if (err) {
//         auto msg = err == GL_INVALID_ENUM ? "GL_INVALID_ENUM"
//                    : err == GL_INVALID_VALUE ? "GL_INVALID_VALUE"
//                    : err == GL_INVALID_OPERATION ? "GL_INVALID_OPERATION"
//                    : err == GL_INVALID_FRAMEBUFFER_OPERATION ? "GL_INVALID_FRAMEBUFFER_OPERATION"
//                    : err == GL_OUT_OF_MEMORY ? "GL_OUT_OF_MEMORY"
//                    : "unknown";
//         throw std::runtime_error(ssnprintf("[%d] error: %x (%s)\n", line, err, msg));
//     }
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
    } else if (texelFormat == CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT) {
        return [](uint8_t* p) {
            auto u = (uint32_t*)p;
            return vec4(glm::unpackHalf2x16(u[0]), glm::unpackHalf2x16(u[1]));
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
                vec4 tex = {remap(readout.r,
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
                                  info.fragmentGamma & CELL_GCM_TEXTURE_GAMMA_A)};
                tex = {crossbar(tex, remapParams.inR.u()),
                       crossbar(tex, remapParams.inG.u()),
                       crossbar(tex, remapParams.inB.u()),
                       crossbar(tex, remapParams.inA.u())};
                tex = {select(tex.r, remapParams.outR.u()),
                       select(tex.g, remapParams.outG.u()),
                       select(tex.b, remapParams.outB.u()),
                       select(tex.a, remapParams.outA.u())};
                return tex;
            };
        }
    }
    throw std::runtime_error("");
}

void TextureReader::read(uint8_t* ptr, vec4& tex) {
    tex = _read(ptr);
}

TextureIterator::TextureIterator(uint8_t* buf, unsigned int pitch, unsigned size)
    : _ptr(buf), _pitch(pitch), _size(size) { }
    
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

GLuint GLTexture::handle() {
    return _handle;
}

SwizzledTextureIterator::SwizzledTextureIterator(uint8_t* buf,
                                                 unsigned width,
                                                 unsigned height,
                                                 unsigned depth,
                                                 unsigned texelSize)
    : _ptr(buf),
      _lg2Width(log2l(width)),
      _lg2Height(log2l(height)),
      _lg2Depth(depth ? log2l(depth) : 0),
      _texelSize(texelSize) {}
      
SwizzledTextureIterator::SwizzledTextureIterator(uint8_t* buf,
                                                 unsigned lgWidth,
                                                 unsigned lgHeight,
                                                 unsigned texelSize)
    : _ptr(buf),
      _lg2Width(lgWidth),
      _lg2Height(lgHeight),
      _lg2Depth(0),
      _texelSize(texelSize) {}

uint8_t* SwizzledTextureIterator::at(unsigned x, unsigned y, unsigned z) {
    return &_ptr[swizzleAddress(x, y, z) * _texelSize];
}

unsigned SwizzledTextureIterator::swizzleAddress(unsigned x, unsigned y, unsigned z) {
    auto offset = 0u;
    auto shift = 0u;
    auto w = _lg2Width;
    auto h = _lg2Height;
    auto d = _lg2Depth;
    while (w | h | d) {
        if (w) {
            offset |= (x & 1) << shift;
            x >>= 1;
            ++shift;
            --w;
        }
        if (h) {
            offset |= (y & 1) << shift;
            y >>= 1;
            ++shift;
            --h;
        }
        if (d) {
            offset |= (z & 1) << shift;
            z >>= 1;
            ++shift;
            --d;
        }
    }
    return offset;
}

std::tuple<unsigned, unsigned> SwizzledTextureIterator::unswizzle(unsigned u,
                                                                  unsigned v) {
    unsigned x = 0, y = 0;
    unsigned offset = (v << _lg2Width) | u;
    for (auto i = 0u; i < _lg2Width; i++) {
        x |= ((offset >> (i * 2)) & 1) << i;
    }
    offset >>= 1;
    for (auto i = 0u; i < _lg2Height; i++) {
        y |= ((offset >> (i * 2)) & 1) << i;
    }
    return std::make_tuple(x, y);
}
