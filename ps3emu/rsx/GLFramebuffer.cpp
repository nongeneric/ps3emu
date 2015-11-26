#include "GLFramebuffer.h"

void GLFramebuffer::dumpTexture(GLSimpleTexture* tex, ps3_uintptr_t va) {
    // TODO:
}

GLSimpleTexture* GLFramebuffer::searchCache(GLuint format,
        ps3_uintptr_t offset,
        unsigned int width,
        unsigned int height)
{
    auto itTex = _cache.find(offset);
    GLSimpleTexture* tex;
    if (itTex == end(_cache)) {
        tex = new GLSimpleTexture(width, height, format);
        _cache[offset].reset(tex);
    } else {
        tex = itTex->second.get();
    }
    assert(tex->width() == width &&
           tex->height() == height &&
           tex->format() == format);
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

void GLFramebuffer::setSurface(const SurfaceInfo& info) {
    _info = info;
    for (int i = 0; i < 4; ++i) {
        GLuint texHandle = 0;
        if (info.colorTarget[i]) {
            auto offset = addressToMainMemory(info.colorLocation[i], info.colorOffset[i]);
            auto tex = searchCache(GL_RGBA32F, offset, info.width, info.height);
            texHandle = tex->handle();
        }
        glcall(glNamedFramebufferTexture(
                   _id,
                   GL_COLOR_ATTACHMENT0 + i,
                   texHandle,
                   0));
    }
    auto offset = addressToMainMemory(info.depthLocation, info.depthOffset);
    auto format = info.depthFormat == SurfaceDepthFormat::z16 ?
                  GL_DEPTH_COMPONENT16 : GL_DEPTH24_STENCIL8;
    auto tex = searchCache(format, offset, info.width, info.height);
    glcall(glNamedFramebufferTexture(
               _id,
               GL_DEPTH_STENCIL_ATTACHMENT,
               tex->handle(),
               0));

    GLenum enabled[] = {
        info.colorTarget[0] ? GL_COLOR_ATTACHMENT0 : (GLenum) GL_NONE,
        info.colorTarget[1] ? GL_COLOR_ATTACHMENT1 : (GLenum) GL_NONE,
        info.colorTarget[2] ? GL_COLOR_ATTACHMENT2 : (GLenum) GL_NONE,
        info.colorTarget[3] ? GL_COLOR_ATTACHMENT3 : (GLenum) GL_NONE
    };
    glcall(glNamedFramebufferDrawBuffers(_id, 4, enabled));
    assert(glCheckNamedFramebufferStatus(_id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glcall(glBindFramebuffer(GL_FRAMEBUFFER, _id));
}

void GLFramebuffer::dumpTextures() {
    for (int i = 0; i < 4; ++i) {
        if (_info.colorTarget[i]) {
            auto offset = addressToMainMemory(_info.colorLocation[i], _info.colorOffset[i]);
            dumpTexture(_cache[offset].get(), offset);
        }
    }
    auto offset = addressToMainMemory(_info.depthLocation, _info.depthOffset);
    dumpTexture(_cache[offset].get(), offset);
}

void GLFramebuffer::updateTexture() {
    // TODO: draw -> dump -> cpu updates texture -> (update) -> flip
}

GLSimpleTexture* GLFramebuffer::findTexture(ps3_uintptr_t va) {
    auto it = _cache.find(va);
    assert(it != end(_cache));
    return it->second.get();
}

ps3_uintptr_t addressToMainMemory(MemoryLocation location, ps3_uintptr_t address) {
    return address + (location == MemoryLocation::Local ? GcmLocalMemoryBase : 0);
}
