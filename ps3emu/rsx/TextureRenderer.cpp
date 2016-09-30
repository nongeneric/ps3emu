#include "TextureRenderer.h"
#include "GLTexture.h"

TextureRenderer::TextureRenderer() {
    auto fragmentCode = R""(
        #version 450 core
        out vec4 color;
        layout (binding = 20) uniform sampler2D tex;
        void main(void) {
            vec2 size = textureSize(tex, 0);
            color = texture(tex, gl_FragCoord.xy / size, 0);
            color.a = 1;
        }
    )"";

    auto vertexCode = R""(
        #version 450 core
        layout (location = 0) in vec2 pos;
        out gl_PerVertex {
            vec4 gl_Position;
        };
        void main(void) {
            gl_Position = vec4(pos, 0, 1);
        }
    )"";

    _fragmentShader = FragmentShader(fragmentCode);
    _vertexShader = VertexShader(vertexCode);
    _pipeline.useShader(_fragmentShader);
    _pipeline.useShader(_vertexShader);

    glm::vec2 vertices[] {
        { -1.f, 1.f }, { -1.f, -1.f }, { 1.f, -1.f },
        { -1.f, 1.f }, { 1.f, 1.f }, { 1.f, -1.f }
    };

    _buffer = GLBuffer(GLBufferType::Static, sizeof(vertices), vertices);
}

void TextureRenderer::render(GLSimpleTexture* tex) {
    glcall(glEnableVertexAttribArray(0));
    glcall(glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0));
    glcall(glVertexAttribBinding(0, 0));
    glcall(glBindVertexBuffer(0, _buffer.handle(), 0, sizeof(glm::vec2)));

    auto samplerIndex = 20;
    glcall(glBindTextureUnit(samplerIndex, tex->handle()));
    glcall(glBindSampler(samplerIndex, _sampler.handle()));
    glSamplerParameteri(_sampler.handle(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(_sampler.handle(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _pipeline.bind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glcall(glViewport(0, 0, tex->width(), tex->height()));
    glcall(glDepthRange(0, 1));
    glcall(glDrawArrays(GL_TRIANGLES, 0, 6));

    // TODO: restore vertex attrib if required
    // TODO: restore cullFace if required
}
