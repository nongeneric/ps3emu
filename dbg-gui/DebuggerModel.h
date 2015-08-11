#pragma once

#include "../ps3emu/PPU.h"
#include "../ps3emu/ELFLoader.h"
#include "MonospaceGrid.h"
#include <QString>
#include <memory>

class GPRModel;
class DasmModel;
class DebuggerModel {
    ELFLoader _elf;
    std::unique_ptr<GPRModel> _grpModel;
    std::unique_ptr<DasmModel> _dasmModel;
    std::unique_ptr<PPU> _ppu;
public:
    DebuggerModel();
    ~DebuggerModel();
    MonospaceGridModel* getDasmModel();
    MonospaceGridModel* getGRPModel();
    MonospaceGridModel* getMemoryDumpModel();
    MonospaceGridModel* getStackModel();
    MonospaceGridModel* getOtherAuxRegistersModel();
    MonospaceGridModel* getLogModel();
    void loadFile(QString path);
    void exec(QString command);
    void stepIn();
    void stepOver();
    void run();
};
