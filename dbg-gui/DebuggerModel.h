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

    PPUThread* _activeThread;
    SPUThread* _activeSPUThread;
    std::unique_ptr<GPRModel> _gprModel;
    std::unique_ptr<DasmModel> _dasmModel;
    std::unique_ptr<MemoryDumpModel> _memoryDumpModel;
    std::unique_ptr<Process> _proc;
    std::vector<SoftBreakInfo> _softBreaks;
    std::vector<SPUBreakInfo> _spuBreaks;
    bool _elfLoaded = false;
    void log(std::string str);
    void traceTo(ps3_uintptr_t va);
    void spuTraceTo(ps3_uintptr_t va);
    void ppuTraceTo(ps3_uintptr_t va);
    void updateUI();
    void setSoftBreak(ps3_uintptr_t va);
    void clearSoftBreak(ps3_uintptr_t va);
    void setSPUSoftBreak(uint32_t elfSource, ps3_uintptr_t va);
    void clearSPUSoftBreak(ps3_uintptr_t va);
    void trySetPendingSPUBreaks();
    void switchThread(PPUThread* ppu);
    void switchThread(SPUThread* spu);

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
    void stepIn();
    void stepOver();
    void run();
    void runto(ps3_uintptr_t va);
    void runToLR();
signals:
    void message(QString text);
};
