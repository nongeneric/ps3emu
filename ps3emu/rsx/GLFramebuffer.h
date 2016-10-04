#pragma once

#include "GLTexture.h"
#include "../constants.h"
#include "ps3emu/enum.h"
#include <array>
#include <memory>
#include <map>

ENUM(SurfaceDepthType,
     (FIXED, 0),
     (FLOAT, 1)
)

ENUM(SurfaceDepthFormat,
     (Z16, 1),
     (Z24S8, 2)
)

ENUM(GcmSurfaceColor,
    (X1R5G5B5_Z1R5G5B5, 1),
    (X1R5G5B5_O1R5G5B5, 2),
    (R5G6B5, 3),
    (X8R8G8B8_Z8R8G8B8, 4),
    (X8R8G8B8_O8R8G8B8, 5),
    (A8R8G8B8, 8),
    (B8, 9),
    (G8B8, 10),
    (F_W16Z16Y16X16, 11),
    (F_W32Z32Y32X32, 12),
    (F_X32, 13),
    (X8B8G8R8_Z8B8G8R8, 14),
    (X8B8G8R8_O8B8G8R8, 15),
    (A8B8G8R8, 16)
)

struct ScissorInfo {
    uint16_t x = 0;
    uint16_t width = 4096;
    uint16_t y = 0;
    uint16_t height = 4096;
};

struct SurfaceInfo {
    MemoryLocation colorLocation[4];
    MemoryLocation depthLocation;
    GcmSurfaceColor colorFormat;
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
        return std::tie(offset, width, height, format) <
               std::tie(other.offset,
                        other.width,
                        other.height,
                        other.format);
    }
};

struct GLFramebufferCacheEntry {
    FramebufferTextureKey key;
    GLSimpleTexture* texture;
};

struct FramebufferTextureResult {
    GLSimpleTexture* texture;
    float xOffset, yOffset;
    float xScale, yScale;
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
    FramebufferTextureResult findTexture(FramebufferTextureKey key);
    std::vector<GLFramebufferCacheEntry> cacheSnapshot();
};
