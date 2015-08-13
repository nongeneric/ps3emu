#include "MainWindow.h"

#include <QDockWidget>
#include <QMenuBar>
#include <QApplication>
#include <QFileDialog>
#include <QStatusBar>
#include <QLabel>

MainWindow::~MainWindow() { }

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(1200, 800);
    setupMenu();
    setupDocks();
    setupStatusBar();
}

void MainWindow::setupDocks() {
    auto gprGrid = new MonospaceGrid();
    gprGrid->setModel(_model.getGRPModel());
    gprGrid->setColumnWidth(0, 5);
    gprGrid->setColumnWidth(1, 17);
    gprGrid->setColumnWidth(2, 7);
    gprGrid->setColumnWidth(3, 16);
    gprGrid->setMinimumWidth(380);
    
    auto gprDock = new QDockWidget(this);
    gprDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    gprDock->setWidget(gprGrid);
    gprDock->setTitleBarWidget(new QWidget(this));
    addDockWidget(Qt::RightDockWidgetArea, gprDock);
    
    auto logGrid = new MonospaceGrid();
    logGrid->setModel(_model.getLogModel());
    logGrid->setColumnWidth(0, 500);
    logGrid->setMinimumHeight(300);
    logGrid->setScrollable(true);
    
    auto bottomDock = new QDockWidget(this);
    bottomDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    bottomDock->setWidget(logGrid);
    bottomDock->setTitleBarWidget(new QWidget(this));
    addDockWidget(Qt::BottomDockWidgetArea, bottomDock);
    
    auto dasmGrid = new MonospaceGrid();
    dasmGrid->setModel(_model.getDasmModel());
    dasmGrid->setColumnWidth(0, 17);
    dasmGrid->setColumnWidth(1, 12);
    dasmGrid->setColumnWidth(2, 30);
    dasmGrid->setColumnWidth(3, 100);
    dasmGrid->setScrollable(true);
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
    
    auto trace = menuBar()->addMenu("&Trace");
    auto stepIn = new QAction("Step In", this);
    stepIn->setShortcut(QKeySequence(Qt::Key_F7));
    connect(stepIn, &QAction::triggered, this, [=]() { _model.stepIn(); });
    trace->addAction(stepIn);
    
    auto stepOver = new QAction("Step Over", this);
    stepOver->setShortcut(QKeySequence(Qt::Key_F8));
    connect(stepOver, &QAction::triggered, this, [=]() { _model.stepOver(); });
    trace->addAction(stepOver);
    
    auto run = new QAction("Run", this);
    run->setShortcut(QKeySequence(Qt::Key_F5));
    connect(run, &QAction::triggered, this, [=]() { _model.run(); });
    trace->addAction(run);
    
    auto restart = new QAction("Restart", this);
    restart->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_F9));
    connect(restart, &QAction::triggered, this, [=]() { _model.restart(); });
    trace->addAction(restart);
    
    auto view = menuBar()->addMenu("&View");
    auto toggleLog = new QAction("Toggle Log", this);
    toggleLog->setShortcut(QKeySequence(Qt::Key_F1));
    connect(toggleLog, &QAction::triggered, this, [=]() { _model.stepIn(); });
    view->addAction(toggleLog);
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

void MainWindow::setupStatusBar() {
    auto label = new QLabel("Ready");
    label->setAlignment(Qt::AlignLeft);
    statusBar()->addPermanentWidget(label);
    statusBar()->setSizeGripEnabled(false);
    connect(&_model, &DebuggerModel::message, this, [=](QString text){
        label->setText(text);
    });
}
