#include "ps3tool.h"

#include "ps3emu/int.h"
#include "ps3emu/spu/R128.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/utils.h"
#include "ps3emu/spu/SPUDasm.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include <QApplication>
#include <QWidget>
#include <QAbstractItemModel>
#include <QTableView>
#include <QMainWindow>
#include <QGridLayout>
#include <QFont>
#include <vector>
#include <map>
#include <string>
#include <optional>
#include <iostream>
#include <fstream>
#include <tuple>
#include <boost/tokenizer.hpp>

struct State {
    std::map<std::string, uint64_t> r64;
    std::map<std::string, R128> r128;
};

struct Change {
    uint32_t offset;
    State effect;
    std::optional<State> complete;
};

class Parser {
    bool _spu = false;
    
    uint32_t parseOffset(auto& it) {
        return std::stoi(*it++, nullptr, 16);
    }
    
    char parseChar(auto& it) {
         return (*it++)[0];
    }
    
    R128 toR128(std::string str) {
        str = std::string(34 - str.size(), '0') + str.substr(2);
        R128 r;
        r.set_dw(1, std::strtoll(str.c_str() + 16, nullptr, 16));
        str[16] = 0;
        r.set_dw(0, std::strtoll(str.c_str(), nullptr, 16));
        return r;
    }
    
    State parseState(auto& it) {
        auto c = parseChar(it);
        assert(c == '[');
        c = parseChar(it);
        State state;
        while (c != ']') {
            if (c == ',') {
                c = parseChar(it);
            }
            assert(c == '(');
            auto key = *it++;
            auto val = *it++;
            c = parseChar(it);
            assert(c == ')');
            c = parseChar(it);
            if (key[0] == 'r') {
                if (_spu) {
                    state.r128[key] = toR128(val);
                } else {
                    state.r64[key] = std::stoll(key, nullptr, 16);
                }
            }
        }
        return state;
    }
     
    Change parseChange(auto& it) {
        auto offset = parseOffset(it);
        auto state = parseState(it);
        return {offset, state};
    }
    
public:
    Parser(bool spu) : _spu(spu) { }
    
    std::vector<Change> parse(std::string const& text) {
        boost::char_delimiters_separator<char> sep(true, "[]()", " ,\n'");
        boost::tokenizer<boost::char_delimiters_separator<char>> tok(text, sep);
        std::vector<Change> changes;
        for (auto it = begin(tok); it != end(tok);) {
            changes.push_back(parseChange(it));
        }
        return changes;
    }
};

void mergeState(State& state, State const& effect) {
    for (auto& [k, v] : effect.r64) {
        state.r64[k] = v;
    }
    for (auto& [k, v] : effect.r128) {
        state.r128[k] = v;
    }
}

void makeKeyChanges(std::vector<Change>& changes) {
    State state;
    for (auto i = 0u; i < changes.size(); i++) {
        mergeState(state, changes[i].effect);
        if (i % 50 == 0) {
            changes[i].complete = state;
        }
    }
}

State getState(std::vector<Change> const& changes, int n) {
    State state;
    int start = n;
    for (int i = n - 1; i >= 0; --i) {
        if (changes[i].complete) {
            state = *changes[i].complete;
            start = i;
            break;
        }
    }
    for (int i = start; i <= n; ++i) {
        mergeState(state, changes[i].effect);
    }
    return state;
}

void synchronize(std::vector<Change>& left, std::vector<Change>& right) {
    auto min = std::min(left.size(), right.size());
    left.resize(min);
    right.resize(min);
}

std::string printState(State const& state) {
    std::string res;
    for (auto& [k, v] : state.r64) {
        res += ssnprintf("%s=%llx ", k, v);
    }
    for (auto& [k, v] : state.r128) {
        res += ssnprintf("%s=%08x %08x %08x %08x ", k, v.w(0), v.w(1), v.w(2), v.w(3));
    }
    return res;
}

class ListModel : public QAbstractItemModel {
    std::vector<Change> const& _left;
    std::vector<Change> const& _right;
    std::vector<uint8_t> const& _dump;
    bool _spu;
    
public:
    ListModel(std::vector<Change> const& left,
              std::vector<Change> const& right,
              std::vector<uint8_t> const& dump,
              bool spu)
        : _left(left), _right(right), _dump(dump), _spu(spu) {
        assert(left.size() == right.size());
    }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 5;
    }
    
    QModelIndex index(int row,
                      int column,
                      const QModelIndex& parent = QModelIndex()) const override {
        return createIndex(row, column);
    }
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return _left.size();
    }
    
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return QVariant();
        switch (section) {
            case 0: return "Offset";
            case 1: return "Bytecode";
            case 2: return "Instruction";
            case 3: return "Left";
            case 4: return "Right";
        }
        return {};
    }
    
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (role == Qt::BackgroundColorRole) {
             if (printState(_left[index.row()].effect) !=
                 printState(_right[index.row()].effect)) {
                 return QColor(Qt::yellow);
             }
             return {};
        }
        if (role != Qt::DisplayRole)
            return {};
        switch (index.column()) {
            case 0:
                return QString::fromStdString(ssnprintf("%x", _left[index.row()].offset));
            case 1:
            case 2: {
                try {
                    auto offset = _left[index.row()].offset;
                    if (offset + 4 < _dump.size()) {
                        if (index.column() == 1) {
                            return QString::fromStdString(print_hex(&_dump[offset], 4));
                        } else {
                            auto instr = *(uint32_t*)&_dump[offset];
                            std::string str;
                            if (_spu) {
                                SPUDasm<DasmMode::Print>(&instr, offset, &str);
                            } else {
                                ppu_dasm<DasmMode::Print>(&instr, offset, &str);
                            }
                            return QString::fromStdString(str);
                        }
                    }
                } catch (...) {
                    return "error";
                }
                return {};
            }
            case 3: return QString::fromStdString(printState(_left[index.row()].effect));
            case 4: return QString::fromStdString(printState(_right[index.row()].effect));
        }
        return {};
    }
    
    QModelIndex parent(const QModelIndex& child) const override {
        return {};
    }
};

std::string printR128(R128 const& r, bool isFloat) {
    if (isFloat) {
        return ssnprintf("%g %g %g %g", r.fs(0), r.fs(1), r.fs(2), r.fs(3));
    }
    return ssnprintf("%08x %08x %08x %08x", r.w(0), r.w(1), r.w(2), r.w(3));
}

class StateModel : public QAbstractItemModel {
    std::vector<std::tuple<std::string, R128>> _r128;
    std::vector<std::tuple<std::string, int64_t>> _r64;
    
public:
    StateModel(State const& state) {
        for (auto& [k, v] : state.r128) {
            _r128.push_back({k, v});
        }
        for (auto& [k, v] : state.r64) {
            _r64.push_back({k, v});
        }
        auto pred = [](auto& a, auto& b) {
            auto anum = std::stoi(std::get<0>(a).substr(1));
            auto bnum = std::stoi(std::get<0>(b).substr(1));
            return anum < bnum;
        };
        std::sort(begin(_r128), end(_r128), pred);
        std::sort(begin(_r64), end(_r64), pred);
    }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 3;
    }
    
    QModelIndex index(int row,
                      int column,
                      const QModelIndex& parent = QModelIndex()) const override {
        return createIndex(row, column);
    }
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return _r128.size() + _r64.size();
    }
    
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return QVariant();
        switch (section) {
            case 0: return "Register";
            case 1: return "Hex";
            case 2: return "Float";
        }
        return {};
    }
    
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole)
            return {};
        if ((uint32_t)index.row() < _r64.size()) {
            auto& [name, r] = _r64[index.row()];
            switch (index.column()) {
                case 0: return QString::fromStdString(name);
                case 1: return QString::fromStdString(ssnprintf("%016llx", r, false));
                case 2: return QString::fromStdString(ssnprintf("%016llx", r, true));
            }
        } else {
            auto& [name, r] = _r128[index.row() - _r64.size()];
            switch (index.column()) {
                case 0: return QString::fromStdString(name);
                case 1: return QString::fromStdString(printR128(r, false));
                case 2: return QString::fromStdString(printR128(r, true));
            }            
        }
        return {};
    }
    
    QModelIndex parent(const QModelIndex& child) const override {
        return {};
    }
};

void HandleTraceViz(TraceVizCommand const& command) {
    int argc = 0;
    char *argv[] = {};
    QApplication app(argc, argv);
    
    std::vector<uint8_t> dump;
    if (!command.dump.empty()) {
        dump = read_all_bytes(command.dump);
    }
    auto good = Parser(command.spu).parse(read_all_text(command.good));
    auto bad = Parser(command.spu).parse(read_all_text(command.bad));
    makeKeyChanges(good);
    makeKeyChanges(bad);
    synchronize(good, bad);
    
    auto layout = new QGridLayout();
    auto window = new QMainWindow();
    window->resize(1280, 720);
    window->setWindowTitle("trace viz");
    auto centralWidget = new QWidget();
    window->setCentralWidget(centralWidget);
    centralWidget->setLayout(layout);
    
    auto goodStateView = new QTableView();
    auto badStateView = new QTableView();
    auto list = new QTableView();
    goodStateView->setFont(QFont("monospace", 10));
    badStateView->setFont(QFont("monospace", 10));
    list->setFont(QFont("monospace", 10));
    
    auto model = new ListModel(good, bad, dump, command.spu);
    list->setModel(model);
    auto selectionModel = list->selectionModel();
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setSelectionBehavior(QAbstractItemView::SelectRows);
    list->setColumnWidth(2, 200);
    for (int i = 3; i < 5; ++i) {
        list->setColumnWidth(i, 320);
    }

    QObject::connect(selectionModel, &QItemSelectionModel::currentRowChanged, [&](auto current) {
        if (current == QModelIndex())
            return;
        
        auto goodState = getState(good, current.row());
        auto goodModel = new StateModel(goodState);
        goodStateView->setModel(goodModel);
        goodStateView->resizeColumnsToContents();
        
        auto badState = getState(bad, current.row());
        auto badModel = new StateModel(badState);
        badStateView->setModel(badModel);
        badStateView->resizeColumnsToContents();
    });
    
    layout->addWidget(list, 0, 0, 2, 1);
    layout->addWidget(goodStateView, 0, 1);
    layout->addWidget(badStateView, 1, 1);
   
    window->show();
    app.exec();
}