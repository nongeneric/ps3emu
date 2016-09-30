#pragma once

#include "GLTexture.h"
#include "../constants.h"
#include <array>
#include <memory>
#include <map>

enum class SurfaceDepthType {
    Fixed, Float
};

enum class SurfaceDepthFormat {
    z16, z24s8
};

struct ScissorInfo {
    uint16_t x;
    uint16_t width;
    uint16_t y;
    uint16_t height;
};

struct SurfaceInfo {
    MemoryLocation colorLocation[4];
    MemoryLocation depthLocation;
    unsigned colorFormat;
    unsigned width;
    unsigned height;
    unsigned colorPitch[4];
    unsigned colorOffset[4];
    unsigned depthPitch;
    unsigned depthOffset;
    unsigned windowOriginX;
    unsigned windowOriginY;
    SurfaceDepthType depthType;
    SurfaceDepthFormat depthFormat;
    std::array<bool, 4> colorTarget;
    ScissorInfo scissor;
};

struct FramebufferTextureKey {
    ps3_uintptr_t offset;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    inline bool operator<(FramebufferTextureKey const& other) const {
        return std::tie(offset, width, width, height, format) <
               std::tie(other.offset,
                        other.width,
                        other.width,
                        other.height,
                        other.format);
    }
};

struct GLFramebufferCacheEntry {
    FramebufferTextureKey key;
    GLSimpleTexture* texture;
};

class GLFramebuffer {
    std::map<FramebufferTextureKey, std::unique_ptr<GLSimpleTexture>> _cache;
    GLuint _id;
    SurfaceInfo _info;
    
    void dumpTexture(GLSimpleTexture* tex, ps3_uintptr_t va);
    
    GLSimpleTexture* searchCache(GLuint format, 
                                 ps3_uintptr_t offset, 
                                 unsigned width, 
                                 unsigned height);
    
public:
    GLFramebuffer();
    ~GLFramebuffer();
    void bindDefault();
    void setSurface(SurfaceInfo const& info, unsigned width, unsigned height);
    void dumpTextures();
    void updateTexture();
    GLSimpleTexture* findTexture(FramebufferTextureKey key);
    std::vector<GLFramebufferCacheEntry> cacheSnapshot();
};
