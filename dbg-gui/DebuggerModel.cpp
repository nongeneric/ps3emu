#include "DebuggerModel.h"
#include "../disassm/ppu_dasm.h"

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
public:
    GPRModel(PPU* ppu) : _ppu(ppu) { }
    
    virtual QString getCell(uint64_t row, int col) override {
        if (col == 0) {
            return QString("GRP%1").arg(row);
        } else {
            return QString("%1").arg(_ppu->getGPR(row), 16, 16, QChar('0'));
        }
    }
    
    virtual uint64_t getMinRow() override {
        return 0;
    }
    
    virtual uint64_t getMaxRow() override {
        return 31;
    }
    
    virtual uint64_t getColumnCount() override {
        return 2;
    }
};

class DasmModel : public MonospaceGridModel {
    PPU* _ppu;
public:
    DasmModel(PPU* ppu) : _ppu(ppu) { }
    
    virtual QString getCell(uint64_t row, int col) override {
        if (col == 0) {
            return QString("%1").arg(row, 16, 16, QChar('0'));
        } else {
            if (_ppu->isAllocated(row)) {
                uint32_t instr;
                _ppu->readMemory(row, &instr, sizeof instr);
                if (col == 1)
                    return printHex(&instr, sizeof instr);
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
    
    virtual uint64_t getColumnCount() override {
        return 3;
    }
    
    virtual uint64_t getRowStep() override {
        return 4;
    }
};

DebuggerModel::DebuggerModel() {
    _ppu.reset(new PPU());
    _grpModel.reset(new GPRModel(_ppu.get()));
    _dasmModel.reset(new DasmModel(_ppu.get()));
}

DebuggerModel::~DebuggerModel() { }

MonospaceGridModel* DebuggerModel::getGRPModel() {
    return _grpModel.get();
}

MonospaceGridModel* DebuggerModel::getDasmModel() {
    return _dasmModel.get();
}

void DebuggerModel::loadFile(QString path) {
    auto str = path.toStdString();
    _elf.load(str);
    _elf.map(_ppu.get());
    _grpModel->update();
    _dasmModel->update();
    _dasmModel->navigate(_ppu->getNIP());
}
