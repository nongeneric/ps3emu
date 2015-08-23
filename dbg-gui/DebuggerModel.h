#pragma once

#include "../ps3emu/PPU.h"
#include "../ps3emu/ELFLoader.h"
#include "MonospaceGrid.h"
#include <QString>
#include <string>
#include <memory>

class GPRModel;
class DasmModel;
class DebuggerModel : public QWidget {
    Q_OBJECT
    
    ELFLoader _elf;
    std::unique_ptr<GPRModel> _grpModel;
    std::unique_ptr<DasmModel> _dasmModel;
    std::unique_ptr<PPU> _ppu;
    bool _elfLoaded = false;
    void log(std::string str);
    void printMemory(uint64_t va);
public:
    DebuggerModel();
    ~DebuggerModel();
    MonospaceGridModel* getDasmModel();
    MonospaceGridModel* getGRPModel();
    MonospaceGridModel* getMemoryDumpModel();
    MonospaceGridModel* getStackModel();
    MonospaceGridModel* getOtherAuxRegistersModel();
    void loadFile(QString path);
    void exec(QString command);
    void stepIn();
    void stepOver();
    void run();
    void restart();
signals:
    void message(QString text);
};
