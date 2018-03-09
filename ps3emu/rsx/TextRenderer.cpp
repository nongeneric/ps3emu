#include "TextRenderer.h"

#include "GLProgramPipeline.h"
#include "GLTexture.h"
#include "GLBuffer.h"
#include "GLSampler.h"
#include "ps3emu/ImageUtils.h"

#include <unicode/utf8.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <vector>
#include <string>

struct TextLine {
    unsigned x;
    unsigned y;
    std::string text;
};

struct AtlasSlot {
    unsigned x = 0, y = 0, width = 0, height = 0;
    unsigned left = 0, top = 0, advance = 0;
};

struct VertexData {
    glm::ivec2 pos;
    glm::ivec2 texCoord;
    glm::ivec2 size;
};

constexpr auto UniformDataBinding = 20;
constexpr auto TextureBinding = 21;

struct UniformData {
    glm::vec4 foreground;
    glm::vec2 screenSize;
};

struct TextRenderer::impl {
    std::vector<TextLine> lines;
    GLProgramPipeline pipeline;
    std::unique_ptr<GLSimpleTexture> atlas;
    GLSampler sampler;
    std::map<FT_UInt, AtlasSlot> slots;
    FragmentShader fragmentShader;
    VertexShader vertexShader;
    GeometryShader geometryShader;
    GLPersistentCpuBuffer vertexAttrs;
    GLPersistentCpuBuffer uniformBuffer;
    UniformData* uniform;
    unsigned lineY = 0;
    unsigned lineX = 0;
    unsigned lineHeight = 0;
    FT_Library freetype;
    FT_Face face;

    impl(int size)
        : vertexAttrs(200),
          uniformBuffer(sizeof(UniformData)),
          uniform((UniformData*)uniformBuffer.mapped()) {
        initAtlas(size);
        initShaders();
    }

    ~impl() {
        FT_Done_Face(face);
        FT_Done_FreeType(freetype);
    }

    void initAtlas(int size) {
        if (FT_Init_FreeType(&freetype)) {
            ERROR(libs) << "can't initialize FreeType";
            exit(0);
        }
        
        if (FT_New_Face(freetype,
                        "/usr/share/fonts/gnu-free/FreeMono.ttf",
                        0,
                        &face)) {
            ERROR(libs) << "can't initialize font";
            exit(0);
        }
        
        FT_Set_Pixel_Sizes(face, 0, size);
        this->lineHeight = .8f * size;

        const auto textureWidth = 2048u;
        unsigned lineHeight = 0, lineY = 0, lineX = 0;

        FT_UInt glyphIndex;
        auto code = FT_Get_First_Char(face, &glyphIndex);
        for (; glyphIndex != 0; code = FT_Get_Next_Char(face, code, &glyphIndex)) {
            if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER | FT_LOAD_NO_HINTING))
                continue;

            auto g = face->glyph;
            if (g->bitmap.width + lineX > textureWidth) {
                lineX = 0;
                lineY += lineHeight;
                lineHeight = 0;
            }

            auto& slot = slots[glyphIndex];
            slot.x = lineX;
            slot.y = lineY;
            slot.left = g->bitmap_left;
            slot.top = g->bitmap_top;
            slot.advance = g->advance.x / 64;
            slot.width = g->bitmap.width;
            slot.height = g->bitmap.rows;

            lineHeight = std::max(lineHeight, g->bitmap.rows);
            lineX += g->bitmap.width;
        }

        auto textureHeight = lineHeight + lineY;
        std::vector<uint8_t> buffer(textureWidth * textureHeight);

        for (auto& [glyphIndex, slot] : slots) {
            if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER | FT_LOAD_NO_HINTING))
                continue;
            auto g = face->glyph;
            for (auto row = 0u; row < g->bitmap.rows; ++row) {
                memcpy(&buffer[(slot.y + row) * textureWidth + slot.x],
                       g->bitmap.buffer + g->bitmap.pitch * row,
                       g->bitmap.width);
            }
        }

        atlas.reset(new GLSimpleTexture(textureWidth, textureHeight, GL_R8));
        glTextureSubImage2D(atlas->handle(),
                            0,
                            0,
                            0,
                            textureWidth,
                            textureHeight,
                            GL_RED,
                            GL_UNSIGNED_BYTE,
                            &buffer[0]);
    }

    void initShaders() {
        auto vertexCode = R""(
            #version 450 core

            layout (location = 0) in vec2 pos;
            layout (location = 1) in vec2 texcoord;
            layout (location = 2) in vec2 size;

            out vec2 v_texcoord;
            out vec2 v_size;
            out gl_PerVertex {
                vec2 gl_Position;
            };

            void main(void) {
                gl_Position = pos;
                v_texcoord = texcoord;
                v_size = size;
            }
        )"";

        auto geometryCode = R""(
            #version 450 core

            layout (points) in;
            layout (triangle_strip) out;
            layout (max_vertices = 4) out;

            layout (std140, binding = 20) uniform UniformData {
                vec4 foreground;
                vec2 screenSize;
            } u;

            in vec2 v_texcoord[];
            in vec2 v_size[];
            in gl_PerVertex {
                vec2 gl_Position;
            } gl_in[];

            out gl_PerVertex {
                vec4 gl_Position;
            };

            out vec2 g_texcoord;

            void main(void) {
                vec2 pos = gl_in[0].gl_Position.xy / u.screenSize * 2 - 1;
                vec2 offset = v_size[0] / u.screenSize * 2;

                gl_Position = vec4(pos, 0, 1);
                g_texcoord = v_texcoord[0];
                EmitVertex();

                gl_Position = vec4(pos + vec2(offset.x, 0), 0, 1);
                g_texcoord = v_texcoord[0] + vec2(v_size[0].x, 0);
                EmitVertex();

                gl_Position = vec4(pos + vec2(0, offset.y), 0, 1);
                g_texcoord = v_texcoord[0] + vec2(0, v_size[0].y);
                EmitVertex();

                gl_Position = vec4(pos + offset, 0, 1);
                g_texcoord = v_texcoord[0] + v_size[0];
                EmitVertex();
            }
        )"";

        auto fragmentCode = R""(
            #version 450 core

            layout (binding = 21) uniform sampler2D tex;
            layout (std140, binding = 20) uniform UniformData {
                vec4 foreground;
                vec2 screenSize;
            } u;
            in vec2 g_texcoord;
            out vec4 color;

            void main(void) {
                vec2 coord = g_texcoord / textureSize(tex, 0);
                color = vec4(u.foreground.rgb, texture(tex, coord).r * u.foreground.a);
            }
        )"";

        vertexShader = VertexShader(vertexCode);
        geometryShader = GeometryShader(geometryCode);
        fragmentShader = FragmentShader(fragmentCode);
        pipeline.useShader(&vertexShader);
        pipeline.useShader(&geometryShader);
        pipeline.useShader(&fragmentShader);
        pipeline.validate();

        glSamplerParameteri(sampler.handle(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glSamplerParameteri(sampler.handle(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    void render(unsigned screenWidth,
                unsigned screenHeight,
                glm::vec4 foreground,
                glm::vec4 background)
    {
        pipeline.bind();
        uniform->screenSize = { screenWidth, screenHeight };
        uniform->foreground = foreground;

        int length = 0;
        for (auto& line : lines) {
            UChar32 chr = 0, offset = 0;
            for (;;) {
                if ((unsigned)offset == line.text.size())
                    break;
                U8_NEXT(line.text.c_str(), offset, -1, chr)
                if (chr < 0)
                    break;
                length++;
            }
        }

        auto newSize = length * sizeof(VertexData);
        if (vertexAttrs.size() < newSize) {
            vertexAttrs = GLPersistentCpuBuffer(newSize * 2);
        }

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glBindVertexBuffer(0, vertexAttrs.handle(), 0, sizeof(VertexData));
        glVertexAttribFormat(0, 2, GL_INT, GL_FALSE, offsetof(VertexData, pos));
        glVertexAttribFormat(1, 2, GL_INT, GL_FALSE, offsetof(VertexData, texCoord));
        glVertexAttribFormat(2, 2, GL_INT, GL_FALSE, offsetof(VertexData, size));
        glVertexAttribBinding(0, 0);
        glVertexAttribBinding(1, 0);
        glVertexAttribBinding(2, 0);

        glBindBufferBase(GL_UNIFORM_BUFFER,
                         UniformDataBinding,
                         uniformBuffer.handle());

        FT_UInt prevChar = 0;
        auto vertexData = (VertexData*)vertexAttrs.mapped();
        for (auto& line : lines) {
            unsigned x = line.x;
            UChar32 chr = 0, offset = 0;
            for (;;) {
                if ((unsigned)offset == line.text.size())
                    break;
                U8_NEXT(line.text.c_str(), offset, -1, chr)
                if (chr < 0)
                    break;
                auto index = FT_Get_Char_Index(face, chr);
                if (index == 0) {
                    index = FT_Get_Char_Index(face, '?');
                }
                auto& slot = slots[index];
                if (prevChar) {
                    FT_Vector delta;
                    FT_Get_Kerning(face, prevChar, index, FT_KERNING_DEFAULT, &delta);
                    x += delta.x / 64;
                }
                vertexData->pos = { x + slot.left, line.y - slot.top };
                vertexData->texCoord = { slot.x, slot.y };
                vertexData->size = { slot.width, slot.height };
                x += slot.advance;
                vertexData++;
                prevChar = index;
            }
        }

        glBindTextureUnit(TextureBinding, atlas->handle());
        glBindSampler(TextureBinding, sampler.handle());

        glDisable(GL_CULL_FACE);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, screenWidth, screenHeight);
        glDepthRange(0, 1);

        glDrawArrays(GL_POINTS, 0, length);
    }
};

TextRenderer::TextRenderer(int size) : p(new impl(size)) {}

TextRenderer::~TextRenderer() = default;

void TextRenderer::move(unsigned x, unsigned y) {
    p->lineX = x;
    p->lineY = y + p->lineHeight; // approximately the baseline
}

void TextRenderer::line(unsigned x, unsigned y, std::string_view text) {
    move(x, y);
    line(text);
}

void TextRenderer::line(std::string_view text) {
    p->lines.push_back({p->lineX, p->lineY, std::string(text)});
    p->lineY += 1.4f * p->lineHeight;
}

void TextRenderer::render(unsigned screenWidth,
                          unsigned screenHeight,
                          glm::vec4 foreground,
                          glm::vec4 background) {
    p->render(screenWidth, screenHeight, foreground, background);
}

void TextRenderer::clear() {
    p->lines.clear();
}
