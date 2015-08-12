#pragma once

#include <QMainWindow>
#include <MonospaceGrid.h>
#include "DebuggerModel.h"

class MainWindow : public QMainWindow {
     Q_OBJECT
     DebuggerModel _model;
     void setupDocks();
     void setupMenu();
     void setupStatusBar();
public slots:
    void openFile();
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
};