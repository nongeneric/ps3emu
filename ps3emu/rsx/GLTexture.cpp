#include "GLTexture.h"
#include "DXT.h"
#include "GcmConstants.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/log.h"
#include <boost/align.hpp>

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

unsigned getTexelSize2(GcmTextureFormat format) {
    switch (format) {
        case GcmTextureFormat::A8R8G8B8:
        case GcmTextureFormat::D8R8G8B8:
        case GcmTextureFormat::DEPTH24_D8:
        case GcmTextureFormat::DEPTH24_D8_FLOAT:
        case GcmTextureFormat::Y16_X16:
        case GcmTextureFormat::X32_FLOAT:
        case GcmTextureFormat::Y16_X16_FLOAT:
            return 4;
        case GcmTextureFormat::A1R5G5B5:
        case GcmTextureFormat::R5G5B5A1:
        case GcmTextureFormat::A4R4G4B4:
        case GcmTextureFormat::R5G6B5:
        case GcmTextureFormat::D1R5G5B5:
        case GcmTextureFormat::R6G5B5:
        case GcmTextureFormat::G8B8:
        case GcmTextureFormat::DEPTH16:
        case GcmTextureFormat::DEPTH16_FLOAT:
        case GcmTextureFormat::X16:
        case GcmTextureFormat::COMPRESSED_HILO8:
        case GcmTextureFormat::COMPRESSED_HILO_S8:
            return 2;
        case GcmTextureFormat::B8:
            return 1;
        case GcmTextureFormat::W16_Z16_Y16_X16_FLOAT:
        case GcmTextureFormat::COMPRESSED_DXT1:
            return 8;
        case GcmTextureFormat::W32_Z32_Y32_X32_FLOAT:
        case GcmTextureFormat::COMPRESSED_DXT23:
        case GcmTextureFormat::COMPRESSED_DXT45:
            return 16;
        default: assert(false);
    }
    return {};
}

FormatInfo getFormat(GcmTextureFormat format) {
    switch (format) {
        case GcmTextureFormat::A8R8G8B8:
            return { FormatType::u8x4, read_A8R8G8B8 };
        case GcmTextureFormat::D8R8G8B8:
            return { FormatType::u8x4, read_D8R8G8B8 };
        case GcmTextureFormat::A1R5G5B5:
            return { FormatType::u8x4, read_A1R5G5B5 };
        case GcmTextureFormat::R5G5B5A1:
            return { FormatType::u8x4, read_R5G5B5A1 };
        case GcmTextureFormat::A4R4G4B4:
            return { FormatType::u8x4, read_A4R4G4B4 };
        case GcmTextureFormat::R5G6B5:
            return { FormatType::u8x4, read_R5G6B5 };
        case GcmTextureFormat::D1R5G5B5:
            return { FormatType::u8x4, read_D1R5G5B5 };
        case GcmTextureFormat::R6G5B5:
            return { FormatType::u8x4, read_R6G5B5 };
        case GcmTextureFormat::B8:
            return { FormatType::u8x4, read_B8 };

        case GcmTextureFormat::G8B8:
        case GcmTextureFormat::DEPTH16:
        case GcmTextureFormat::DEPTH16_FLOAT:
        case GcmTextureFormat::X16:
        case GcmTextureFormat::COMPRESSED_HILO8:
        case GcmTextureFormat::COMPRESSED_HILO_S8:
        case GcmTextureFormat::DEPTH24_D8:
        case GcmTextureFormat::DEPTH24_D8_FLOAT:
        case GcmTextureFormat::Y16_X16:
        case GcmTextureFormat::X32_FLOAT:
        case GcmTextureFormat::Y16_X16_FLOAT:
        case GcmTextureFormat::W32_Z32_Y32_X32_FLOAT:
        case GcmTextureFormat::COMPRESSED_DXT1:
        case GcmTextureFormat::COMPRESSED_DXT23:
        case GcmTextureFormat::COMPRESSED_DXT45:
        default: INFO(libs) << sformat("unsupported texture format {}", format); assert(false);
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
            //assert(signedRemap == CELL_GCM_TEXTURE_SIGNED_REMAP_CLAMPED);
            r = (r + 128.f) / 127.5f - 1.f;
        }
    } else if (gamma) {
        r = (r + 128.f) / 255.f;
        r = r <= 0.03928f ? r / 12.92f : glm::pow((r + 0.055f) / 1.055f, 2.4f);
    } else {
        if (unsignedRemap == CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL) {
            r /= 255.f;
        } else {
            //assert(unsignedRemap == CELL_GCM_TEXTURE_UNSIGNED_REMAP_BIASED);
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

GLenum gcmTextureFormatToOpengl(uint32_t dimension, bool cube) {
    if (dimension == 2) {
        if (cube)
            return GL_TEXTURE_CUBE_MAP;
        return GL_TEXTURE_2D;
    }
    throw std::runtime_error("unsupported texture format");
}

GLTexture::GLTexture(const RsxTextureInfo& info): _info(info) {
    // TODO: make sure cube is only checked for fragment textures
    auto target = gcmTextureFormatToOpengl(info.dimension, info.fragmentCubemap);
    glCreateTextures(target, 1, &_handle);
    glTextureStorage2D(_handle, info.mipmap, GL_RGBA32F, info.width, info.height);
    if (info.fragmentCubemap) {
        for (auto layer = 0; layer < 6; ++layer) {
            for (auto i = 0; i < info.mipmap; ++i) {
                auto handle = glGetImageHandleARB(_handle, i, false, layer, GL_RGBA32F);
                _levelHandles.push_back(handle);
                glMakeImageHandleResidentARB(handle, GL_WRITE_ONLY);
            }
        }
    } else {
        for (auto i = 0; i < info.mipmap; ++i) {
            auto handle = glGetImageHandleARB(_handle, i, false, 0, GL_RGBA32F);
            _levelHandles.push_back(handle);
            glMakeImageHandleResidentARB(handle, GL_WRITE_ONLY);
        }
    }
}

class LinearTextureIterator {
    uint8_t* _buf;
    uint32_t _pitch;
    //uint32_t _width;
    //uint32_t _height;
    uint32_t _texelSize;
    //uint32_t _level;
    //bool _paddedLevels;
    uint32_t _levelOffset;
    uint32_t _levelWidth;
    uint32_t _levelHeight;
    uint32_t _levelPitch;

public:
    LinearTextureIterator(void* buf,
                          uint32_t pitch,
                          uint32_t level0width,
                          uint32_t level0height,
                          uint32_t texelSize,
                          uint32_t level,
                          bool paddedLevels = false)
        : _buf((uint8_t*)buf),
          _pitch(pitch == 0 ? level0width * texelSize : pitch),
//           _width(level0width),
//           _height(level0height),
           _texelSize(texelSize),
//           _level(level),
//           _paddedLevels(paddedLevels),
          _levelWidth(level0width),
          _levelHeight(level0height)
    {
        _levelOffset = 0;
        _levelPitch = _pitch;
        for (auto i = 0u; i < level; ++i) {
            _levelOffset += _levelWidth * _levelHeight * texelSize;
            _levelWidth = std::max<uint32_t>(_levelWidth / 2, 1);
            _levelHeight = std::max<uint32_t>(_levelHeight / 2, 1);
            _levelPitch = std::max<uint32_t>(_levelPitch / 2, 1);
        }
    }

    uint8_t* at(uint32_t x, uint32_t y) {
        return &_buf[y * _levelPitch + x * _texelSize] + _levelOffset;
    }

    uint32_t w() { return _levelWidth; }
    uint32_t h() { return _levelHeight; }
    uint32_t size() { return at(0, h()) - _buf; }
};

uint32_t GLTexture::read2d(
    uint8_t* raw,
    std::function<void(unsigned, unsigned, unsigned, glm::vec4*)> handler)
{
    auto size = 0ul;
    std::vector<vec4> conv(_info.width * _info.height);
    for (auto level = 0u; level < _info.mipmap; ++level) {
        LinearTextureIterator srcDecoded(nullptr, 0, _info.width, _info.height, 16, level);
        auto isDX1 = _info.format == GcmTextureFormat::COMPRESSED_DXT1;
        auto isDX23 = _info.format == GcmTextureFormat::COMPRESSED_DXT23;
        auto isDX45 = _info.format == GcmTextureFormat::COMPRESSED_DXT45;
        if (isDX1 || isDX23 || isDX45) {
            auto decode = isDX1 ? decodeDXT1 : isDX23 ? decodeDXT23 : decodeDXT45;
            auto blockSize = isDX1 ? 8 : 16;
            unsigned blockWidth = _info.width / 4;
            unsigned blockHeight = _info.height / 4;
            std::array<glm::vec4, 16> decoded;
            LinearTextureIterator src(raw, 0, blockWidth, blockHeight, blockSize, level);
            LinearTextureIterator dest(&conv[0], 0, srcDecoded.w(), srcDecoded.h(), 16, 0);
            for (auto by = 0u; by < src.h(); ++by) {
                for (auto bx = 0u; bx < src.w(); ++bx) {
                    auto block = src.at(bx, by);
                    decode(block, &decoded[0]);
                    auto a = print_hex(block, 16);
                    std::string b;
                    for (auto f4 : decoded) {
                        b += sformat("{:0.3f} ", f4.r);
                        b += sformat("{:0.3f} ", f4.g);
                        b += sformat("{:0.3f} ", f4.b);
                        b += sformat("{:0.3f} ", f4.a);
                        b += "| ";
                    }
                    for (int i = 0; i < 4; ++i) {
                        memcpy(dest.at(bx * 4, by * 4 + i),
                               &decoded[4 * i],
                               4 * 4 * sizeof(float));
                    }
                }
            }
            size = src.size();
        } else {
            TextureReader reader(_info.format, _info);
            auto texelSize = getTexelSize2(_info.format);
            if (!!(_info.lnUn & GcmTextureLnUn::LN)) {
                LinearTextureIterator it(
                    raw, _info.pitch, _info.width, _info.height, texelSize, level);
                for (auto y = 0u; y < _info.height; ++y) {
                    for (auto x = 0u; x < _info.width; ++x) {
                        reader.read(it.at(x, y), conv[_info.width * y + x]);
                    }
                }
                size = it.size();
            } else {
                SwizzledTextureIterator it(
                    raw, _info.width, _info.height, 1, texelSize);
                auto i = 0u;
                for (auto v = 0; v < _info.height; ++v) {
                    for (auto u = 0; u < _info.width; ++u) {
                        reader.read(it.at(u, v, 0), conv[i]);
                        i++;
                    }
                }
                // TODO: implement mipmaps and size calculation
            }
        }
        handler(level, srcDecoded.w(), srcDecoded.h(), &conv[0]);
    }
    return size;
}

void GLTexture::update(std::vector<uint8_t>& blob) {
    if (_info.fragmentCubemap) {
        auto pos = 0ul;
        for (int layer = 0; layer < 6; ++layer) {
            pos += read2d(&blob[pos], [&](unsigned level, unsigned w, unsigned h, glm::vec4* ptr) {
                glTextureSubImage3D(_handle, level, 0, 0, layer, w, h, 1, GL_RGBA, GL_FLOAT, ptr);
            });
            pos = boost::alignment::align_up(pos, 128);
        }
    } else {
        read2d(&blob[0], [&](unsigned level, unsigned w, unsigned h, glm::vec4* ptr) {
            glTextureSubImage2D(_handle, level, 0, 0, w, h, GL_RGBA, GL_FLOAT, ptr);
        });
    }
}

GLTexture::~GLTexture() {
    glDeleteTextures(1, &_handle);
}

const RsxTextureInfo& GLTexture::info() const {
    return _info;
}

void GLTexture::bind(GLuint textureUnit) {
    glBindTextureUnit(textureUnit, _handle);
}

TextureReader::TextureReader(GcmTextureFormat format, const RsxTextureInfo& info)
    : _read(make_read(format, info)) { }

std::function<glm::vec4(uint8_t*)> TextureReader::make_read(GcmTextureFormat texelFormat,
                                                            RsxTextureInfo const& info)
{
    if (texelFormat == GcmTextureFormat::W32_Z32_Y32_X32_FLOAT) {
        return [](uint8_t* p) {
            auto fp = (float*)p;
            return vec4(fp[0], fp[1], fp[2], fp[3]);
        };
    } else if (texelFormat == GcmTextureFormat::W16_Z16_Y16_X16_FLOAT) {
        return [](uint8_t* p) {
            auto u = (uint32_t*)p;
            return vec4(glm::unpackHalf2x16(u[0]), glm::unpackHalf2x16(u[1]));
        };
    } else if (texelFormat == GcmTextureFormat::Y16_X16_FLOAT) {
        return [](uint8_t* p) {
            auto u = (uint32_t*)p;
            return vec4(glm::unpackHalf2x16(u[0]), vec2(0, 0));
        };
    } else if (texelFormat == GcmTextureFormat::DEPTH16) {
        return [](uint8_t* p) {
            auto u = (big_uint16_t*)p;
            return vec4(*u, 0, 0, 0);
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

GLSimpleTexture::GLSimpleTexture(unsigned int width,
                                 unsigned int height,
                                 GLuint format)
    : _width(width), _height(height), _format(format)
{
    glCreateTextures(GL_TEXTURE_2D, 1, &_handle);
    glTextureStorage2D(_handle, 1, format, width, height);
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

std::vector<uint64_t>& GLTexture::levelHandles() {
    return _levelHandles;
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

bool isTextureValid(RsxTextureInfo const& info) {
    return info.width != 0 && info.width != 0xffff && info.height != 0 &&
           info.height != 0xffff;
}
