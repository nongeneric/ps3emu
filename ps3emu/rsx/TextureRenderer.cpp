#include "TextureRenderer.h"
#include "GLTexture.h"

TextureRenderer::TextureRenderer() {
    auto fragmentCode =
        "#version 450 core\n"
        "out vec4 color;\n"
        "layout (binding = 20) uniform sampler2D tex;\n"
        "void main(void) {\n"
        "    vec2 size = textureSize(tex, 0);\n"
        "    color = texture(tex, gl_FragCoord.xy / size, 0);\n"
        "}";

    auto vertexCode =
        "#version 450 core\n"
        "layout (location = 0) in vec2 pos;\n"
        "out gl_PerVertex {\n"
        "    vec4 gl_Position;\n"
        "};\n"
        "void main(void) {\n"
        "    gl_Position = vec4(pos, 0, 1);\n"
        "}\n";

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

    glClearDepth(1);
    glClear(GL_DEPTH_BUFFER_BIT);
    glcall(glViewport(0, 0, tex->width(), tex->height()));
    glcall(glDepthRange(0, 1));
    glcall(glDrawArrays(GL_TRIANGLES, 0, 6));

    // TODO: restore vertex attrib if required
}
