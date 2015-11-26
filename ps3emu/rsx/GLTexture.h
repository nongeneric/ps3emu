#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
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

class TextureReader {
    std::function<glm::vec4(uint8_t*)> _read;
    std::function<glm::vec4(uint8_t*)> make_read(uint32_t texelFormat,
                                                 RsxTextureInfo const& info);
public:
    TextureReader(uint32_t format, RsxTextureInfo const& info);
    void read(uint8_t* ptr, glm::vec4& tex);
};

// TODO: swizzle textures
// remap order for 2x16 and 32 ...
class TextureIterator : std::iterator<std::forward_iterator_tag, glm::vec4> {
    uint8_t* _ptr;
    unsigned _pos = 0;
    unsigned _pitch;
    unsigned _width;
    unsigned _size;
    TextureIterator(TextureIterator const&) = delete;
public:
    TextureIterator(uint8_t* buf, unsigned pitch, unsigned width, unsigned size);
    TextureIterator& operator++();
    bool operator==(TextureIterator const& other);
    uint8_t* operator*();
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