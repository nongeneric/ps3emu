#include "OpenGLPreview.h"

OpenGLPreview::OpenGLPreview(QWidget* parent) {
    _ui.setupUi(this);
}

OpenGLPreviewWidget* OpenGLPreview::widget() {
    return _ui.openGLWidget;
}
