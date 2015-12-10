#pragma once

#include "../ps3emu/PPU.h"
#include "../ps3emu/ELFLoader.h"
#include "MonospaceGrid.h"
#include <QString>
#include <string>
#include <memory>

class GPRModel;
class DasmModel;
class Rsx;
class DebuggerModel : public QWidget {
    Q_OBJECT
    
    ELFLoader _elf;
    std::unique_ptr<GPRModel> _gprModel;
    std::unique_ptr<DasmModel> _dasmModel;
    std::unique_ptr<PPU> _ppu;
    std::unique_ptr<Rsx> _rsx;
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
    void restart();
signals:
    void message(QString text);
};
