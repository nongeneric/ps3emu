#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "GLBuffer.h"
#include "../constants.h"
#include "../libs/graphics/gcm.h"
#include "ps3emu/enum.h"
#include <stdint.h>
#include <functional>

ENUM(GcmTextureFormat,
    (B8, 0x81),
    (A1R5G5B5, 0x82),
    (A4R4G4B4, 0x83),
    (R5G6B5, 0x84),
    (A8R8G8B8, 0x85),
    (COMPRESSED_DXT1, 0x86),
    (COMPRESSED_DXT23, 0x87),
    (COMPRESSED_DXT45, 0x88),
    (G8B8, 0x8B),
    (R6G5B5, 0x8F),
    (DEPTH24_D8, 0x90),
    (DEPTH24_D8_FLOAT, 0x91),
    (DEPTH16, 0x92),
    (DEPTH16_FLOAT, 0x93),
    (X16, 0x94),
    (Y16_X16, 0x95),
    (R5G5B5A1, 0x97),
    (COMPRESSED_HILO8, 0x98),
    (COMPRESSED_HILO_S8, 0x99),
    (W16_Z16_Y16_X16_FLOAT, 0x9A),
    (W32_Z32_Y32_X32_FLOAT, 0x9B),
    (X32_FLOAT, 0x9C),
    (D1R5G5B5, 0x9D),
    (D8R8G8B8, 0x9E),
    (Y16_X16_FLOAT, 0x9F),
    (COMPRESSED_B8R8_G8R8, 0xAD),
    (COMPRESSED_R8B8_R8G8, 0xAE)
);

ENUMF(GcmTextureLnUn,
    (LN, 0x20),
    (UN, 0x40)
)

ENUMF(TextureRemapInput,
    (FromA, 0),
    (FromR, 1),
    (FromG, 2),
    (FromB, 3)
)

ENUMF(TextureRemapOutput,
    (Zero, 0),
    (One, 1),
    (Remap, 2)
)

struct RsxTextureInfo {
    uint32_t pitch;
    uint16_t width;
    uint16_t height;
    uint32_t offset; 
    uint8_t mipmap;
    GcmTextureFormat format;
    GcmTextureLnUn lnUn;
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
    uint32_t read2d(
        uint8_t* raw,
        std::function<void(unsigned, unsigned, unsigned, glm::vec4*)> handler);
    void bind(GLuint samplerIndex);
    GLuint handle();
};

class TextureReader {
    std::function<glm::vec4(uint8_t*)> _read;
    std::function<glm::vec4(uint8_t*)> make_read(GcmTextureFormat texelFormat,
                                                 RsxTextureInfo const& info);
public:
    TextureReader(GcmTextureFormat format, RsxTextureInfo const& info);
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
