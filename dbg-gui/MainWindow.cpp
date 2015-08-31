#include "MainWindow.h"
#include "CommandLineEdit.h"

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
    gprGrid->setMinimumWidth(380);
    
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
    
    auto dasmGrid = new MonospaceGrid();
    dasmGrid->setModel(_model.getDasmModel());
    dasmGrid->setColumnWidth(0, 17);
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
    
    auto restart = new QAction("Restart", this);
    restart->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_F9));
    connect(restart, &QAction::triggered, this, [=]() { _model.restart(); });
    trace->addAction(restart);
    
    auto view = menuBar()->addMenu("&View");
    auto toggleFPR = new QAction("Toggle GPR/FPR", this);
    toggleFPR->setShortcut(QKeySequence(Qt::Key_Tab));
    connect(toggleFPR, &QAction::triggered, this, [=]() { _model.toggleFPR(); });
    view->addAction(toggleFPR);
}

void MainWindow::openFile() {
#if DEBUG
    auto args = QStringList() << "a.elf" << "-3.4" << "-1" << "9";
    _model.loadFile("/g/ps3/reversing/printf/a.elf", args);
    return;
#endif
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
    
    auto label = new QLabel("Ready");
    label->setAlignment(Qt::AlignRight);    
    statusBar()->addWidget(_command, 1);
    statusBar()->addWidget(label, 1);
    statusBar()->setSizeGripEnabled(false);
    connect(&_model, &DebuggerModel::message, this, [=](QString text){
        label->setText(text);
        _log->append(text);
    });
}
