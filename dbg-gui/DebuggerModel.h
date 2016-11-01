#pragma once

#include "ps3emu/Process.h"
#include "ps3emu/libs/ConcurrentBoundedQueue.h"
#include "MonospaceGrid.h"
#include <boost/thread.hpp>
#include <boost/variant.hpp>
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

struct LoadElfCommand {
    QString path;
    QStringList args;
};

struct RunCommand {
    
};

using DebugCommand = boost::variant<LoadElfCommand, RunCommand>;

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
    boost::thread _debugThread;
    ConcurrentBoundedQueue<DebugCommand> _debugThreadQueue;
    bool _elfLoaded = false;
    bool _paused = false;
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
    void dbgLoop();
    void loadElfHandler(LoadElfCommand command);
    void runHandler(RunCommand command);
    void printBacktrace();
    uint64_t evalExpr(std::string expr);
    
    template<typename... Ts>
    void messagef(const char* format, Ts... args) {
        emit message(QString::fromStdString(ssnprintf(format, args...)));
    }

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
    void pause();
    
signals:
    void message(QString text);
    void output(QString text);
};
