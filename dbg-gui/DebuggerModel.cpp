#include "DebuggerModel.h"
#include "../ps3emu/Rsx.h"
#include "../ps3emu/ppu_dasm.h"
#include <boost/log/trivial.hpp>
#include <QStringList>
#include "stdio.h"

class GridModelChangeTracker {
    MonospaceGridModel* _model;
    std::map<std::pair<uint64_t, int>, std::pair<QString, QString>> _map;
public:
    GridModelChangeTracker(MonospaceGridModel* model)
        : _model(model) { }
        
    void track() {
        for (auto r = _model->getMinRow(); 
             r <= _model->getMaxRow();
             r += _model->getRowStep()) {
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
};

QString printHex(void* ptr, int len) {
    QString res;
    auto bytes = reinterpret_cast<uint8_t*>(ptr);
    for (auto byte = bytes; byte != bytes + len; ++byte) { 
        res += QString("%1 ").arg(*byte, 2, 16, QChar('0'));
    }
    return res;
}

class GPRModel : public MonospaceGridModel {
    PPU* _ppu;
    GridModelChangeTracker _tracker;
    int _view = 1;
public:
    GPRModel(PPU* ppu) : _ppu(ppu), _tracker(this) {
        _tracker.track();
        _tracker.track();
    }
    
    void toggleFPR() {
        _view = (_view + 1) % 4;
        _tracker.track();
        _tracker.track();
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
    
    virtual QString getCell(uint64_t row, int col) override {
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
                auto field = (_ppu->getCR() >> (31 - fpos - 3)) & 0xf;
                return QString("%1%2%3%4")
                    .arg(field & 8 ? "<" : " ")
                    .arg(field & 4 ? ">" : " ")
                    .arg(field & 2 ? "=" : " ")
                    .arg(field & 1 ? "SO" : "");
            }
            switch (row) {
                case 0: return print(_ppu->getLR());
                case 1: return print(_ppu->getCTR());
                case 2: return print(_ppu->getXER());
                case 3: return print(_ppu->getCR());
                case 4: return printBit(_ppu->getCA());
                case 5: return printBit(_ppu->getOV());
                case 6: return printBit(_ppu->getSO());
                case 17: return print(_ppu->getNIP());
                case 19: {
                    auto pages = _ppu->allocatedPages();
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
                    return QString("%1%2 (%3)")
                        .arg(usage).arg(measure).arg(pages);
                }
                default: return "";
            }
        }
        if (col == 2) {
            switch (_view) {
                case 0: return QString("FPR%1").arg(row);
                case 1: return QString("GPR%1").arg(row);
                case 2: return QString("V%1").arg(row);
                case 3: return QString("Vf%1").arg(row);
            }
        }
        if (col == 3) {
            if (_view == 0) {
                return QString("%1").arg(_ppu->getFPRd(row));
            } else if (_view == 1) {
                return print(_ppu->getGPR(row));
            } else if (_view == 2) {
                uint8_t be[16];
                auto be64 = (big_uint64_t*)be;
                _ppu->getV(row, be);
                return QString::fromStdString(
                    ssnprintf("%016" PRIx64 "%016" PRIx64,
                              (uint64_t)be64[0], 
                              (uint64_t)be64[1]));
            } else if (_view == 3) {
                uint8_t be[16];
                _ppu->getV(row, be);
                auto ui32 = (uint32_t*)be;
                auto fs = (float*)be;
                for (int i = 0; i < 4; ++i) {
                    boost::endian::endian_reverse_inplace(ui32[i]);
                }
                return QString::fromStdString(
                    ssnprintf("%g:%g:%g:%g", fs[0], fs[1], fs[2], fs[3]));
            }
        }
        throw std::runtime_error("bad column");
    }
    
    virtual uint64_t getMinRow() override {
        return 0;
    }
    
    virtual uint64_t getMaxRow() override {
        return 31;
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
    PPU* _ppu;
    ELFLoader* _elf;
public:
    DasmModel(PPU* ppu, ELFLoader* elf) : _ppu(ppu), _elf(elf) { }
    
    virtual QString getCell(uint64_t row, int col) override {
        if (col == 0) {
            return QString("%1").arg(row, 16, 16, QChar('0'));
        }
        if (!_ppu->isAllocated(row) || col == 2)
            return "";
        uint32_t instr;
        _ppu->readMemory(row, &instr, sizeof instr);
        if (col == 1)
            return printHex(&instr, sizeof instr);
        if (col == 4) {
            auto sym = _elf->getGlobalSymbolByValue(row, -1);
            if (!sym)
                return "";
            return _elf->getString(sym->st_name);
        }
        if (col == 3) {
            std::string str;
            try {
                ppu_dasm<DasmMode::Print>(&instr, row, &str);
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
        return row == _ppu->getNIP();
    }
    
    virtual bool pointsTo(uint64_t row, uint64_t& to, bool& highlighted) {
        if (!_ppu->isAllocated(row))
            return false;
        uint32_t instr;
        _ppu->readMemory(row, &instr, sizeof instr);
        if (isAbsoluteBranch(&instr)) {
            to = getTargetAddress(&instr, row);
            highlighted = row == _ppu->getNIP() && isTaken(&instr, row, _ppu);
            return true;
        }
        return false;
    }
};

DebuggerModel::DebuggerModel() {
    _ppu.reset(new PPU());
    _rsx.reset(new Rsx(_ppu.get()));
    _ppu->setRsx(_rsx.get());
    _gprModel.reset(new GPRModel(_ppu.get()));
    _dasmModel.reset(new DasmModel(_ppu.get(), &_elf));
}

DebuggerModel::~DebuggerModel() {
    _rsx->shutdown();
}

MonospaceGridModel* DebuggerModel::getGPRModel() {
    return _gprModel.get();
}

MonospaceGridModel* DebuggerModel::getDasmModel() {
    return _dasmModel.get();
}

void DebuggerModel::loadFile(QString path, QStringList args) {
    _ppu->reset();
    auto str = path.toStdString();
    _elf.load(str);
    std::vector<std::string> argsVec;
    for (auto arg : args) {
        argsVec.push_back(arg.toStdString());
    }
    _elf.map(_ppu.get(), argsVec);
    _elf.link(_ppu.get());
    updateUI();
    _elfLoaded = true;
    emit message(QString("Loaded %1").arg(path));
}

void DebuggerModel::stepIn(bool updateUI) {
    if (!_elfLoaded)
        return;
    uint32_t instr;
    try {
        auto cia = _ppu->getNIP();
        _ppu->readMemory(cia, &instr, sizeof instr);
        _ppu->setNIP(cia + sizeof instr);
        ppu_dasm<DasmMode::Emulate>(&instr, cia, _ppu.get());
        if (updateUI) {
            this->updateUI();
        }
    } catch (ProcessFinishedException&) {
        emit message("process terminated");
    } catch (std::exception& exc) {
        auto msg = QString("error: %1 (NIP: %2)")
            .arg(exc.what()).arg(_ppu->getNIP(), 16, 16);
        emit message(msg);
        BOOST_LOG_TRIVIAL(fatal) << msg.toStdString();
        throw exc;
    }
}

void DebuggerModel::stepOver() {}

void DebuggerModel::run() {
    try {
        for (int i = 0; i < 15000; ++i) {
            uint32_t instr;
            auto cia = _ppu->getNIP();
            _ppu->readMemory(cia, &instr, sizeof instr);
            _ppu->setNIP(cia + sizeof instr);
            ppu_dasm<DasmMode::Emulate>(&instr, cia, _ppu.get());
        }
        updateUI();
    } catch (std::exception& e) {
        auto msg = QString("error: %1 (NIP: %2)")
            .arg(e.what()).arg(_ppu->getNIP(), 16, 16);
        BOOST_LOG_TRIVIAL(fatal) << msg.toStdString();
    }
}

void DebuggerModel::restart() {}

void DebuggerModel::log(std::string str) {
    emit message(QString::fromStdString(str));
}

void DebuggerModel::exec(QString command) {
    auto s = command.split(' ', QString::QString::SkipEmptyParts);
    if (s.size() < 1) {
        emit message(QString("command \"%1\" parsing error").arg(command));
    }
    auto name = s[0];
    if (s.size() > 1) {
        bool ok;
        auto va = s[1].toULongLong(&ok, 16);
        if (!ok) {
            emit message("bad argument");
        }
        try {
            if (name == "go") {
                _dasmModel->navigate(va);
                return;
            } else if (name == "runto") {
                while (_ppu->getNIP() != va)
                    stepIn(false);
                updateUI();
                return;
            } else if (name == "mem") {
                printMemory(va);
                return;
            } else if (name == "traceto") {
                traceTo(va);
                updateUI();
                return;
            }
        } catch (...) {
            emit message("command failed");
        }
    }
    emit message("unknown command");
}

void DebuggerModel::printMemory(uint64_t va) {
    auto s = QString("%1    ").arg(va, 8, 16, QChar('0'));
    uint8_t buf[16];
    _ppu->readMemory(va, buf, sizeof buf);
    for (auto i = 0u; i < sizeof buf; ++i) {
        s += QString("%1 ").arg(buf[i], 2, 16, QChar('0'));
    }
    emit message(s);
}

void DebuggerModel::runToLR() {
    try {
        auto lr = _ppu->getLR();
        while (_ppu->getNIP() != lr) {
            stepIn();
        }
    } catch (...) {
        emit message("unhandled exception");
    }
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
    while ((nip = _ppu->getNIP()) != va) {
        uint32_t instr;
        _ppu->readMemory(nip, &instr, sizeof instr);
        ppu_dasm<DasmMode::Print>(&instr, nip, &str);
        std::string name;
        ppu_dasm<DasmMode::Name>(&instr, nip, &name);
        counts[name]++;
        fprintf(f, "%08x  %s\n", nip, str.c_str());
        stepIn(false);
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
    _gprModel->update();
    _dasmModel->update();
    _dasmModel->navigate(_ppu->getNIP());
}
