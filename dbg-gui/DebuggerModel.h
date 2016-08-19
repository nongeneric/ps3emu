#pragma once

#include "../ps3emu/Process.h"
#include "MonospaceGrid.h"
#include <QString>
#include <string>
#include <memory>
#include <vector>

struct SoftBreakInfo {
    ps3_uintptr_t va;
    uint32_t bytes;
};

struct SPUBreakInfo {
    uint32_t bytes;
    bool isPending;
    uint32_t elfSource;
    ps3_uintptr_t va;
};

class GPRModel;
class DasmModel;
class MemoryDumpModel;
class Rsx;
class DebuggerModel : public QWidget {
    Q_OBJECT

    PPUThread* _activeThread = nullptr;
    SPUThread* _activeSPUThread = nullptr;
    std::unique_ptr<GPRModel> _gprModel;
    std::unique_ptr<DasmModel> _dasmModel;
    std::unique_ptr<MemoryDumpModel> _memoryDumpModel;
    std::unique_ptr<Process> _proc;
    std::vector<SoftBreakInfo> _softBreaks;
    std::vector<SPUBreakInfo> _spuBreaks;
    bool _elfLoaded = false;
    void log(std::string str);
    void traceTo(ps3_uintptr_t va);
    void spuTraceTo(FILE* f, ps3_uintptr_t va, std::map<std::string, int>& counts);
    void ppuTraceTo(FILE* f, ps3_uintptr_t va, std::map<std::string, int>& counts);
    void spuTraceTo(ps3_uintptr_t va);
    void ppuTraceTo(ps3_uintptr_t va);
    void updateUI();
    void setSoftBreak(ps3_uintptr_t va);
    void clearSoftBreak(ps3_uintptr_t va);
    void clearSoftBreaks();
    void setSPUSoftBreak(uint32_t elfSource, ps3_uintptr_t va);
    void clearSPUSoftBreak(ps3_uintptr_t va);
    void trySetPendingSPUBreaks();
    void switchThread(PPUThread* ppu);
    void switchThread(SPUThread* spu);
    void dumpSegments();
    void dumpImports();
    void dumpThreads();
    void changeThread(uint32_t index);
    void printSegment(uint32_t ea);
    void execSingleCommand(QString command);

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
    void exec(QString commands);
    void stepIn();
    void stepOver();
    void run();
    void runto(ps3_uintptr_t va);
    void runToLR();
signals:
    void message(QString text);
};
