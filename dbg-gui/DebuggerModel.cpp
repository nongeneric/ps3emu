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
#include "Config.h"
#include <QStringList>
#include "stdio.h"
#include <boost/regex.hpp>
#include <boost/range/algorithm.hpp>
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
                g_state.mm->readMemory(row, buf, sizeof buf);
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
                    auto pages = g_state.mm->allocatedPages();
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
                uint8_t be[16];
                auto be64 = (big_uint64_t*)be;
                _thread->getV(row, be);
                return QString::fromStdString(ssnprintf("%016" PRIx64 "%016" PRIx64,
                                                        (uint64_t)be64[0],
                                                        (uint64_t)be64[1]));
            } else if (_view == 3) {
                uint8_t be[16];
                _thread->getV(row, be);
                auto ui32 = (uint32_t*)be;
                auto fs = (float*)be;
                for (int i = 0; i < 4; ++i) {
                    boost::endian::endian_reverse_inplace(ui32[i]);
                }
                return QString::fromStdString(
                    ssnprintf("%g:%g:%g:%g", fs[0], fs[1], fs[2], fs[3]));
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
                                                        r.fs<0>(),
                                                        r.fs<1>(),
                                                        r.fs<2>(),
                                                        r.fs<3>()));
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
public:
    DasmModel() : _thread(nullptr), _spuThread(nullptr), _elf(nullptr) { }
    
    void setThread(PPUThread* thread) {
        _thread = thread;
        _spuThread = nullptr;
        _elf = g_state.proc->elfLoader();
    }
    
    void setThread(SPUThread* thread) {
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
            g_state.mm->readMemory(row, &instr, sizeof instr);
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
                if (ppu) {
                    ppu_dasm<DasmMode::Print>(&instr, row, &str);
                } else {
                    SPUDasm<DasmMode::Print>(&instr, row, &str);
                }
                return QString::fromStdString(str);
            } catch(...) {
                return "<error>";
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
        uint32_t instr;
        g_state.mm->readMemory(row, &instr, sizeof instr);
        if (isAbsoluteBranch(&instr)) {
            to = getTargetAddress(&instr, row);
            highlighted = row == _thread->getNIP() && isTaken(&instr, row, _thread);
            return true;
        }
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
        }
    }
}

DebuggerModel::DebuggerModel() : _debugThreadQueue(10) {
    _debugThread = boost::thread([&] { dbgLoop(); });
    if (g_config.config().CaptureRsx) {
        Rsx::setOperationMode(RsxOperationMode::RunCapture);
    }
    auto callback = [&](auto str, auto len) {
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
        _activeThread->singleStepBreakpoint();
    } else if (_activeSPUThread) {
        _activeSPUThread->singleStepBreakpoint();
    }
    run();
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
            auto& last = segments[segments.size() - 2];
            emit message(QString::fromStdString(ssnprintf("module loaded: %s", last.elf->shortName())));
            if (g_config.config().StopAtNewModule) {
                cont = false;
                switchThread(ev->thread);
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
    updateUI();
}

void DebuggerModel::log(std::string str) {
    emit message(QString::fromStdString(str));
}

void DebuggerModel::exec(QString command) {
    for (auto c : command.split(';', QString::SkipEmptyParts)) {
        execSingleCommand(c);
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
    } else if (name == "threads") {
        dumpThreads();
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
            messagef("thread %s", th->getName());
            _activeThread = th;
            printBacktrace();
        }
        return;
    }
    
    auto expr = command.section(':', 1, 1);
    if (name.isEmpty() || expr.isEmpty()) {
        emit message("incorrect command format");
        return;
    }
    
    uint64_t exprVal;
    try {
        exprVal = _activeThread ? evalExpr(expr.toStdString(), _activeThread)
                : _activeSPUThread ? evalExpr(expr.toStdString(), _activeSPUThread)
                : 0;
    } catch (std::exception& e) {
        emit message(QString("expr eval failed: ") + e.what());
        return;
    }
    
    try {
        if (name == "go") {
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
        } else if (name == "sbp") {
            bool ok;
            auto elfSource = command.section(':', 2, 2).toInt(&ok, 16);
            if (!ok) {
                emit message("bad elfSource");
                return;
            }
            setSPUSoftBreak(elfSource, exprVal);
            return;
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
            printSegment(exprVal);
        }
    } catch (...) {
        emit message("command failed");
        return;
    }
}

void DebuggerModel::printSegment(uint32_t ea) {
    for (auto& segment : _proc->getSegments()) {
        if (intersects(segment.va, segment.size, ea, 0u)) {
            auto msg = ssnprintf("base: %08x, rva: %08x, name: %s",
                                 segment.va,
                                 ea - segment.va,
                                 segment.elf->shortName());
            emit message(QString::fromStdString(msg));
            return;
        }
    }
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

void DebuggerModel::dumpThreads() {
    unsigned i = 0;
    emit message("PPU Threads (id, nip)");
    for (auto th : _proc->dbgPPUThreads()) {
        auto current = th == _activeThread;
        emit message(QString::asprintf("[%d]%s %08x  %08x  %s",
                                       i,
                                       current ? "*" : " ",
                                       (uint32_t)th->getId(),
                                       (uint32_t)th->getNIP(),
                                       th->getName().c_str()));
        i++;
    }
    emit message("SPU Threads (id, nip, source, name)");
    for (auto th : _proc->dbgSPUThreads()) {
        auto current = th == _activeSPUThread;
        emit message(QString::asprintf("[%d]%s %08x  %08x  %08x  %s",
                                       i,
                                       current ? "*" : " ",
                                       (uint32_t)th->getId(),
                                       th->getNip(),
                                       th->getElfSource(),
                                       th->getName().c_str()));
        i++;
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
        std::tie(imports, count) = s.elf->imports(g_state.mm);
        
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
                            auto tocVa = g_state.mm->load<4>(stubs[j] + 4);
                            stub = g_state.mm->load<4>(tocVa);
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
        _activeSPUThread->singleStepBreakpoint();
        _proc->run();
    }
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
    fclose(scriptf);
    message(QString("trace completed and saved to ") + tracefile);
}

void DebuggerModel::ppuTraceTo(FILE* f, ps3_uintptr_t va, std::map<std::string, int>& counts) {
    ps3_uintptr_t nip;
    std::string str;
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
        for (auto i = 0u; i < 32; ++i) {
            auto v = _activeThread->getV(i);
            fprintf(f, "v%d:%08x%08x%08x%08x;", i, 
                    (uint32_t)(v >> 96),
                    (uint32_t)(v >> 64),
                    (uint32_t)(v >> 32),
                    (uint32_t)v);
        }
        fprintf(f, " #%s\n", str.c_str());
        
        fflush(f);
        _activeThread->singleStepBreakpoint();
        _proc->run();
    }
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

void DebuggerModel::traceTo(ps3_uintptr_t va) {
    if (_activeSPUThread) {
        spuTraceTo(va);
    } else {
        ppuTraceTo(va);
    }
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

void DebuggerModel::setSoftBreak(ps3_uintptr_t va) {
    if (_activeSPUThread) {
        setSPUSoftBreak(_activeSPUThread->getElfSource(), va);
        return;
    }
    auto bytes = g_state.mm->load<4>(va);
    g_state.mm->store<4>(va, 0x7fe00088);
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
    g_state.mm->store<4>(va, it->bytes);
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
        backChain = g_state.mm->load<8>(backChain);
        if (!backChain)
            break;
        auto lr = g_state.mm->load<8>(backChain + 16);
        frames.push_back({lr});
    }
    return frames;
}

void DebuggerModel::printBacktrace() {
    if (_activeThread) {
        messagef("backtrace nip = %x", _activeThread->getNIP());
        for (auto frame : walkStack(_activeThread->getGPR(1))) {
            messagef("  %x", frame.lr);
        }
    }
}
