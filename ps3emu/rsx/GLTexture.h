#pragma once

#include <glad/glad.h>
#include <stdint.h>

struct RsxTextureInfo {
    uint32_t pitch;
    uint16_t width;
    uint16_t height;
    uint32_t offset; 
    uint8_t mipmap;
    uint8_t format;
    uint8_t dimension;
    uint8_t location;
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

#define glcall(a) { a; glcheck(__LINE__, #a); }
void glcheck(int line, const char* call);

class PPU;
class GLTexture {
    RsxTextureInfo _info;
    GLuint _handle;
    GLTexture(GLTexture const&) = delete;
    void operator=(GLTexture const&) = delete;
public:
    GLTexture(PPU* ppu, RsxTextureInfo const& info);
    ~GLTexture();
    RsxTextureInfo const& info() const;
    void bind(GLuint samplerIndex);
};