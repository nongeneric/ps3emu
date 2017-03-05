#include <catch/catch.hpp>

#include "ps3emu/rsx/GLBuffer.h"
#include "ps3emu/rsx/GLProgram.h"
#include "ps3emu/rsx/GLShader.h"
#include "ps3emu/rsx/GLTexture.h"
#include "ps3emu/rsx/RsxTextureReader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <fstream>

void glDebugCallbackFunction(GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar *message,
            const void *userParam) {
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        REQUIRE(!message);
    }
}

std::vector<uint8_t> dumpTexture(GLuint texture, std::string path, unsigned width, unsigned height) {
    std::vector<uint8_t> vec;
    vec.resize(width * height * 3);
    glGetTextureImage(texture, 0, GL_RGB, GL_UNSIGNED_BYTE, vec.size(), &vec[0]);
    std::ofstream f(path);
    REQUIRE(f.is_open());
    f.write((const char*)vec.data(), vec.size());
    return vec;
}

TEST_CASE("rsx_texture_basic") {
    REQUIRE(glfwInit());
    auto window = glfwCreateWindow(1024, 768, "test", NULL, NULL);
    REQUIRE(window);
    glfwMakeContextCurrent(window);
    REQUIRE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));
        
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(&glDebugCallbackFunction, nullptr);
    
    {
        auto width = 200;
        auto pitch = width * 4;
        auto height = 200;
        auto bufsize = width * height * 4;
        GLPersistentCpuBuffer buffer(bufsize);
        auto mapped = buffer.mapped();
        for (auto y = 0; y < height; y++) {
            for (auto x = 0; x < width; x++) {
                auto i = y * pitch + x * 4;
                if (y % 2 == 0) {
                    mapped[i + 0] = 0xff; // a
                    mapped[i + 1] = 0xff; // r
                    mapped[i + 2] = 0x00; // g
                    mapped[i + 3] = 0x00; // b
                } else {
                    mapped[i + 0] = 0xff; // a
                    mapped[i + 1] = 0x00; // r
                    mapped[i + 2] = 0xff; // g
                    mapped[i + 3] = 0x00; // b
                }
            }
        }
        
        GLSimpleTexture texture(width, height, GL_RGBA32F);
    
        RsxTextureReader reader;
        reader.init();
        RsxTextureInfo info;
        info.pitch = pitch;
        info.width = width;
        info.height = height;
        info.offset = 0;
        info.format = GcmTextureFormat::A8R8G8B8;
        info.lnUn = GcmTextureLnUn::LN;
        reader.loadTexture(info, texture.handle(), buffer.handle());
        
        auto bytes = dumpTexture(texture.handle(), "/tmp/test.rgb", width, height);
        REQUIRE(bytes[0] == 0xff);
        REQUIRE(bytes[1] == 0x00);
        REQUIRE(bytes[2] == 0x00);
        
        REQUIRE(bytes[3] == 0xff);
        REQUIRE(bytes[4] == 0x00);
        REQUIRE(bytes[5] == 0x00);
        
        REQUIRE(bytes[width * 3 + 0] == 0x00);
        REQUIRE(bytes[width * 3 + 1] == 0xff);
        REQUIRE(bytes[width * 3 + 2] == 0x00);
    }
    
    glfwTerminate();
}
