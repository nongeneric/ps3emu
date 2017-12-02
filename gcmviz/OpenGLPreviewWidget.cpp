#include "OpenGLPreviewWidget.h"
#include "ps3emu/utils.h"
#include "ps3emu/log.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <QEvent>
#include <QMouseEvent>

#define POS_ATTRIB_INDEX 0
#define ISLINES_ATTRIB_LOCATION 1

#define POS_BINDING_INDEX 0
#define AXES_BINDING_INDEX 1
#define MVP_BINDING_INDEX 2

const char* vsText =
R""(
    #version 450 core
    
    layout (location = 0) in vec4 position;
    layout (std140, binding = 2) uniform MVP {
        mat4 mvp;
    } u;
    
    out vec4 vs_position;
    out gl_PerVertex {
        vec4 gl_Position;
    };
    
    void main(void) {
        vs_position = u.mvp * position;
        gl_Position = vs_position;
    }
)"";

const char* fsText =
R""(
    #version 450 core
    
    layout (location = 1) uniform float isLines;
    
    in vec4 vs_position;
    out vec4 color;
    
    void main(void) {
        if (isLines == 1) {
            color = vec4(0, 0, 0, 1);
        } else {
            float zcolor = (vs_position.z + 1) / 2;
            color = vec4(1, 1, 1 - 0.5 * zcolor, 1);
        }
    }
)"";

OpenGLPreviewWidget::OpenGLPreviewWidget(QWidget* parent)
    : QOpenGLWidget(parent),
      _vertices(0),
      _x(0),
      _y(0),
      _z(0),
      _isDrag(false),
      _scale(1),
      _rotX(0),
      _rotY(0),
      _initialized(false),
      _mode(GL_TRIANGLES) {
    QSurfaceFormat format;
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    setFormat(format);
}

void OpenGLPreviewWidget::paintGL() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3, 0.3, 0.3, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glVertexAttribBinding(POS_ATTRIB_INDEX, POS_BINDING_INDEX);
    
    glProgramUniform1f(_fs->handle(), ISLINES_ATTRIB_LOCATION, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(_mode, 0, _vertices);
    
    glProgramUniform1f(_fs->handle(), ISLINES_ATTRIB_LOCATION, 1);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(2);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(_mode, 0, _vertices);
    
    glVertexAttribBinding(POS_ATTRIB_INDEX, AXES_BINDING_INDEX);
    glLineWidth(2);
    glDrawArrays(GL_LINES, 0, _axes);
}

void OpenGLPreviewWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void glDebugCallbackFunction(GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar *message,
            const void *userParam) {
    WARNING(debugger) << ssnprintf("gl callback: %s", message);
    if (severity == GL_DEBUG_SEVERITY_HIGH)
        exit(1);
}

void OpenGLPreviewWidget::initializeGL() {
    if (!gladLoadGL()) {
        throw std::runtime_error("opengl init failed");
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(&glDebugCallbackFunction, nullptr);
    
    _vs.reset(new VertexShader(vsText));
    _fs.reset(new FragmentShader(fsText));
    INFO(debugger) << _vs->log();
    INFO(debugger) << _fs->log();
    _pipeline.reset(new GLProgramPipeline());
    _pipeline->useShader(_vs.get());
    _pipeline->useShader(_fs.get());
    _pipeline->validate();
    _pipeline->bind();
    
    _matrixUniformBuffer.reset(new GLPersistentCpuBuffer(sizeof(glm::mat4)));
    _viewMatrix = (glm::mat4*)_matrixUniformBuffer->mapped();
    *_viewMatrix = glm::mat4();
    glBindBufferBase(GL_UNIFORM_BUFFER,
                     MVP_BINDING_INDEX,
                     _matrixUniformBuffer->handle());
    
    glEnableVertexAttribArray(POS_ATTRIB_INDEX);
    glVertexAttribFormat(POS_ATTRIB_INDEX, 3, GL_FLOAT, GL_FALSE, 0);
    
    std::vector<PreviewVertex> axes = {
        { -1, 0, 0 },
        { 1, 0, 0 },
        { 0, -1, 0 },
        { 0, 1, 0 },
        { 0, 0, -1 },
        { 0, 0, 1 }
    };
    _axes = axes.size();
    _axesVertexBuffer.reset(new GLBuffer(GLBufferType::Static,
                                         sizeof(PreviewVertex) * axes.size(),
                                         &axes[0]));
    glBindVertexBuffer(AXES_BINDING_INDEX, _axesVertexBuffer->handle(), 0, sizeof(PreviewVertex));
    
    _initialized = true;
    for (auto f : _afterInit)
        f();
    if (_afterInit.empty()) {
        std::vector<PreviewVertex> vertices = {
            {0, 0, -1}, {1, 1, -1}, {0, 1, -1}, {0.5, 0, 0}, {1.5, 1, 0}, {0.5, 1, 0},
        };
        setVertices(vertices);
    }
    _afterInit.clear();
    
    updateCamera();
}

void OpenGLPreviewWidget::setVertices(std::vector<PreviewVertex> const& vertices) {
    auto f = [=] {
        auto byteSize = vertices.size() * sizeof(PreviewVertex);
        _vertexBuffer.reset(
            new GLBuffer(GLBufferType::Static, byteSize, &vertices[0]));
        _vertices = vertices.size();
        glBindVertexBuffer(
            POS_BINDING_INDEX, _vertexBuffer->handle(), 0, sizeof(PreviewVertex));
        update();
    };

    if (_initialized) {
        f();
    } else {
        _afterInit.push_back(f);
    }
}

void OpenGLPreviewWidget::mousePressEvent(QMouseEvent* event) {
    _isDrag = true;
    _lastMousePosX = event->x();
    _lastMousePosY = event->y();
}

void OpenGLPreviewWidget::mouseReleaseEvent(QMouseEvent* event) {
    _isDrag = false;
}

void OpenGLPreviewWidget::mouseMoveEvent(QMouseEvent* event) {
    float dx = event->x() - _lastMousePosX;
    float dy = event->y() - _lastMousePosY;
    if (event->modifiers() & Qt::MetaModifier) {
        _rotX += dy / 200.f;
        _rotY += dx / 200.f;
    } else if (event->modifiers() & Qt::ShiftModifier) {
        _z += dy / 200.f;
    } else if (event->modifiers() & Qt::ControlModifier) {
        _scale *= 1.f + dy / 400.f;
    } else {
        _x += dx / 300.f;
        _y -= dy / 200.f;
    }
    _lastMousePosX = event->x();
    _lastMousePosY = event->y();
    updateCamera();
}

void OpenGLPreviewWidget::updateCamera() {
    auto aspect = 1280.f / 768.f;
    glm::mat4 p = glm::ortho(-2.f, 2.f, -2.f / aspect, 2.f / aspect, -2.f, 2.f);
    glm::mat4 v;
    v = glm::scale(v, glm::vec3(_scale));
    v = glm::translate(v, glm::vec3(_x, _y, _z));
    v = glm::rotate(v, _rotX, glm::vec3(1, 0, 0));
    v = glm::rotate(v, _rotY, glm::vec3(0, 1, 0));
    *_viewMatrix = p * v;
    update();
}

void OpenGLPreviewWidget::setMode(unsigned mode) {
    _mode = mode;
    update();
}
