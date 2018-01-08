#pragma once

#include <QMainWindow>
#include <MonospaceGrid.h>
#include "DebuggerModel.h"

class QTextEdit;
class CommandLineEdit;
class QDockWidget;
class MainWindow : public QMainWindow {
     Q_OBJECT
     
     QTextEdit* _log;
     CommandLineEdit* _command;
     DebuggerModel _model;
     QDockWidget* _bottomLogDock;
     void setupDocks();
     void setupMenu();
     void setupStatusBar();
     
public slots:
    void openFile();
    
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void loadElf(QString path, QStringList args);
};
