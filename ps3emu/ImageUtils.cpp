#include "ImageUtils.h"
#include "fileutils.h"
#include "ps3emu/utils.h"

#include <png.h>
#include <glad/glad.h>
#include <vector>
#include <stdint.h>
#include <assert.h>

void dumpOpenGLTexture(unsigned texture, bool cube, unsigned level, std::string pngPath, bool rgb, bool flip) {
    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    int width, height, depth = cube ? 6 : 1;
    glGetTextureLevelParameteriv(texture, level, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(texture, level, GL_TEXTURE_HEIGHT, &height);
    std::vector<uint8_t> raw;
    raw.resize(width * height * depth * (rgb ? 3 : 4));
    glGetTextureImage(texture, level, rgb ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, raw.size(), &raw[0]);
    
    //write_all_bytes(&raw[0], raw.size(), pngPath + ".rgba");
    
    auto png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    auto info_ptr = png_create_info_struct(png_ptr);
    assert(png_ptr && info_ptr);
    
    auto f = fopen(pngPath.c_str(), "w");
    
    png_init_io(png_ptr, f);
    png_set_IHDR(png_ptr,
                 info_ptr,
                 width,
                 height * depth,
                 8,
                 rgb ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);
    if (flip) {
        for (int y = height * depth - 1; y >= 0; --y) {
            png_write_row(png_ptr, &raw[y * width * (rgb ? 3 : 4)]);
        }
    } else {
        for (auto y = 0; y < height * depth; ++y) {
            png_write_row(png_ptr, &raw[y * width * (rgb ? 3 : 4)]);
        }
    }
    png_write_end(png_ptr, NULL);
    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    
    fclose(f);
}

void dumpOpenGLTextureAllImages(unsigned texture,
                                bool cube,
                                unsigned levels,
                                std::string directory,
                                std::string fileName) {
    for (auto i = 0u; i < levels; ++i) {
        auto name = ssnprintf("%s/%s_level%d.png", directory, fileName, i);
        dumpOpenGLTexture(texture, cube, i, name);
    }
}
