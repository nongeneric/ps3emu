#include "message.h"

#include "Controller.h"
#include "ps3emu/log.h"
#include "ps3emu/rsx/GLProgramPipeline.h"
#include "ps3emu/rsx/GLShader.h"
#include "ps3emu/rsx/GLSampler.h"
#include "ps3emu/rsx/GLBuffer.h"
#include "ps3emu/rsx/GLTexture.h"
#include "libsysutil.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <assert.h>
#include <boost/regex/pending/unicode_iterator.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
namespace {
    using u32_string = std::basic_string<uint32_t>;
    
    struct Context {
        GLProgramPipeline pipeline;
        VertexShader vertexShader;
        FragmentShader fragmentShader;
        FragmentShader textFragmentShader;
        GLSampler sampler;
        GLBuffer buffer;
        GLuint textVerticesBuffer;
        std::unique_ptr<GLSimpleTexture> texture;
        FT_Library freetype;
        FT_Face face;
    };
    Context* context = nullptr;
    bool g_shown = false;
    uint32_t g_type;
    u32_string g_message;
    std::string g_u8message;
    uint32_t g_callback = 0;
    uint32_t g_userData;
    int g_controller;
    uint32_t g_result;
}

#define CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE (0 << 4)
#define CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO (1 << 4)
#define CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK (2 << 4)

enum CellMsgDialogButtonType {
    CELL_MSGDIALOG_BUTTON_NONE = -1,
    CELL_MSGDIALOG_BUTTON_INVALID = 0,
    CELL_MSGDIALOG_BUTTON_OK = 1,
    CELL_MSGDIALOG_BUTTON_YES = 1,
    CELL_MSGDIALOG_BUTTON_NO = 2,
    CELL_MSGDIALOG_BUTTON_ESCAPE = 3
};

void initContext() {
    if (context)
        return;
        
    context = new Context();

    if (FT_Init_FreeType(&context->freetype)) {
        ERROR(libs) << "can't initialize FreeType";
        exit(0);
    }
    
    if (FT_New_Face(context->freetype,
                    "/usr/share/fonts/gnu-free/FreeSans.ttf",
                    0,
                    &context->face)) {
        ERROR(libs) << "can't initialize font";
        exit(0);
    }
    
    FT_Set_Pixel_Sizes(context->face, 0, 24);
    
    auto vertexCode = R""(
        #version 450 core
        
        layout (location = 0) in vec2 pos;
        layout (location = 1) in vec2 texcoord;
        out vec2 v_texcoord;
        
        out gl_PerVertex {
            vec4 gl_Position;
        };
        
        void main(void) {
            gl_Position = vec4(pos, 0, 1);
            v_texcoord = texcoord;
        }
    )"";
    
    auto fragmentCode = R""(
        #version 450 core
        
        out vec4 color;
        
        void main(void) {
            color = vec4(1, 1, 1, 0.8);
        }
    )"";
    
    auto textFragmentCode = R""(
        #version 450 core
        
        layout (binding = 20) uniform sampler2D tex;
        in vec2 v_texcoord;
        out vec4 color;
        
        void main(void) {
            color = vec4(0, 0, 0, 1 * texture(tex, v_texcoord).r);
        }
    )"";

    context->fragmentShader = FragmentShader(fragmentCode);
    context->textFragmentShader = FragmentShader(textFragmentCode);
    context->vertexShader = VertexShader(vertexCode);

    glm::vec2 vertices[] {
        { -1.f, 0.4f }, { -1.f, -0.4f }, { 1.f, -0.4f },
        { -1.f, 0.4f }, { 1.f, 0.4f }, { 1.f, -0.4f }
    };

    context->buffer = GLBuffer(GLBufferType::Static, sizeof(vertices), vertices);
    context->texture.reset(new GLSimpleTexture(50, 50, GL_R8));
    glCreateBuffers(1, &context->textVerticesBuffer);
}

int32_t cellMsgDialogOpen2(uint32_t type,
                           cstring_ptr_t msgString,
                           uint32_t func,
                           uint32_t userData,
                           uint32_t extParam) {
    INFO(libs) << ssnprintf("cellMsgDialogOpen2(\"%s\")", msgString.str);
    assert(extParam == 0);
    g_shown = true;
    g_type = type;
//     using iter_t = boost::u8_to_u32_iterator<typename std::string::iterator>;
//     iter_t it(msgString.str.begin(), msgString.str.begin(), msgString.str.end());
//     context->message = u32_string(it, iter_t());
    g_u8message = msgString.str;
    g_callback = func;
    g_userData = userData;
    
    CellPadInfo2 info;
    cellPadGetInfo2(&info);
    
    for (int i = 0; i < CELL_PAD_MAX_PORT_NUM; ++i) {
        if (info.port_status[i] == CELL_PAD_STATUS_CONNECTED) {
            g_controller = i;
            break;
        }
    }
    return CELL_OK;
}

void drawText(float x, float y, float width, float height, std::string text) {
    auto sx = 2.f / width, sy = 2.f / height;
    for (auto ch : text) {
        if (ch == '\n') {
            y -= 0.08;
            x = -0.8;
            continue;
        }
        
        if (FT_Load_Char(context->face, ch, FT_LOAD_RENDER))
            continue;
        
        FT_GlyphSlot g = context->face->glyph;
    
        glTextureSubImage2D(context->texture->handle(),
                            0,
                            0,
                            0,
                            g->bitmap.width,
                            g->bitmap.rows,
                            GL_RED,
                            GL_UNSIGNED_BYTE,
                            g->bitmap.buffer);
    
        float x2 = x + g->bitmap_left * sx;
        float y2 = -y - g->bitmap_top * sy;
        float w = g->bitmap.width * sx;
        float h = g->bitmap.rows * sy;
    
        auto stx = (float)g->bitmap.width / context->texture->width();
        auto sty = (float)g->bitmap.rows / context->texture->height();
        
        GLfloat box[4][4] = {
            {x2, y2, 0, 0},
            {x2 + w, y2, stx, 0},
            {x2, y2 + h, 0, sty},
            {x2 + w, y2 + h, stx, sty},
        };
        
        glNamedBufferData(context->textVerticesBuffer, sizeof box, box, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
        x += (g->advance.x / 64) * sx;
        y += (g->advance.y / 64) * sy;
    }
}

void emuMessageDraw(uint32_t width, uint32_t height) {
    initContext();
    if (!g_shown)
        return;
    
    CellPadData data;
    cellPadGetData(g_controller, &data);
    bool isCross = data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CROSS;
    bool isCircle = data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE;
    if (g_type & CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK) {
        if (isCross || isCircle) {
            g_shown = false;
            g_result = CELL_MSGDIALOG_BUTTON_OK;
        }
    } else if (g_type & CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO) {
        if (isCross) {
            g_shown = false;
            g_result = CELL_MSGDIALOG_BUTTON_YES;
        } else if (isCircle) {
            g_shown = false;
            g_result = CELL_MSGDIALOG_BUTTON_NO;
        }
    } else {
        if (isCircle) {
            g_shown = false;
            g_result = CELL_MSGDIALOG_BUTTON_NONE;
        }
    }
    
    if (!g_shown) {
        emuCallback(g_callback, { g_result, g_userData }, false);
    }
    
    glEnableVertexAttribArray(0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexAttribBinding(0, 0);
    glBindVertexBuffer(0, context->buffer.handle(), 0, sizeof(glm::vec2));
    
    context->pipeline.useShader(&context->fragmentShader);
    context->pipeline.useShader(&context->vertexShader);
    context->pipeline.bind();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);
    glDepthRange(0, 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    context->pipeline.useShader(&context->textFragmentShader);
    
    auto samplerIndex = 20;
    glBindTextureUnit(samplerIndex, context->texture->handle());
    glBindSampler(samplerIndex, context->sampler.handle());
    glSamplerParameteri(context->sampler.handle(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(context->sampler.handle(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glEnableVertexAttribArray(0);
    glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexAttribBinding(0, 0);
    glBindVertexBuffer(0, context->textVerticesBuffer, 0, sizeof(float) * 4);
    
    glEnableVertexAttribArray(1);
    glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2);
    glVertexAttribBinding(1, 1);
    glBindVertexBuffer(1, context->textVerticesBuffer, 0, sizeof(float) * 4);
    
    drawText(-0.8f, 0.3f, width, height, g_u8message);
    if (g_type & CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK) {
        drawText(0.8, -0.3, width, height, "[OK]");
    } else if (g_type & CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO) {
        drawText(0.8, -0.3, width, height, "[YES / NO]");
    }
}

int32_t cellMsgDialogProgressBarSetMsg(uint32_t progressbarIndex,
                                       cstring_ptr_t msgString) {
    return CELL_OK;
}

int32_t cellMsgDialogClose(float delayTime) {
    g_shown = false;
    emuCallback(g_callback, { g_result, g_userData }, false);
    return CELL_OK;
}

int32_t cellMsgDialogProgressBarInc(uint32_t progressbarIndex, uint32_t delta) {
    return CELL_OK;
}

int32_t cellMsgDialogAbort() {
    g_shown = false;
    return CELL_OK;
}
