#include "MainWindow.h"
#include <QDockWidget>
#include <QMenuBar>
#include <QApplication>
#include <QFileDialog>

MainWindow::~MainWindow() { }

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(1200, 800);
    setupMenu();
    setupDocks();
}

void MainWindow::setupDocks() {
    auto gprGrid = new MonospaceGrid();
    gprGrid->setModel(_model.getGRPModel());
    gprGrid->setColumnWidth(0, 7);
    gprGrid->setColumnWidth(1, 16);
    gprGrid->setMinimumWidth(200);
    
    auto gprDock = new QDockWidget(this);
    gprDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    gprDock->setWidget(gprGrid);
    gprDock->setTitleBarWidget(new QWidget(this));
    addDockWidget(Qt::RightDockWidgetArea, gprDock);
    
    auto dasmGrid = new MonospaceGrid();
    dasmGrid->setModel(_model.getDasmModel());
    dasmGrid->setColumnWidth(0, 16);
    dasmGrid->setColumnWidth(1, 12);
    dasmGrid->setColumnWidth(2, 100);
    setCentralWidget(dasmGrid);
}

void MainWindow::setupMenu() {
    auto file = menuBar()->addMenu("&File");
    auto open = new QAction("&Open", this);
    open->setShortcut(QKeySequence::Open);
    connect(open, &QAction::triggered, this, &MainWindow::openFile);
    file->addAction(open);
    
    auto exit = new QAction("&Exit", this);
    connect(exit, &QAction::triggered, this, []() { QApplication::quit(); });
    file->addAction(exit);
}

void MainWindow::openFile() {
#if DEBUG
    _model.loadFile("/g/ps3/reversing/simple_printf/a.elf");
    return;
#endif
    auto path = QFileDialog::getOpenFileName(this, "Open ELF executable");
    if (!path.isEmpty()) {
        _model.loadFile(path);
    }
}
