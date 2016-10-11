#include "MainWindow.h"
#include "CommandLineEdit.h"
#include "Config.h"

#include <QDockWidget>
#include <QMenuBar>
#include <QApplication>
#include <QFileDialog>
#include <QStatusBar>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QKeyEvent>

MainWindow::~MainWindow() { }

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(1200, 900);
    setupMenu();
    setupDocks();
    setupStatusBar();
    
    _command->setFocus();
}

void MainWindow::setupDocks() {
    auto gprGrid = new MonospaceGrid();
    gprGrid->setModel(_model.getGPRModel());
    gprGrid->setColumnWidth(0, 5);
    gprGrid->setColumnWidth(1, 17);
    gprGrid->setColumnWidth(2, 7);
    gprGrid->setColumnWidth(3, 16);
    gprGrid->setScrollable(true);
    gprGrid->setMinimumWidth(530);
    
    auto gprDock = new QDockWidget(this);
    gprDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    gprDock->setWidget(gprGrid);
    gprDock->setTitleBarWidget(new QWidget(this));
    addDockWidget(Qt::RightDockWidgetArea, gprDock);
    
    auto bottomDock = new QDockWidget(this);
    _log = new QTextEdit();
    _log->setReadOnly(true);
    _log->setFont(QFont("monospace", 10));
    bottomDock->setWidget(_log);
    addDockWidget(Qt::BottomDockWidgetArea, bottomDock);
    
    auto memoryGrid = new MonospaceGrid();
    memoryGrid->setModel(_model.getMemoryDumpModel());
    memoryGrid->setColumnWidth(0, 9);
    memoryGrid->setColumnWidth(1, 48);
    memoryGrid->setColumnWidth(2, 17);
    memoryGrid->setScrollable(true);
    memoryGrid->setMinimumWidth(260);
    
    auto memoryDock = new QDockWidget(this);
    memoryDock->setWidget(memoryGrid);
    addDockWidget(Qt::BottomDockWidgetArea, memoryDock);
    
    auto dasmGrid = new MonospaceGrid();
    dasmGrid->setNavigationMode(NavigationMode::Fuzzy);
    dasmGrid->setModel(_model.getDasmModel());
    dasmGrid->setColumnWidth(0, 9);
    dasmGrid->setColumnWidth(1, 12);
    dasmGrid->setColumnWidth(2, 12);
    dasmGrid->setColumnWidth(3, 30);
    dasmGrid->setColumnWidth(4, 100);
    dasmGrid->setArrowsColumn(2);
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
    
    auto runToLR = new QAction("Run to LR", this);
    runToLR->setShortcut(QKeySequence(Qt::Key_F4));
    connect(runToLR, &QAction::triggered, this, [=]() { _model.runToLR(); });
    trace->addAction(runToLR);
    
    auto run = new QAction("Run", this);
    run->setShortcut(QKeySequence(Qt::Key_F5));
    connect(run, &QAction::triggered, this, [=]() { _model.run(); });
    trace->addAction(run);
    
//     auto restart = new QAction("Restart", this);
//     restart->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_F9));
//     connect(restart, &QAction::triggered, this, [=]() { _model.restart(); });
//     trace->addAction(restart);
    
    auto view = menuBar()->addMenu("&View");
    auto toggleFPR = new QAction("Toggle GPR/FPR", this);
    toggleFPR->setShortcut(QKeySequence(Qt::Key_Tab));
    connect(toggleFPR, &QAction::triggered, this, [=]() { _model.toggleFPR(); });
    view->addAction(toggleFPR);
    
    auto settings = menuBar()->addMenu("Settings");
    { 
        auto action = new QAction("Stop at new SPU Thread", this);
        action->setCheckable(true);
        action->setChecked(g_config.config().StopAtNewSpuThread);
        connect(action, &QAction::triggered, this, [=]() {
            g_config.config().StopAtNewSpuThread = action->isChecked();
            g_config.save();
        });
        settings->addAction(action);
    }
    {
        auto action = new QAction("Stop at new PPU Thread", this);
        action->setCheckable(true);
        action->setChecked(g_config.config().StopAtNewPpuThread);
        connect(action, &QAction::triggered, this, [=]() {
            g_config.config().StopAtNewPpuThread = action->isChecked();
            g_config.save();
        });
        settings->addAction(action);
    }
    { 
        auto action = new QAction("Stop at new module", this);
        action->setCheckable(true);
        action->setChecked(g_config.config().StopAtNewModule);
        connect(action, &QAction::triggered, this, [=]() {
            g_config.config().StopAtNewModule = action->isChecked();
            g_config.save();
        });
        settings->addAction(action);
    }
    {
        auto action = new QAction("Log SPU", this);
        action->setCheckable(true);
        action->setChecked(g_config.config().LogSpu);
        connect(action, &QAction::triggered, this, [=]() {
            g_config.config().LogSpu = action->isChecked();
            g_config.save();
        });
        settings->addAction(action);
    }
    {
        auto action = new QAction("Log RSX", this);
        action->setCheckable(true);
        action->setChecked(g_config.config().LogRsx);
        connect(action, &QAction::triggered, this, [=]() {
            g_config.config().LogRsx = action->isChecked();
            g_config.save();
        });
        settings->addAction(action);
    }
    {
        auto action = new QAction("Log Dates", this);
        action->setCheckable(true);
        action->setChecked(g_config.config().LogDates);
        connect(action, &QAction::triggered, this, [=]() {
            g_config.config().LogDates = action->isChecked();
            g_config.save();
        });
        settings->addAction(action);
    }
    {
        auto action = new QAction("SPURS Trace", this);
        action->setCheckable(true);
        action->setChecked(g_config.config().EnableSpursTrace);
        connect(action, &QAction::triggered, this, [=]() {
            g_config.config().EnableSpursTrace = action->isChecked();
            g_config.save();
        });
        settings->addAction(action);
    }
}

void MainWindow::openFile() {
    auto path = QFileDialog::getOpenFileName(this, "Open ELF executable");
    if (!path.isEmpty()) {
        _model.loadFile(path, {});
    }
}

void MainWindow::setupStatusBar() {
    _command = new CommandLineEdit(this);
    connect(_command, &CommandLineEdit::selected, this, [=] {
        auto line = _command->text();
        _command->clear();
       _log->append(QString("executing \"%1\"").arg(line));
       _model.exec(line);
    });
    
    statusBar()->addWidget(_command, 1);
    statusBar()->setSizeGripEnabled(false);
    connect(&_model, &DebuggerModel::message, this, [=](QString text){
        _log->append(text);
    });
}

void MainWindow::loadElf(QString path, QStringList args) {
    _model.loadFile(path, args); 
}
