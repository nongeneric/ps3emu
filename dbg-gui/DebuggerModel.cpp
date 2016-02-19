#include "DebuggerModel.h"
#include "DebugExpr.h"
#include "../ps3emu/MainMemory.h"
#include "../ps3emu/ELFLoader.h"
#include "../ps3emu/rsx/Rsx.h"
#include "../ps3emu/ppu/ppu_dasm.h"
#include "../ps3emu/spu/SPUDasm.h"
#include <QStringList>
#include "stdio.h"
#include <boost/regex.hpp>

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
    GridModelChangeTracker _tracker;
public:
    MemoryDumpModel() : _thread(nullptr), _spuThread(nullptr), _tracker(this, 16) {
        _tracker.reset();
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
        bool ppu = _thread && _thread->mm()->isAllocated(row);
        bool spu = _spuThread && row <= LocalStorageSize - 16;
        if (!ppu && !spu)
            return "";
        if (col == 0) {
            return QString("%1    ").arg(row, 8, 16, QChar('0'));
        } else {
            uint8_t buf[16];
            if (ppu) {
                _thread->mm()->readMemory(row, buf, sizeof buf);
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
        _tracker.track();
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
                    auto pages = _thread->mm()->allocatedPages();
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
        _elf = thread->proc()->elfLoader();
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
        auto ppu = _thread && _thread->mm()->isAllocated(row);
        auto spu = _spuThread && row < LocalStorageSize - 4;
        if ((!ppu && !spu) || col == 2)
            return "";
        
        uint32_t instr;
        if (ppu) {
            _thread->mm()->readMemory(row, &instr, sizeof instr);
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
        if (!_thread || !_thread->mm()->isAllocated(row))
            return false;
        uint32_t instr;
        _thread->mm()->readMemory(row, &instr, sizeof instr);
        if (isAbsoluteBranch(&instr)) {
            to = getTargetAddress(&instr, row);
            highlighted = row == _thread->getNIP() && isTaken(&instr, row, _thread);
            return true;
        }
        return false;
    }
};

DebuggerModel::DebuggerModel() {
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
    _proc.reset(new Process());
    auto str = path.toStdString();
    std::vector<std::string> argsVec;
    for (auto arg : args) {
        argsVec.push_back(arg.toStdString());
    }
    _proc->init(str, argsVec);
    auto ev = _proc->run();
    _activeThread = boost::get<PPUThreadStartedEvent>(ev).thread;
    updateUI();
    _elfLoaded = true;
    emit message(QString("Loaded %1").arg(path));
}

void DebuggerModel::stepIn() {
    if (_activeThread) {
        _activeThread->singleStepBreakpoint();
    } else if (_activeSPUThread) {
        _activeSPUThread->singleStepBreakpoint();
    }
    run();
}

void DebuggerModel::stepOver() {}

void DebuggerModel::run() {
    bool cont = true;
    while (cont) {
        auto untyped = _proc->run();
        if (boost::get<PPUThreadFailureEvent>(&untyped)) {
            emit message("failure");
            cont = false;
        } else if (auto ev = boost::get<PPUInvalidInstructionEvent>(&untyped)) {
            emit message(QString("invalid instruction at %1")
                             .arg(ev->thread->getNIP(), 8, 16, QChar('0')));
            cont = false;
        } else if (auto ev = boost::get<MemoryAccessErrorEvent>(&untyped)) {
            emit message(QString("memory access error at %1")
                             .arg(ev->thread->getNIP(), 8, 16, QChar('0')));
            cont = false;
        } else if (boost::get<ProcessFinishedEvent>(&untyped)) {
            emit message("process finished");
            cont = false;
        } else if (boost::get<PPUThreadStartedEvent>(&untyped)) {
            emit message("thread created");
        } else if (boost::get<PPUThreadFinishedEvent>(&untyped)) {
            emit message("thread finished");
        } else if (auto ev = boost::get<PPUBreakpointEvent>(&untyped)) {
            emit message("breakpoint");
            _activeThread = ev->thread;
            _activeSPUThread = nullptr;
            clearSoftBreak(ev->thread->getNIP());
            cont = false;
        } else if (boost::get<PPUSingleStepBreakpointEvent>(&untyped)) {
            cont = false;
        } else if (auto ev = boost::get<SPUInvalidInstructionEvent>(&untyped)) {
            emit message(QString("invalid spu instruction at %1")
                             .arg(ev->thread->getNip(), 8, 16, QChar('0')));
            cont = false;
        } else if (auto ev = boost::get<SPUBreakpointEvent>(&untyped)) {
            _activeThread = nullptr;
            _activeSPUThread = ev->thread;
            emit message("spu breakpoint");
            clearSPUSoftBreak(ev->thread->getNip());
            cont = false;
        } else if (boost::get<SPUSingleStepBreakpointEvent>(&untyped)) {
            cont = false;
        } else if (auto ev = boost::get<SPUThreadStartedEvent>(&untyped)) {
            trySetPendingSPUBreaks();
            cont = false;
            _activeThread = nullptr;
            _activeSPUThread = ev->thread;
        }
    }
    updateUI();
}

void DebuggerModel::log(std::string str) {
    emit message(QString::fromStdString(str));
}

void DebuggerModel::exec(QString command) {
    auto name = command.section(':', 0, 0).trimmed();
    auto expr = command.section(':', 1, 1);
    if (name.isEmpty() || expr.isEmpty()) {
        emit message("incorrect command format");
        return;
    }
    
    uint64_t exprVal;
    try {
        exprVal = _activeThread
                      ? evalExpr(expr.toStdString(), _activeThread)
                      : _activeSPUThread
                            ? evalExpr(expr.toStdString(), _activeSPUThread)
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
            message(QString(" : #%1").arg(exprVal, 0, 16));
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
        }
    } catch (...) {
        emit message("command failed");
        return;
    }
}

void DebuggerModel::runToLR() {
    runto(_activeThread->getLR());
}

void DebuggerModel::toggleFPR() {
    _gprModel->toggleFPR();
}

void DebuggerModel::traceTo(ps3_uintptr_t va) {
    auto tracefile = "/tmp/ps3trace";
    auto f = fopen(tracefile, "w");
    ps3_uintptr_t nip;
    std::string str;
    std::map<std::string, int> counts;
    while ((nip = _activeThread->getNIP()) != va) {
        uint32_t instr;
        _proc->mm()->readMemory(nip, &instr, sizeof instr);
        ppu_dasm<DasmMode::Print>(&instr, nip, &str);
        std::string name;
        ppu_dasm<DasmMode::Name>(&instr, nip, &name);
        counts[name]++;
        fprintf(f, "%08x  %s\n", nip, str.c_str());
        _activeThread->singleStepBreakpoint();
        _proc->run();
    }
    fprintf(f, "\ninstruction frequencies:\n");
    std::vector<std::pair<std::string, int>> sorted;
    for (auto p : counts) {
        sorted.push_back(p);
    }
    std::sort(begin(sorted), end(sorted), [](auto a, auto b) {
        return b.second < a.second;
    });
    for (auto p : sorted) {
        fprintf(f, "%-10s%-5d\n", p.first.c_str(), p.second);
    }
    fclose(f);
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
}

void DebuggerModel::runto(ps3_uintptr_t va) {
    setSoftBreak(va);
    run();
    updateUI();
}

void DebuggerModel::setSoftBreak(ps3_uintptr_t va) {
    auto bytes = _proc->mm()->load<4>(va);
    _proc->mm()->store<4>(va, 0x7fe00088);
    _softBreaks.push_back({va, bytes});
}

void DebuggerModel::clearSoftBreak(ps3_uintptr_t va) {
    auto it = std::find_if(
        begin(_softBreaks), end(_softBreaks), [=](auto b) { return b.va == va; });
    if (it == end(_softBreaks)) {
        emit message(
            QString::fromStdString(ssnprintf("there is no breakpoint at %x", va)));
        return;
    }
    _proc->mm()->store<4>(va, it->bytes);
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
