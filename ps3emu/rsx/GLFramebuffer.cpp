#include "GLFramebuffer.h"
#include "../utils.h"
#include "../log.h"

void GLFramebuffer::dumpTexture(GLSimpleTexture* tex, ps3_uintptr_t va) {
    // TODO:
}

GLSimpleTexture* GLFramebuffer::searchCache(GLuint format,
        ps3_uintptr_t offset,
        unsigned int width,
        unsigned int height)
{
    FramebufferTextureKey key{offset, width, height, format};
    auto itTex = _cache.find(key);
    GLSimpleTexture* tex;
    if (itTex == end(_cache)) {
        tex = new GLSimpleTexture(width, height, format);
        _cache[key].reset(tex);
    } else {
        tex = itTex->second.get();
    }
    assert(tex->width() == width);
    assert(tex->height() == height);
    assert(tex->format() == format);
    return tex;
}

GLFramebuffer::GLFramebuffer() {
    glcall(glCreateFramebuffers(1, &_id));
}

GLFramebuffer::~GLFramebuffer() {
    glDeleteFramebuffers(1, &_id);
}

void GLFramebuffer::bindDefault() {
    glcall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void GLFramebuffer::setSurface(const SurfaceInfo& info, unsigned width, unsigned height) {
    _info = info;
    for (int i = 0; i < 4; ++i) {
        GLuint texHandle = 0;
        if (info.colorTarget[i]) {
            auto offset = rsxOffsetToEa(info.colorLocation[i], info.colorOffset[i]);
            auto tex = searchCache(GL_RGBA8, offset, width, height); // GL_RGB32F
            texHandle = tex->handle();
        }
        glNamedFramebufferTexture(
                   _id,
                   GL_COLOR_ATTACHMENT0 + i,
                   texHandle,
                0);
    }
    glNamedFramebufferTexture(_id, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);
    glNamedFramebufferTexture(_id, GL_STENCIL_ATTACHMENT, 0, 0);
    glNamedFramebufferTexture(_id, GL_DEPTH_ATTACHMENT, 0, 0);
    
    auto offset = rsxOffsetToEa(info.depthLocation, info.depthOffset);
    auto format = GL_DEPTH24_STENCIL8;
    auto attachment = GL_DEPTH_STENCIL_ATTACHMENT;
    if (info.depthFormat == SurfaceDepthFormat::Z16) {
        format = GL_DEPTH_COMPONENT16;
        attachment = GL_DEPTH_ATTACHMENT;
    }
    auto tex = searchCache(format, offset, width, height);
    glNamedFramebufferTexture(_id, attachment, tex->handle(), 0);

    GLenum enabled[] = {
        info.colorTarget[0] ? GL_COLOR_ATTACHMENT0 : (GLenum) GL_NONE,
        info.colorTarget[1] ? GL_COLOR_ATTACHMENT1 : (GLenum) GL_NONE,
        info.colorTarget[2] ? GL_COLOR_ATTACHMENT2 : (GLenum) GL_NONE,
        info.colorTarget[3] ? GL_COLOR_ATTACHMENT3 : (GLenum) GL_NONE
    };
    glNamedFramebufferDrawBuffers(_id, 4, enabled);
    glBindFramebuffer(GL_FRAMEBUFFER, _id);
#if DEBUG
    auto status = glCheckNamedFramebufferStatus(_id, GL_FRAMEBUFFER);
#define X(x) status == x ? #x
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG << ssnprintf("framebuffer incomplete %s %d",
            (status == 0x8cd9 ? "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT" :
            X(GL_FRAMEBUFFER_UNDEFINED) :
            X(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) :
            X(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) :
            X(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER) :
            X(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER) :
            X(GL_FRAMEBUFFER_UNSUPPORTED) :
            X(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE) :
            X(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS) : ""), status);
        exit(0);
#undef X
    }
#endif
}

void GLFramebuffer::dumpTextures() {
//     for (int i = 0; i < 4; ++i) {
//         if (_info.colorTarget[i]) {
//             auto offset = rsxOffsetToEa(_info.colorLocation[i], _info.colorOffset[i]);
//             dumpTexture(_cache[offset].get(), offset);
//         }
//     }
//     auto offset = rsxOffsetToEa(_info.depthLocation, _info.depthOffset);
//     dumpTexture(_cache[offset].get(), offset);
}

void GLFramebuffer::updateTexture() {
    // TODO: draw -> dump -> cpu updates texture -> (update) -> flip
}

GLSimpleTexture* GLFramebuffer::findTexture(FramebufferTextureKey key) {
    auto it = _cache.find(key);
    if (it == end(_cache))
        return nullptr;
    return it->second.get();
}

std::vector<GLFramebufferCacheEntry> GLFramebuffer::cacheSnapshot() {
    std::vector<GLFramebufferCacheEntry> res;
    for (auto& pair : _cache) {
        res.push_back({ pair.first, pair.second.get() });
    }
    return res;
}
