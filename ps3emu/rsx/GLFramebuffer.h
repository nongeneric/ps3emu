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

struct SurfaceInfo {
    MemoryLocation colorLocation[4];
    MemoryLocation depthLocation;
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
};

class GLFramebuffer {
    std::map<ps3_uintptr_t, std::unique_ptr<GLSimpleTexture>> _cache;
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
    GLSimpleTexture* findTexture(ps3_uintptr_t va);
};