#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "GLBuffer.h"
#include "../constants.h"
#include "../../libs/graphics/gcm.h"
#include <stdint.h>
#include <functional>

struct RsxTextureInfo {
    uint32_t pitch;
    uint16_t width;
    uint16_t height;
    uint32_t offset; 
    uint8_t mipmap;
    uint8_t format;
    uint8_t dimension;
    MemoryLocation location;
    bool fragmentBorder;
    bool fragmentCubemap;
    uint16_t fragmentDepth;
    uint32_t fragmentRemapCrossbarSelect;
    uint8_t fragmentGamma;
    uint8_t fragmentUnsignedRemap;
    uint8_t fragmentSignedRemap;
    uint8_t fragmentAs;
    uint8_t fragmentRs;
    uint8_t fragmentGs;
    uint8_t fragmentBs;
};

class PPU;
class GLTexture {
    RsxTextureInfo _info;
    GLuint _handle;
    GLTexture(GLTexture const&) = delete;
    void operator=(GLTexture const&) = delete;
public:
    GLTexture(RsxTextureInfo const& info);
    ~GLTexture();
    RsxTextureInfo const& info() const;
    void update(std::vector<uint8_t>& blob);
    void bind(GLuint samplerIndex);
    GLuint handle();
};

class TextureReader {
    std::function<glm::vec4(uint8_t*)> _read;
    std::function<glm::vec4(uint8_t*)> make_read(uint32_t texelFormat,
                                                 RsxTextureInfo const& info);
public:
    TextureReader(uint32_t format, RsxTextureInfo const& info);
    void read(uint8_t* ptr, glm::vec4& tex);
};

class SwizzledTextureIterator {
    uint8_t* _ptr;
    unsigned _lg2Width;
    unsigned _lg2Height;
    unsigned _lg2Depth;
    unsigned _texelSize;

public:
    SwizzledTextureIterator(uint8_t* buf,
                            unsigned width,
                            unsigned height,
                            unsigned depth,
                            unsigned texelSize);
    SwizzledTextureIterator(uint8_t* buf,
                            unsigned lgWidth,
                            unsigned lgHeight,
                            unsigned texelSize);
    uint8_t* at(unsigned x, unsigned y, unsigned z);
    unsigned swizzleAddress(unsigned x, unsigned y, unsigned z);
    std::tuple<unsigned, unsigned> unswizzle(unsigned u, unsigned v);
};

class GLSimpleTexture {
    GLuint _handle;
    unsigned _width;
    unsigned _height;
    GLuint _format;
public:
    GLSimpleTexture(unsigned width, unsigned height, GLuint format);
    ~GLSimpleTexture();
    GLuint handle();
    unsigned width();
    unsigned height();
    GLuint format();
};

template <typename BF>
uint8_t ext8(BF bf) {
    static_assert(BF::W < 8, "");
    uint8_t val = bf.u();
    return (val << (8 - BF::W)) | (val >> (2 * BF::W - 8));
}