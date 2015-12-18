#pragma once

#include "../ps3emu/Process.h"
#include "MonospaceGrid.h"
#include <QString>
#include <string>
#include <memory>

class GPRModel;
class DasmModel;
class Rsx;
class DebuggerModel : public QWidget {
    Q_OBJECT

    PPUThread* _activeThread;
    std::unique_ptr<GPRModel> _gprModel;
    std::unique_ptr<DasmModel> _dasmModel;
    std::unique_ptr<Process> _proc;
    bool _elfLoaded = false;
    void log(std::string str);
    void printMemory(uint64_t va);
    void traceTo(ps3_uintptr_t va);
    void updateUI();
public:
    DebuggerModel();
    ~DebuggerModel();
    MonospaceGridModel* getDasmModel();
    MonospaceGridModel* getGPRModel();
    MonospaceGridModel* getMemoryDumpModel();
    MonospaceGridModel* getStackModel();
    MonospaceGridModel* getOtherAuxRegistersModel();
    void toggleFPR();
    void loadFile(QString path, QStringList args);
    void exec(QString command);
    void stepIn(bool updateUI = true);
    void stepOver();
    void run();
    void runto(ps3_uintptr_t va);
    void runToLR();
signals:
    void message(QString text);
};
