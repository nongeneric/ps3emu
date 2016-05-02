#pragma once

#include "ui_MainWindow.h"
#include <QMainWindow>
#include <string>
#include <memory>
#include "../ps3emu/gcmviz/GcmDatabase.h"

class Process;
class Rsx;
class MainWindowModel {
    Ui::MainWindow _window;
    QMainWindow _qwindow;
    GcmDatabase _db;
    unsigned _lastDrawCount;
    std::unique_ptr<Process> _proc;
    std::unique_ptr<Rsx> _rsx;
    unsigned _currentCommand;
    unsigned _currentFrame;
    void changeFrame();
    void updateContextTable();
    
public:
    MainWindowModel();
    ~MainWindowModel();
    QMainWindow* window();
    void loadTrace(std::string path);
    void update();
    void runTo(unsigned lastCommand, unsigned frame);
public slots:
    void onRun();
};