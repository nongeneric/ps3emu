#pragma once

#include "ui_MainWindow.h"
#include <QMainWindow>
#include <string>
#include "../ps3emu/gcmviz/GcmDatabase.h"

class MainWindowModel {
    Ui::MainWindow _window;
    QMainWindow _qwindow;
    GcmDatabase _db;
public:
    MainWindowModel();
    QMainWindow* window();
    void loadTrace(std::string path);
};