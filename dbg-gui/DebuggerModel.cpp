#include "DebuggerModel.h"
#include "DebugExpr.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/rsx/Rsx.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/spu/SPUDasm.h"
#include "ps3emu/state.h"
#include "ps3emu/libs/spu/sysSpu.h"
#include "ps3emu/EmuCallbacks.h"
#include "ps3emu/log.h"
#include "ps3emu/libs/sync/mutex.h"
#include "ps3emu/libs/sync/lwmutex.h"
#include "ps3emu/execmap/ExecutionMapCollection.h"
#include "ps3emu/execmap/InstrDb.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/exports/splicer.h"
#include "ps3emu/spu/SPUGroupManager.h"
#include "ps3emu/BBCallMap.h"
#include "ps3tool-core/Rewriter.h"
#include "Config.h"
#include <QStringList>
#include "stdio.h"
#include <boost/regex.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/thread/locks.hpp>
#include <boost/filesystem.hpp>
#include <set>

class GridModelChangeTracker {
    MonospaceGridModel* _model;
    std::map<std::pair<uint64_t, int>, std::pair<QString, QString>> _map;
    unsigned _rows;
public:
    GridModelChangeTracker(MonospaceGridModel* model, unsigned rows = 0)
        : _model(model) {
        _rows = rows == 0 ? _model->getMaxRow() : rows;
    }
        
    void track() {
        for (auto r = _model->getMinRow(); r <= _rows; r += _model->getRowStep()) {
            for (auto c = 0; c < _model->getColumnCount(); ++c) {
                auto pair = std::make_pair(r, c);
                _map[pair].first = _map[pair].second;
                _map[pair].second = _model->getCell(r, c);
            }
        }
    }
    
    bool isHighlighted(uint64_t row, int col) {
        auto pair = std::make_pair(row, col);
        return _map[pair].first != _map[pair].second;
    }
    
    void reset() {
        _map.clear();
        track();
        track();
    }
};

QString printHex(void* ptr, int len) {
    QString res;
    auto bytes = reinterpret_cast<uint8_t*>(ptr);
    for (auto byte = bytes; byte != bytes + len; ++byte) { 
        res += QString("%1 ").arg(*byte, 2, 16, QChar('0'));
    }
    return res;
}

class MemoryDumpModel : public MonospaceGridModel {
    PPUThread* _thread;
    SPUThread* _spuThread;
public:
    MemoryDumpModel() : _thread(nullptr), _spuThread(nullptr) {
    }
    
    void setThread(PPUThread* thread) {
        _thread = thread;
        _spuThread = nullptr;
    }
    
    void setThread(SPUThread* thread) {
        _thread = nullptr;
        _spuThread = thread;
    }

    virtual QString getCell(uint64_t row, int col) override {
        bool ppu = _thread && g_state.mm->isAllocated(row);
        bool spu = _spuThread && row <= LocalStorageSize - 16;
        if (!ppu && !spu)
            return "";
        if (col == 0) {
            return QString("%1    ").arg(row, 8, 16, QChar('0'));
        } else {
            uint8_t buf[16];
            if (ppu) {
                g_state.mm->readMemory(row, buf, sizeof buf, false);
            } else if (spu) {
                memcpy(buf, _spuThread->ptr(row), 16);
            }
            if (col == 1) {
                return printHex(buf, 16);
            } else {
                QString s;
                for (auto i = 0u; i < sizeof buf; ++i) {
                    auto c = (char)buf[i];
                    if (isprint(c)) {
                        s += c;
                    } else {
                        s += ".";
                    }
                }
                return s;
            }
        }
    }
    
    virtual uint64_t getMinRow() override {
        return 0;
    }
    
    virtual uint64_t getMaxRow() override {
        return -1;
    }
    
    virtual int getColumnCount() override {
        return 3;
    }
    
    virtual void update() override {
        MonospaceGridModel::update();
    }
    
    virtual uint64_t getRowStep() override {
        return 16;
    }
    
    virtual bool isHighlighted(uint64_t row, int col) override {
        return false;
    }
};

class GPRModel : public MonospaceGridModel {
    PPUThread* _thread;
    SPUThread* _spuThread;
    GridModelChangeTracker _tracker;
    int _view = 1;
public:
    GPRModel() : _thread(nullptr), _spuThread(nullptr), _tracker(this) {
        _tracker.reset();
    }
    
    void toggleFPR() {
        _view = (_view + 1) % 4;
        _tracker.reset();
        update();
    }
    
    QString print(uint64_t value) {
        return QString("%1").arg(value, 16, 16, QChar('0'));
    }
    
    QString print(uint32_t value) {
        return QString("%1").arg(value, 8, 16, QChar('0'));
    }
    
    QString printBit(uint8_t value) {
        return QString("%1").arg(value);
    }
    
    void setThread(PPUThread* thread) {
        _thread = thread;
        _spuThread = nullptr;
    }
    
    void setThread(SPUThread* thread) {
        _thread = nullptr;
        _spuThread = thread;
    }
    
    QString getPPUCell(uint64_t row, int col) {
        if (col == 0) {
            if (8 <= row && row <= 15) {
                return QString("CR%1").arg(row - 8);
            }
            switch (row) {
                case 0: return "LR";
                case 1: return "CTR";
                case 2: return "XER";
                case 3: return "CR";
                case 4: return "CA";
                case 5: return "OV";
                case 6: return "SO";
                case 17: return "NIP";
                case 19: return "MEM";
                case 21: return "EMUR0";
                case 22: return "EMUR1";
                default: return "";
            }
        }
        if (col == 1) {
            if (8 <= row && row <= 15) {
                auto fpos = (row - 8) * 4;
                auto field = (_thread->getCR() >> (31 - fpos - 3)) & 0xf;
                return QString("%1%2%3%4")
                    .arg(field & 8 ? "<" : " ")
                    .arg(field & 4 ? ">" : " ")
                    .arg(field & 2 ? "=" : " ")
                    .arg(field & 1 ? "SO" : "");
            }
            switch (row) {
                case 0: return print(_thread->getLR());
                case 1: return print(_thread->getCTR());
                case 2: return print(_thread->getXER());
                case 3: return print(_thread->getCR());
                case 4: return printBit(_thread->getCA());
                case 5: return printBit(_thread->getOV());
                case 6: return printBit(_thread->getSO());
                case 17: return print(_thread->getNIP());
                case 19: {
                    auto pages = 0;
                    auto usage = pages * DefaultMainMemoryPageSize;
                    auto measure = "";
                    if (usage > (1 << 30)) {
                        measure = "GB";
                        usage >>= 30;
                    } else if (usage > (1 << 20)) {
                        measure = "MB";
                        usage >>= 20;
                    } else if (usage > (1 << 10)) {
                        measure = "KB";
                        usage >>= 10;
                    }
                    return QString("%1%2 (%3)").arg(usage).arg(measure).arg(pages);
                }
                case 21: return print(_thread->getEMUREG(0));
                case 22: return print(_thread->getEMUREG(1));
                default: return "";
            }
        }
        if (col == 2) {
            switch (_view) {
                case 0: return QString("F%1").arg(row);
                case 1: return QString("R%1").arg(row);
                case 2: return QString("V%1").arg(row);
                case 3: return QString("Vf%1").arg(row);
            }
        }
        if (col == 3) {
            if (_view == 0) {
                return QString("%1").arg(_thread->getFPRd(row));
            } else if (_view == 1) {
                return print(_thread->getGPR(row));
            } else if (_view == 2) {
                auto r = _thread->r(row);
                return QString::fromStdString(ssnprintf("%016" PRIx64 "%016" PRIx64,
                                                        (uint64_t)r.dw(0),
                                                        (uint64_t)r.dw(1)));
            } else if (_view == 3) {
                auto r = _thread->r(row);
                return QString::fromStdString(
                    ssnprintf("%g:%g:%g:%g", r.fs(0), r.fs(1), r.fs(2), r.fs(3)));
            }
        }
        return "";
    }
    
    QString getSPUCell(uint64_t row, int col) {
        if (col == 0) {
            switch (row) {
                case 0: return "NIP";   
            }
        }
        if (col == 1) {
            switch (row) {
                case 0: return print(_spuThread->getNip());
            }
        }
        if (col == 2) {
            return QString("R%1").arg(row);
        }
        if (col == 3) {
            auto r = _spuThread->r(row);
            if (_view == 1 || _view == 3) {
                return QString::fromStdString(ssnprintf(
                    "%08x %08x %08x %08x", r.w<0>(), r.w<1>(), r.w<2>(), r.w<3>()));
            } else {
                return QString::fromStdString(ssnprintf("%g : %g : %g : %g",
                                                        r.fs(0),
                                                        r.fs(1),
                                                        r.fs(2),
                                                        r.fs(3)));
            }
        }
        return "";
    }
    
    virtual QString getCell(uint64_t row, int col) override {
        if (_thread && row < 32)
            return getPPUCell(row, col);
        if (_spuThread)
            return getSPUCell(row, col);
        return "";
    }
    
    virtual uint64_t getMinRow() override {
        return 0;
    }
    
    virtual uint64_t getMaxRow() override {
        return 127;
    }
    
    virtual int getColumnCount() override {
        return 4;
    }
    
    virtual void update() override {
        _tracker.track();
        MonospaceGridModel::update();
    }
    
    virtual bool isHighlighted(uint64_t row, int col) override {
        return _tracker.isHighlighted(row, col);
    }
};

class DasmModel : public MonospaceGridModel {
    PPUThread* _thread;
    SPUThread* _spuThread;
    ELFLoader* _elf;
    std::vector<EmbeddedElfInfo> _spuElfs;
    
    bool isSpuElfAddress(uint32_t va) {
        for (auto& elf : _spuElfs) {
            if (elf.startOffset <= va && va < elf.startOffset + elf.size)
                return true;
        }
        return false;
    }
    
public:
    DasmModel() : _thread(nullptr), _spuThread(nullptr), _elf(nullptr) {
    }
    
    void updateSpuElfs() {
        _spuElfs.clear();
        for (auto& segment : g_state.proc->getSegments()) {
            std::vector<uint8_t> dump(segment.size);
            g_state.mm->readMemory(segment.va, &dump[0], dump.size());
            for (auto& elf : discoverEmbeddedSpuElfs(dump)) {
                elf.startOffset += segment.va;
                elf.size = 500;
                elf.header = nullptr;
                _spuElfs.push_back(elf);
            }
        }
    }
    
    void setThread(PPUThread* thread) {
        //updateSpuElfs();
        _thread = thread;
        _spuThread = nullptr;
        _elf = g_state.proc->elfLoader();
    }
    
    void setThread(SPUThread* thread) {
        //updateSpuElfs();
        _thread = nullptr;
        _spuThread = thread;
        _elf = nullptr;
    }
    
    virtual QString getCell(uint64_t row, int col) override {
        if (!_thread && !_spuThread)
            return "";
        if (col == 0) {
            return QString("%1").arg(row, 8, 16, QChar('0'));
        }
        auto ppu = _thread && g_state.mm->isAllocated(row);
        auto spu = _spuThread && row < LocalStorageSize - 4;
        if ((!ppu && !spu) || col == 2)
            return "";
        
        uint32_t instr;
        if (ppu) {
            try {
                g_state.mm->readMemory(row, &instr, sizeof instr, false);
            } catch (...) {
                return "err";
            }
        } else {
            memcpy(&instr, _spuThread->ptr(row), 4);
        }
        if (col == 1)
            return printHex(&instr, sizeof instr);
        if (col == 4) {
            if (spu)
                return "";
            auto sym = _elf->getGlobalSymbolByValue(row, -1);
            if (!sym)
                return "";
            return _elf->getString(sym->st_name);
        }
        if (col == 3) {
            std::string str;
            try {
                auto isSpuElf = isSpuElfAddress(row);
                if (ppu && !isSpuElf) {
                    ppu_dasm<DasmMode::Print>(&instr, row, &str);
                    auto bbcall = g_state.bbcallMap->get(row);
                    str = ssnprintf("%s%s", bbcall ? "." : " ", str);
                } else {
                    SPUDasm<DasmMode::Print>(&instr, row, &str);
                }
                if (ppu && isSpuElf) {
                    str = "<spu>. " + str;
                }
                return QString::fromStdString(str);
            } catch(...) {
                return "";
            }
        }
        return "";
    }
    
    virtual uint64_t getMinRow() override {
        return 0;
    }
    
    virtual uint64_t getMaxRow() override {
        return -1;
    }
    
    virtual int getColumnCount() override {
        return 5;
    }
    
    virtual uint64_t getRowStep() override {
        return 4;
    }
    
    virtual bool isHighlighted(uint64_t row, int col) override {
        if (_thread)
            return row == _thread->getNIP();
        if (_spuThread)
            return row == _spuThread->getNip();
        return false;
    }
    
    virtual bool pointsTo(uint64_t row, uint64_t& to, bool& highlighted) override {
        if (!_thread || !g_state.mm->isAllocated(row))
            return false;
        try {
            uint32_t instr = g_state.mm->load32(row);
            if (isAbsoluteBranch(instr)) {
                to = getTargetAddress(instr, row);
                highlighted = row == _thread->getNIP() && isTaken(instr, row, _thread);
                return true;
            }
        } catch (...) { }
        return false;
    }
};

void DebuggerModel::loadElfHandler(LoadElfCommand command) {
    _proc.reset(new Process());
    auto str = command.path.toStdString();
    std::vector<std::string> argsVec;
    for (auto arg : command.args) {
        argsVec.push_back(arg.toStdString());
    }
    _proc->init(str, argsVec);
    auto ev = _proc->run();
    _activeThread = boost::get<PPUThreadStartedEvent>(ev).thread;
    updateUI();
    _elfLoaded = true;
    emit message(QString("Loaded %1").arg(command.path));
}

void DebuggerModel::runAndWait() {
    _isRunning = true;
    boost::unique_lock<boost::mutex> lock(_runMutex);
    run();
    _runCv.wait(lock, [&] { return !_isRunning; });
}

void DebuggerModel::run() {
    if (_paused) {
        _proc->dbgPause(false);
        _paused = false;
        return;
    }
    _debugThreadQueue.enqueue(RunCommand{});
}

void DebuggerModel::dbgLoop() {
    for (;;) {
        auto untyped = _debugThreadQueue.dequeue();
        if (auto c = boost::get<LoadElfCommand>(&untyped)) {
            loadElfHandler(*c);
        } else if (auto c = boost::get<RunCommand>(&untyped)) {
            runHandler(*c);
        } else if (auto c = boost::get<TraceToCommand>(&untyped)) {
            traceToHandler(*c);
        }
    }
}

DebuggerModel::DebuggerModel()
    : _debugThreadQueue(1), _dbgOutput("/tmp/ps3_dbg_output.log")
{
    _debugThread = boost::thread([&] { dbgLoop(); });
    if (g_config.config().CaptureRsx) {
        Rsx::setOperationMode(RsxOperationMode::RunCapture);
    }

    auto callback = [&](auto str, auto len) {
        _dbgOutput << std::string(str, len);
        _dbgOutput.flush();
        emit this->output(QString::fromLatin1(str, len));
    };
    g_state.callbacks->stdout = callback;
    g_state.callbacks->stderr = callback;
    g_state.callbacks->spustdout = callback;
    _gprModel.reset(new GPRModel());
    _dasmModel.reset(new DasmModel());
    _memoryDumpModel.reset(new MemoryDumpModel());
}

DebuggerModel::~DebuggerModel() {}

MonospaceGridModel* DebuggerModel::getGPRModel() {
    return _gprModel.get();
}

MonospaceGridModel* DebuggerModel::getDasmModel() {
    return _dasmModel.get();
}

MonospaceGridModel* DebuggerModel::getMemoryDumpModel() {
    return _memoryDumpModel.get();
}

void DebuggerModel::loadFile(QString path, QStringList args) {
    _debugThreadQueue.enqueue(LoadElfCommand{path, args});
}

void DebuggerModel::stepIn() {
    if (_activeThread) {
        _activeThread->singleStepBreakpoint(true);
        runAndWait();
        _activeThread->singleStepBreakpoint(false);
    } else if (_activeSPUThread) {
        _activeSPUThread->singleStepBreakpoint(true);
        runAndWait();
        _activeSPUThread->singleStepBreakpoint(false);
    }
}

void DebuggerModel::stepOver() {
    exec("runto : nip + 4");
}

void DebuggerModel::switchThread(PPUThread* ppu) {
    _activeThread = ppu;
    _activeSPUThread = nullptr;
}

void DebuggerModel::switchThread(SPUThread* spu) {
    _activeThread = nullptr;
    _activeSPUThread = spu;
}

void DebuggerModel::runHandler(RunCommand command) {
    bool cont = true;
    while (cont) {
        auto untyped = _proc->run();
        if (auto ev = boost::get<PPUThreadFailureEvent>(&untyped)) {
            emit message("failure");
            cont = false;
            switchThread(ev->thread);
        } else if (auto ev = boost::get<PPUInvalidInstructionEvent>(&untyped)) {
            emit message(QString("invalid instruction at %1")
                             .arg(ev->thread->getNIP(), 8, 16, QChar('0')));
            cont = false;
            switchThread(ev->thread);
        } else if (auto ev = boost::get<MemoryAccessErrorEvent>(&untyped)) {
            emit message(QString("memory access error at %1")
                             .arg(ev->thread->getNIP(), 8, 16, QChar('0')));
            cont = false;
            switchThread(ev->thread);
        } else if (boost::get<ProcessFinishedEvent>(&untyped)) {
            emit message("process finished");
            cont = false;
        } else if (auto ev = boost::get<PPUThreadStartedEvent>(&untyped)) {
            emit message(QString::fromStdString(ssnprintf("ppu thread started %x",
                                                          ev->thread->getNIP())));
            if (g_config.config().StopAtNewPpuThread) {
                cont = false;
                switchThread(ev->thread);
            }
        } else if (boost::get<PPUThreadFinishedEvent>(&untyped)) {
            emit message("thread finished");
        } else if (auto ev = boost::get<PPUBreakpointEvent>(&untyped)) {
            emit message("breakpoint");
            switchThread(ev->thread);
            clearSoftBreak(ev->thread->getNIP());
            cont = false;
        } else if (auto ev = boost::get<PPUSingleStepBreakpointEvent>(&untyped)) {
            cont = false;
            switchThread(ev->thread);
        } else if (auto ev = boost::get<PPUModuleLoadedEvent>(&untyped)) {
            auto segments = _proc->getSegments();
            auto it = std::find_if(begin(segments), end(segments), [&](auto& s) {
                return s.va == ev->imageBase;
            });
            assert(it != end(segments));
            emit messagef("module loaded: %s", it->elf->shortName());
            auto stopAlways = g_config.config().StopAtNewModule;
            auto expectedModule = _moduleToWait == it->elf->shortName();
            if (stopAlways || expectedModule) {
                cont = false;
                switchThread(ev->thread);
                if (expectedModule) {
                    _moduleToWait = "";
                    message("the module was expected");
                }
            }
        } else if (auto ev = boost::get<SPUInvalidInstructionEvent>(&untyped)) {
            emit message(QString("invalid spu instruction at %1")
                             .arg(ev->thread->getNip(), 8, 16, QChar('0')));
            cont = false;
            switchThread(ev->thread);
        } else if (auto ev = boost::get<SPUBreakpointEvent>(&untyped)) {
            switchThread(ev->thread);
            emit message("spu breakpoint");
            clearSPUSoftBreak(ev->thread->getNip());
            cont = false;
        } else if (auto ev = boost::get<SPUSingleStepBreakpointEvent>(&untyped)) {
            cont = false;
            switchThread(ev->thread);
        } else if (auto ev = boost::get<SPUThreadStartedEvent>(&untyped)) {
            trySetPendingSPUBreaks();
            if (g_config.config().StopAtNewSpuThread) {
                cont = false;
                switchThread(ev->thread);
            }
        }
    }
    if (_updateUIWhenRunning) {
        updateUI();
    }

    boost::unique_lock<boost::mutex> _lock(_runMutex);
    _isRunning = false;
    _runCv.notify_all();
}

void DebuggerModel::log(std::string str) {
    emit message(QString::fromStdString(str));
}

void DebuggerModel::exec(QString command) {
    for (auto c : command.split(';', QString::SkipEmptyParts)) {
        execSingleCommand(c);
    }
}

uint64_t DebuggerModel::evalExpr(std::string expr) {
    try {
        return _activeThread ? ::evalExpr(expr, _activeThread)
             : _activeSPUThread ? ::evalExpr(expr, _activeSPUThread) : 0;
    } catch (std::exception& e) {
        emit message(QString("expr eval failed: ") + e.what());
        return -1;
    }
}

void DebuggerModel::execSingleCommand(QString command) {
    auto name = command.section(':', 0, 0).trimmed();
    
    if (name == "segments") {
        dumpSegments();
        return;
    } else if (name == "imports") {
        dumpImports();
        return;
    } else if (name == "groups") {
        dumpGroups();
        return;
    } else if (name == "threads") {
        dumpThreads();
        return;
    } else if (name == "mutexes") {
        dumpMutexes(true);
        dumpMutexes(false);
        return;
    } else if (name == "clearbps") {
        clearSoftBreaks();
        return;
    } else if (name == "so") {
        stepIn();
        return;
    } else if (name == "run") {
        run();
        return;
    } else if (name == "spurs") {
        dumpSpursTrace([&](auto line) { this->message(QString::fromStdString(line)); });
        return;
    } else if (name == "backtrace") {
        printBacktrace();
        return;
    } else if (name == "backtraceall") {
        for (auto th : _proc->dbgPPUThreads()) {
            _activeThread = th;
            printBacktrace();
        }
        return;
    } else if (name == "execmap-toggle") {
        g_state.executionMaps->enabled = !g_state.executionMaps->enabled;
        return;
    } else if (name == "spu-execmap") {
        dumpSpuExecutionMaps();
        return;
    } else if (name == "execmap") {
        dumpExecutionMap();
        return;
    } else if (name == "libfunc") {
        auto name = command.section(':', 1, 1).trimmed().toStdString();
        messagef("%x", findLibFunc(name));
        return;
    } else if (name == "waitmodule") {
        _moduleToWait = command.section(':', 1, 1).trimmed().toStdString();
        messagef("waiting for module %s", _moduleToWait);
        return;
    }
    
    auto expr = command.section(':', 1, 1);
    if (name.isEmpty() || expr.isEmpty()) {
        emit message("incorrect command format");
        return;
    }
    
    auto exprVal = evalExpr(expr.toStdString());
    
    try {
        if (name == "spursb") {
            auto buffer = (char*)g_state.mm->getMemoryPointer(exprVal, 1);
            dumpSpursTrace([&](auto line) { this->message(QString::fromStdString(line)); }, buffer, 1024*1024*10);
            return;
        } else if (name == "go") {
            _dasmModel->navigate(exprVal);
            return;
        } else if (name == "runto") {
            runto(exprVal);
            return;
        } else if (name == "mem") {
            _memoryDumpModel->navigate(exprVal);
            return;
        } else if (name == "traceto") {
            traceTo(exprVal);
            updateUI();
            return;
        } else if (name == "bp") {
            setSoftBreak(exprVal);
            return;
        } else if (name == "mwbp") {
            bool ok;
            auto size = command.section(':', 2, 2).toInt(&ok, 16);
            if (!ok) {
                emit message("bad size");
                return;
            }
            setMemoryBreakpoint(exprVal, size, true);
            return;
        } else if (name == "mrbp") {
            bool ok;
            auto size = command.section(':', 2, 2).toInt(&ok, 16);
            setMemoryBreakpoint(exprVal, size, false);
            if (!ok) {
                emit message("bad size");
                return;
            }
            return;
        } else if (name == "sbp") {
            bool ok;
            auto elfSource = command.section(':', 2, 2).toInt(&ok, 16);
            if (!ok) {
                emit message("bad elfSource");
                return;
            }
            setSPUSoftBreak(elfSource, exprVal);
            return;
        } else if (name == "dump") {
            auto start = exprVal;
            auto end = evalExpr(command.section(':', 2, 2).toStdString());
            std::vector<uint8_t> vec(end - start);
            if (_activeThread) {
                g_state.mm->readMemory(start, &vec[0], vec.size());
            } else if (_activeSPUThread) {
                memcpy(&vec[0], _activeSPUThread->ls(0), vec.size());
            } else {
                messagef("no active thread");
            }
            auto path = "/tmp/dump.bin";
            write_all_bytes(&vec[0], vec.size(), path);
            messagef("memory dump %x-%x saved to %s", start, end, path);
        } else if (name == "p") {
            emit message(QString(" : 0x%1").arg(exprVal, 0, 16));
            return;
        } else if (name == "put") {
            auto id = command.section(':', 2, 2).trimmed().toStdString();
            boost::regex rxgpr("r([0-9]+)");
            boost::smatch m;
            if (boost::regex_match(id, m, rxgpr)) {
                auto n = std::stoul(m[1]);
                if (n <= 31) {
                    _activeThread->setGPR(n, exprVal);
                }
            } else if (id == "nip") {
                _activeThread->setNIP(exprVal);
            } else {
                emit message("unknown id");
                return;
            }
            updateUI();
            return;
        } else if (name == "thread") {
            changeThread(exprVal);
            return;
        } else if (name == "segment") {
            messagef("%s", printSegment(exprVal));
        } else if (name == "info") {
            auto range = g_state.mm->addressRange(exprVal);
            messagef("range: %x-%x, %s, %s",
                     range.start,
                     range.start + range.len,
                     range.readonly ? "R" : "RW",
                     range.comment);
        }
    } catch (...) {
        emit message("command failed");
        return;
    }
}

uint32_t DebuggerModel::findLibFunc(std::string name) {
    auto fnid = calcFnid(name.c_str());
    for (auto& segment : _proc->getSegments()) {
        if (segment.index != 0)
            continue;
        auto[exports, nexports] = segment.elf->exports();
        for (auto i = 0; i < nexports; ++i) {
            auto fnids = (big_uint32_t*)g_state.mm->getMemoryPointer(
                exports[i].fnid_table, 4 * exports[i].functions);
            auto stubs = (big_uint32_t*)g_state.mm->getMemoryPointer(
                exports[i].stub_table, 4 * exports[i].functions);
            for (auto j = 0; j < exports[i].functions; ++j) {
                if (fnids[j] == fnid) {
                    auto codeVa = g_state.mm->load32(stubs[j]);
                    return codeVa;
                }
            }
        }
    }
    return 0;
}

std::string DebuggerModel::printSegment(uint32_t ea) {
    for (auto& segment : _proc->getSegments()) {
        if (intersects(segment.va, segment.size, ea, 1u)) {
            return ssnprintf("base: %08x, rva: %08x, name: %s",
                                 segment.va,
                                 ea - segment.va,
                                 segment.elf->shortName());
        }
    }
    return "none";
}

void DebuggerModel::changeThread(uint32_t index) {
    unsigned i = 0;
    for (auto th : _proc->dbgPPUThreads()) {
        if (i == index) {
            switchThread(th);
        }
        i++;
    }
    for (auto th : _proc->dbgSPUThreads()) {
        if (i == index) {
            switchThread(th);
        }
        i++;
    }
    updateUI();
}

PPUThread* getThreadByTid(std::vector<PPUThread*>& threads, unsigned tid) {
    for (auto& th : threads) {
        if (th->getTid() == tid)
            return th;
    }
    return nullptr;
}

void DebuggerModel::dumpMutexes(bool lw) {
    emit messagef("%smutexes (id, name, type, owner)", lw ? "lw " : "");
    auto threads = _proc->dbgPPUThreads();
    for (auto& pair : lw ? dbgDumpLwMutexes() : dbgDumpMutexes()) {
        auto owner = getThreadByTid(threads, pair.second->mutex.__data.__owner);
        emit messagef("  %x, %s, %s, %s",
                      pair.first,
                      pair.second->name,
                      pair.second->typestr(),
                      owner ? owner->getName() : "unlocked");
    }
}

void DebuggerModel::dumpGroups() {
    messagef("%s", g_state.spuGroupManager->dbgDumpGroups());
}

void DebuggerModel::dumpThreads() {
    auto i = 0u;
    emit message("PPU Threads (id, nip, state)");
    for (auto th : _proc->dbgPPUThreads()) {
        auto current = th == _activeThread;
        emit message(QString::asprintf("[%03d]%s %08x  %08x %s %s",
                                       i,
                                       current ? "*" : " ",
                                       (uint32_t)th->getId(),
                                       (uint32_t)th->getNIP(),
                                       th->dbgIsPaused() ? "PAUSED" : "RUNNING",
                                       th->getName().c_str()));
        i++;
    }
    auto printSpuThread = [&](auto i, auto th, auto prefix) {
        auto state = th->suspended() ? "SUSPENDED"
                   : th->dbgIsPaused() ? "PAUSED" 
                   : "RUNNING";
        emit messagef("%s[%03d]%s %08x  %08x  %08x  %s  %s",
                      prefix,
                      i,
                      th == _activeSPUThread ? "*" : " ",
                      (uint32_t)th->getId(),
                      th->getNip(),
                      th->getElfSource(),
                      state,
                      th->getName().c_str());
    };
    emit message("SPU Threads (id, nip, source, name) by ID");
    auto allSpuThreads = _proc->dbgSPUThreads();
    for (auto th : allSpuThreads) {
        printSpuThread(i, th, "");
        i++;
    }
    
    std::set<uint32_t> printed;
    for (auto& group : getThreadGroups()) {
        i = 0u;
        emit messagef("\nGroup (prio %d) %s", group->priority, group->name);
        for (auto& th : group->threads) {
            printSpuThread(i, th, "  ");
            printed.insert(th->getId());
            i++;
        }
    }
    
    i = 0u;
    emit messagef("No group");
    for (auto& th : allSpuThreads) {
        if (printed.find(th->getId()) != end(printed))
            continue;
        printSpuThread(i, th, "  ");
    }
}

void DebuggerModel::dumpSegments() {
    for (auto segment : _proc->getSegments()) {
        emit message(QString::asprintf(
            "%08x  %08x  %s",
            segment.va,
            segment.size,
            segment.elf->shortName().c_str()));
    }
}

void DebuggerModel::dumpImports() {
    auto segments = _proc->getSegments();
    std::set<ELFLoader*> elfs;
    for (auto s : segments) {
         if (elfs.find(s.elf.get()) != end(elfs))
            continue;
        elfs.insert(s.elf.get());
        
        emit message(ssnprintf("module %s", s.elf->shortName()).c_str());
        
        prx_import_t* imports;
        int count;
        std::tie(imports, count) = s.elf->imports();
        
        for (auto i = 0; i < count; ++i) {
            std::string name;
            readString(g_state.mm, imports[i].name, name);
            emit message(ssnprintf("  import %s", name).c_str());
            
            auto printImports = [&](auto idsVa, auto stubsVa, auto count, auto type, auto isVar) {
                auto fnids = (big_uint32_t*)g_state.mm->getMemoryPointer(idsVa, count * 4);
                auto stubs = (big_uint32_t*)g_state.mm->getMemoryPointer(stubsVa, count * 4);
                for (auto j = 0; j < count; ++j) {
                    auto segment = boost::find_if(segments, [=](auto& s) {
                        auto stub = stubs[j];
                        if (isVar) {
                            auto tocVa = g_state.mm->load32(stubs[j] + 4);
                            stub = g_state.mm->load32(tocVa);
                        }
                        return s.va <= stub && stub < s.va + s.size;
                    });
                    auto resolution = segment == end(segments) ? "NCALL" : segment->elf->shortName();
                    emit message(QString(ssnprintf("    %s fnid_%08X | %s", type, fnids[j], resolution).c_str()));
                }
            };
            printImports(imports[i].fnids, imports[i].fstubs, imports[i].functions, "FUNC    ", false);
            printImports(imports[i].vnids, imports[i].vstubs, imports[i].variables, "VARIABLE", true);
            printImports(imports[i].tls_vnids, imports[i].tls_vstubs, imports[i].tls_variables, "TLS VARIABLE", true);
        }
    }
}

void DebuggerModel::runToLR() {
    runto(_activeThread->getLR());
}

void DebuggerModel::toggleFPR() {
    _gprModel->toggleFPR();
}

void printFrequencies(FILE* f, std::map<std::string, int>& counts) {
    fprintf(f, "#\n#instruction frequencies:\n");
    std::vector<std::pair<std::string, int>> sorted;
    for (auto p : counts) {
        sorted.push_back(p);
    }
    std::sort(begin(sorted), end(sorted), [](auto a, auto b) {
        return b.second < a.second;
    });
    for (auto p : sorted) {
        fprintf(f, "#%-10s%-5d\n", p.first.c_str(), p.second);
    }
}

void DebuggerModel::spuTraceTo(FILE* f, ps3_uintptr_t va, std::map<std::string, int>& counts) {
    ps3_uintptr_t nip;
    std::string str;
    _activeSPUThread->singleStepBreakpoint(true);
    while ((nip = _activeSPUThread->getNip()) != va) {
        auto instr = _activeSPUThread->ptr(nip);
        SPUDasm<DasmMode::Print>(instr, nip, &str);
        std::string name;
        SPUDasm<DasmMode::Name>(instr, nip, &name);
        counts[name]++;
        
        fprintf(f, "pc:%08x;", nip);
        for (auto i = 0u; i < 128; ++i) {
            auto v = _activeSPUThread->r(i);
            fprintf(f, "r%03d:%08x%08x%08x%08x;", i, 
                    v.w<0>(),
                    v.w<1>(),
                    v.w<2>(),
                    v.w<3>());
        }
        fprintf(f, " #%s\n", str.c_str());
        
        fflush(f);
        for (;;) {
            auto ev = _proc->run();
            if (boost::get<SPUSingleStepBreakpointEvent>(&ev))
                break;
        }
    }
    _activeSPUThread->singleStepBreakpoint(false);
}

void DebuggerModel::spuTraceTo(ps3_uintptr_t va) {
    auto tracefile = "/tmp/ps3trace-spu";
    auto traceScript = "/tmp/ps3trace-spu-script";
    auto f = fopen(tracefile, "w");
    std::map<std::string, int> counts;
    
    auto scriptf = fopen(traceScript, "r");
    if (va == 0 && scriptf) {
        char command;
        while (fscanf(scriptf, "%c %X\n", &command, &va) == 2) {
            if (command == 's') {
                printf("skipping to %x\n", va);
                runto(va);
            } else {
                printf("tracing to %x\n", va);
                spuTraceTo(f, va, counts);
            }
        }
    } else {
        spuTraceTo(f, va, counts);
    }
    
    printFrequencies(f, counts);
    
    fclose(f);
    if (scriptf) {
        fclose(scriptf);
    }
    message(QString("trace completed and saved to ") + tracefile);
}

void DebuggerModel::ppuTraceTo(FILE* f, ps3_uintptr_t va, std::map<std::string, int>& counts) {
    ps3_uintptr_t nip;
    std::string str;
    _updateUIWhenRunning = false;
    _activeThread->singleStepBreakpoint(true);
    while ((nip = _activeThread->getNIP()) != va) {
        uint32_t instr;
        g_state.mm->readMemory(nip, &instr, sizeof instr);
        ppu_dasm<DasmMode::Print>(&instr, nip, &str);
        std::string name;
        ppu_dasm<DasmMode::Name>(&instr, nip, &name);
        counts[name]++;
        
        fprintf(f, "pc:%08x;", nip);
        for (auto i = 0u; i < 32; ++i) {
            auto r = _activeThread->getGPR(i);
            fprintf(f, "r%d:%08x%08x;", i, (uint32_t)(r >> 32), (uint32_t)r);
        }
        fprintf(f, "r%d:%08x%08x;", 32, 0, (uint32_t)_activeThread->getLR());
        for (auto i = 0u; i < 32; ++i) {
            auto r = _activeThread->r(i);
            fprintf(f, "v%d:%08x%08x%08x%08x;", i, 
                    (uint32_t)r.w(0),
                    (uint32_t)r.w(1),
                    (uint32_t)r.w(2),
                    (uint32_t)r.w(3));
        }
        fprintf(f, " #%s\n", str.c_str());
        fflush(f);
        
        for (;;) {
            auto ev = _proc->run();
            if (boost::get<PPUSingleStepBreakpointEvent>(&ev))
                break;
        }
    }
    _updateUIWhenRunning = true;
    _activeThread->singleStepBreakpoint(false);
    updateUI();
    printFrequencies(f, counts);
}

void DebuggerModel::ppuTraceTo(ps3_uintptr_t va) {
    auto tracefile = "/tmp/ps3trace";
    auto f = fopen(tracefile, "w");
    auto traceScript = "/tmp/ps3trace-ppu-script";
    std::map<std::string, int> counts;
    
    auto scriptf = fopen(traceScript, "r");
    if (va == 0 && scriptf) {
        char command;
        while (fscanf(scriptf, "%c %X\n", &command, &va) == 2) {
            if (command == 's') {
                printf("skipping to %x\n", va);
                runto(va);
            } else {
                printf("tracing to %x\n", va);
                ppuTraceTo(f, va, counts);
            }
        }
    } else {
        ppuTraceTo(f, va, counts);
    }
    
    fclose(f);
    if (scriptf)
        fclose(scriptf);
    message(QString("trace completed and saved to ") + tracefile);
}

void DebuggerModel::traceToHandler(TraceToCommand command) {
    if (_activeSPUThread) {
        spuTraceTo(command.to);
    } else {
        ppuTraceTo(command.to);
    }
}

void DebuggerModel::traceTo(ps3_uintptr_t va) {
    _debugThreadQueue.enqueue(TraceToCommand{va});
}

void DebuggerModel::updateUI() {
    if (_activeThread) {
        _gprModel->setThread(_activeThread);
        _dasmModel->setThread(_activeThread);
        _dasmModel->navigate(_activeThread->getNIP());
        _memoryDumpModel->setThread(_activeThread);
    } else {
        _gprModel->setThread(_activeSPUThread);
        _memoryDumpModel->setThread(_activeSPUThread);
        _dasmModel->setThread(_activeSPUThread);
        _dasmModel->navigate(_activeSPUThread->getNip());
    }
    _gprModel->update();
    _dasmModel->update();
    _memoryDumpModel->update();
}

void DebuggerModel::runto(ps3_uintptr_t va) {
    setSoftBreak(va);
    run();
}

void DebuggerModel::setMemoryBreakpoint(ps3_uintptr_t va, uint32_t size, bool write) {
    g_state.mm->dbgMemoryBreakpoint(va, size, write);
}

void DebuggerModel::setSoftBreak(ps3_uintptr_t va) {
    if (_activeSPUThread) {
        setSPUSoftBreak(_activeSPUThread->getElfSource(), va);
        return;
    }
    auto bytes = g_state.mm->load32(va);
    g_state.mm->store32(va, 0x7fe00088);
    _softBreaks.push_back({va, bytes});
}

void DebuggerModel::clearSoftBreaks() {
    auto breaks = _softBreaks;
    for (auto& bp : breaks) {
        clearSoftBreak(bp.va);
    }
}

void DebuggerModel::clearSoftBreak(ps3_uintptr_t va) {
    auto it = std::find_if(
        begin(_softBreaks), end(_softBreaks), [=](auto b) { return b.va == va; });
    if (it == end(_softBreaks)) {
        emit message(
            QString::fromStdString(ssnprintf("there is no breakpoint at %x", va)));
        return;
    }
    g_state.mm->store32(va, it->bytes);
    _softBreaks.erase(it);
}

void DebuggerModel::setSPUSoftBreak(uint32_t elfSource, ps3_uintptr_t va) {
    _spuBreaks.push_back({0, true, elfSource, va});
    trySetPendingSPUBreaks();
}

void DebuggerModel::trySetPendingSPUBreaks() {
    for (auto& bp : _spuBreaks) {
        if (!bp.isPending)
            continue;
        for (auto th : _proc->dbgSPUThreads()) {
            if (th->getElfSource() == bp.elfSource) {
                emit message("setting pending spu breakpoint");
                auto ptr = (big_uint32_t*)th->ptr(bp.va);
                bp.bytes = *ptr;
                bp.isPending = false;
                *ptr = 0x7b000000;
            }
        }
    }
}

void DebuggerModel::clearSPUSoftBreak(ps3_uintptr_t va) {
    auto it = std::find_if(
        begin(_spuBreaks), end(_spuBreaks), [=](auto b) { return b.va == va; });
    if (it == end(_spuBreaks)) {
        emit message(
            QString::fromStdString(ssnprintf("there is no spu breakpoint at %x", va)));
        _activeSPUThread->setNip(_activeSPUThread->getNip() + 4);
        return;
    }
    if (!_activeSPUThread) {
        emit message("can't clear spu breakpoint without active spu thread");
        return;
    }
    if (!it->isPending) {
        auto ptr = (big_uint32_t*)_activeSPUThread->ptr(va);
        *ptr = it->bytes;
    }
    _spuBreaks.erase(it);
}

void DebuggerModel::pause() {
    _paused = true;
    _proc->dbgPause(true);
}

struct StackFrame {
    uint64_t lr;
};

std::vector<StackFrame> walkStack(uint64_t backChain) {
    std::vector<StackFrame> frames;   
    for (;;) {
        backChain = g_state.mm->load64(backChain);
        if (!backChain)
            break;
        auto lr = g_state.mm->load64(backChain + 16);
        frames.push_back({lr});
    }
    return frames;
}

void DebuggerModel::printBacktrace() {
    if (_activeThread) {
        messagef("backtrace thread %s", _activeThread->getName());
        auto print = [&](auto ip) { this->messagef("  %x\t %s", ip, printSegment(ip)); };
        print(_activeThread->getLR());
        for (auto frame : walkStack(_activeThread->getGPR(1))) {
            print(frame.lr);
        }
    }
}

void DebuggerModel::dumpExecutionMap() {
    InstrDb db;
    db.open();
    bool isPrimary = true;
    for (auto& s : _proc->getSegments()) {
        if (s.index)
            continue;
        auto entries = g_state.executionMaps->ppu.dump(s.va, s.size);
        if (!isPrimary) {
            for (auto& va : entries) {
                va -= s.va;
            }
        }
        
        auto entry = db.findPpuEntry(s.elf->elfName());
        if (entry) {
            std::copy(begin(entries), end(entries), std::back_inserter(entry->leads));
            db.deleteEntry(entry->id);
            db.insertEntry(*entry);
        } else {
            InstrDbEntry newEntry;
            newEntry.elfPath = s.elf->elfName();
            newEntry.leads = entries;
            newEntry.isPPU = true;
            db.insertEntry(newEntry);
        }
        
        messagef("dumping %d entries into %s", entries.size(), s.elf->shortName());
        isPrimary = false;
    }
}

void DebuggerModel::dumpSpuExecutionMaps() {
    InstrDb db;
    db.open("/tmp/instr.db");
    for (auto& [entry, map] : g_state.executionMaps->spu) {
        db.deleteEntry(entry.id);
        auto leads = map.dump(0, LocalStorageSize);
        std::copy(begin(leads), end(leads), std::back_inserter(entry.leads));
        db.insertEntry(entry);
        messagef("adding %d new leads to %s",
                 leads.size(),
                 entry.elfPath);
    }
    messagef("spu execmap is broken, the results lead to crash, entries are added to the unused db at /tmp/instr.db");
}

void DebuggerModel::captureFrames() {
    if (g_state.rsx) {
        messagef("capturing frames");
        g_state.rsx->captureFrames();
    }
}
