#pragma once

#include <QMainWindow>

class MainWindow : public QMainWindow {
     Q_OBJECT
     
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
};