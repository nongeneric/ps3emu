#pragma once

#include "ps3emu/rsx/GLBuffer.h"
#include "ps3emu/rsx/GLProgramPipeline.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <QOpenGLWidget>
#include <vector>
#include <memory>
#include <array>
#include <functional>

struct PreviewVertex {
    std::array<float, 3> xyz;
};

class OpenGLPreviewWidget : public QOpenGLWidget {
    Q_OBJECT
    
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void initializeGL() override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    
    void updateCamera();
    
    std::unique_ptr<GLBuffer> _vertexBuffer;
    std::unique_ptr<GLBuffer> _axesVertexBuffer;
    std::unique_ptr<GLPersistentCpuBuffer> _matrixUniformBuffer;
    glm::mat4* _viewMatrix;
    std::unique_ptr<GLProgramPipeline> _pipeline;
    std::unique_ptr<VertexShader> _vs;
    std::unique_ptr<FragmentShader> _fs;
    int _vertices;
    int _axes;
    float _x, _y, _z;
    int _lastMousePosX, _lastMousePosY;
    bool _isDrag;
    float _scale;
    float _rotX, _rotY;
    bool _initialized;
    unsigned _mode;
    std::vector<std::function<void()>> _afterInit;
    
public:
    OpenGLPreviewWidget(QWidget* parent = nullptr);
    void setVertices(std::vector<PreviewVertex> const& vertices);
    void setMode(unsigned mode);
};
