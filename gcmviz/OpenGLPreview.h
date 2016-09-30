#pragma once

#include <QDialog>
#include "ui_OpenGLPreview.h"

class OpenGLPreviewWidget;
class OpenGLPreview : public QDialog {
    Ui::OpenGLPreview _ui;
    
public:
    OpenGLPreview(QWidget* parent = nullptr);
    OpenGLPreviewWidget* widget();
};
