#pragma once

#include <string>

void dumpOpenGLTexture(unsigned texture,
                       bool cube,
                       unsigned level,
                       std::string pngPath,
                       bool rgb = false,
                       bool flip = false);
void dumpOpenGLTextureAllImages(unsigned texture,
                                bool cube,
                                unsigned levels,
                                std::string directory,
                                std::string name);
