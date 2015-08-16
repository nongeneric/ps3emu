#include "DebuggerModel.h"
#include "../ps3emu/ppu_dasm.h"
#include <QStringList>

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
public:
    GPRModel(PPU* ppu) : _ppu(ppu), _tracker(this) { 
        _tracker.track();
        _tracker.track();
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
            switch (row) {
                case 0: return "LR";
                case 1: return "CTR";
                case 2: return "XER";
                case 3: return "CR";
                case 4: return "CA";
                case 5: return "OV";
                case 6: return "SO";
                case 7: return "Sign";
                case 8: return "NIP";
                case 10: return "MEM";
                default: return "";
            }
        }
        if (col == 1) {
            switch (row) {
                case 0: return print(_ppu->getLR());
                case 1: return print(_ppu->getCTR());
                case 2: return print(_ppu->getXER());
                case 3: return print(_ppu->getCR());
                case 4: return printBit(_ppu->getCA());
                case 5: return printBit(_ppu->getOV());
                case 6: return printBit(_ppu->getSO());
                case 7: {
                    auto sign = _ppu->getCR0_sign();
                    auto symbol = sign == 4 ? "<" : sign == 2 ? ">" : "=";
                    return QString("%1 (%2)").arg(sign).arg(symbol);
                }
                case 8: return print(_ppu->getNIP());
                case 10: {
                    auto pages = _ppu->allocatedPages();
                    auto usage = pages * MemoryPage::pageSize;
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
            return QString("GRP%1").arg(row);
        }
        if (col == 3) {
            return print(_ppu->getGPR(row));
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
        } else {
            if (_ppu->isAllocated(row)) {
                uint32_t instr;
                _ppu->readMemory(row, &instr, sizeof instr);
                if (col == 1)
                    return printHex(&instr, sizeof instr);
                if (col == 3) {
                    auto sym = _elf->getGlobalSymbolByValue(row, -1);
                    if (!sym)
                        return "";
                    return _elf->getString(sym->st_name);
                }
                std::string str;
                try {
                    ppu_dasm<DasmMode::Print>(&instr, row, &str);
                    return QString::fromStdString(str);
                } catch(...) {
                    return "<error>";
                }
            } else {
                return QString();
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
        return 4;
    }
    
    virtual uint64_t getRowStep() override {
        return 4;
    }
    
    virtual bool isHighlighted(uint64_t row, int col) override {
        return row == _ppu->getNIP();
    }
};

DebuggerModel::DebuggerModel() {
    _ppu.reset(new PPU());
    _grpModel.reset(new GPRModel(_ppu.get()));
    _dasmModel.reset(new DasmModel(_ppu.get(), &_elf));
}

DebuggerModel::~DebuggerModel() { }

MonospaceGridModel* DebuggerModel::getGRPModel() {
    return _grpModel.get();
}

MonospaceGridModel* DebuggerModel::getDasmModel() {
    return _dasmModel.get();
}

void DebuggerModel::loadFile(QString path) {
    emit message(QString("Loading %1").arg(path));
    auto str = path.toStdString();
    _elf.load(str);
    auto stdStringLog = [=](std::string s){ log(s); };
    _elf.map(_ppu.get(), stdStringLog);
    _elf.link(_ppu.get(), stdStringLog);
    _grpModel->update();
    _dasmModel->update();
    _dasmModel->navigate(_ppu->getNIP());
    _elfLoaded = true;
    emit message(QString("Loaded %1").arg(path));
}

void DebuggerModel::stepIn() {
    if (!_elfLoaded)
        return;
    uint32_t instr;
    try {
        auto cia = _ppu->getNIP();
        _ppu->readMemory(cia, &instr, sizeof instr);
        _ppu->setNIP(cia + sizeof instr);
        ppu_dasm<DasmMode::Emulate>(&instr, cia, _ppu.get());
        _grpModel->update();
        _dasmModel->update();
        _dasmModel->navigate(_ppu->getNIP());
    } catch (std::exception& exc) {
        emit message(QString("error: %1").arg(exc.what()));
    }
}

void DebuggerModel::stepOver() {}

void DebuggerModel::run() {
    for (int i = 0; i < 500; ++i) {
        stepIn();
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
                    stepIn();
                return;
            }
        } catch (...) {
            emit message("command failed");
        }
    }
    throw std::runtime_error("unknown command");
}