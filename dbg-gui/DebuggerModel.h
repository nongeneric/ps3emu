#pragma once

#include "ps3emu/Process.h"
#include "ps3emu/libs/ConcurrentBoundedQueue.h"
#include "ps3emu/EmuCallbacks.h"
#include "MonospaceGrid.h"
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/variant.hpp>
#include <QString>
#include <string>
#include <memory>
#include <vector>
#include <fstream>

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

struct TraceToCommand {
    uint32_t to;
};

using DebugCommand = boost::variant<LoadElfCommand, RunCommand, TraceToCommand>;

class GPRModel;
class DasmModel;
class MemoryDumpModel;
class Rsx;
class DebuggerModel : public QWidget, public IEmuCallbacks {
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
    std::string _moduleToWait;
    void log(std::string str);
    void traceTo(ps3_uintptr_t va);
    void spuTraceTo(FILE* f, ps3_uintptr_t va, std::map<std::string, int>& counts);
    void ppuTraceTo(FILE* f, ps3_uintptr_t va, std::map<std::string, int>& counts);
    void spuTraceTo(ps3_uintptr_t va);
    void ppuTraceTo(ps3_uintptr_t va);
    void updateUI();
    void setSoftBreak(ps3_uintptr_t va);
    void setMemoryBreakpoint(ps3_uintptr_t va, uint32_t size, bool write);
    void clearSoftBreak(ps3_uintptr_t va);
    void clearSoftBreaks();
    void setSPUSoftBreak(uint32_t elfSource, ps3_uintptr_t va);
    void clearSPUSoftBreak(ps3_uintptr_t va);
    void trySetPendingSPUBreaks();
    void switchThread(PPUThread* ppu);
    void switchThread(SPUThread* spu);
    void dumpSegments();
    void dumpImports();
    void dumpGroups();
    void dumpThreads();
    void dumpMutexes(bool lw);
    void changeThread(uint32_t index);
    std::string printSegment(uint32_t ea);
    uint32_t findLibFunc(std::string name);
    void execSingleCommand(QString command);
    void dbgLoop();
    void loadElfHandler(LoadElfCommand command);
    void runHandler(RunCommand command);
    void traceToHandler(TraceToCommand command);
    void printBacktrace();
    void dumpExecutionMap();
    void dumpSpuExecutionMaps();
    uint64_t evalExpr(std::string expr);
    bool _isRunning = false;
    bool _updateUIWhenRunning = true;
    boost::mutex _runMutex;
    boost::condition_variable _runCv;
    std::ofstream _dbgOutput;
    
    void debugStdout(const char* str, int len);
    virtual void stdout(const char* str, int len) override;
    virtual void stderr(const char* str, int len) override;
    virtual void spustdout(const char* str, int len) override;

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
    void runAndWait();
    void runto(ps3_uintptr_t va);
    void runToLR();
    void pause();
    void captureFrames();
    
signals:
    void message(QString text);
    void output(QString text);
};
