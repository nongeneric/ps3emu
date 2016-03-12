#include "MainWindowModel.h"

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include "../ps3emu/utils.h"
#include "../ps3emu/rsx/Rsx.h"

class CommandTableModel : public QAbstractItemModel {
    GcmDatabase* _db;
    
public:
    CommandTableModel(GcmDatabase* db) : _db(db) { }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 1;
    }
    
    QModelIndex index(int row,
                      int column,
                      const QModelIndex& parent = QModelIndex()) const override {
        return createIndex(row, column);
    }
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return _db->commands(0);
    }
    
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override {
         if (role != Qt::DisplayRole)
             return QVariant();
         auto command = _db->getCommand(0, index.row());
         return printCommandId((CommandId)command.id);
    }
    
    QModelIndex parent(const QModelIndex& child) const override {
        return QModelIndex();
    }
};

class ArgumentTableModel : public QAbstractItemModel {
    GcmCommand _command;
public:
    ArgumentTableModel(GcmCommand const& command) : _command(command) { }
    
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return QVariant();
        switch (section) {
            case 0: return "Name";
            case 1: return "Value";
            case 2: return "Hex";
            case 3: return "Type";
        }
        return QVariant();
    }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 4;
    }
    
    QModelIndex index(int row,
                      int column,
                      const QModelIndex& parent = QModelIndex()) const override {
        return createIndex(row, column);
    }
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return _command.args.size();
    }
    
    std::string printArgDecimal(GcmCommandArg const& arg) const {
        switch ((GcmArgType)arg.type) {
            case GcmArgType::None: return ssnprintf("NONE(#%x)", arg.value);
            case GcmArgType::Bool: return arg.value ? "True" : "False";
            case GcmArgType::Float: {
                float value = union_cast<uint32_t, float>(arg.value);
                return ssnprintf("%g", value);
            }
            case GcmArgType::Int32:
            case GcmArgType::Int16:
            case GcmArgType::UInt8:
            case GcmArgType::UInt16:
            case GcmArgType::UInt32: return ssnprintf("%d", arg.value);
        }
    }
    
    std::string printArgHex(GcmCommandArg const& arg) const {
        switch ((GcmArgType)arg.type) {
            case GcmArgType::None:
            case GcmArgType::Bool:
            case GcmArgType::UInt8: return ssnprintf("#%02x", arg.value);
            case GcmArgType::Int16:
            case GcmArgType::UInt16: return ssnprintf("#%04x", arg.value);
            case GcmArgType::Float:
            case GcmArgType::Int32:
            case GcmArgType::UInt32: return ssnprintf("#%08x", arg.value);
        }
    }
    
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override {
         if (role != Qt::DisplayRole)
             return QVariant();
         auto& arg = _command.args[index.row()];
         switch (index.column()) {
             case 0: return QString::fromStdString(arg.name);
             case 1: return QString::fromStdString(printArgDecimal(arg));
             case 2: return QString::fromStdString(printArgHex(arg));
             case 3: return printArgType((GcmArgType)arg.type);
         }
         return QVariant();
    }
    
    QModelIndex parent(const QModelIndex& child) const override {
        return QModelIndex();
    }
};

MainWindowModel::MainWindowModel() {
    _window.setupUi(&_qwindow);
    Rsx::setOperationMode(RsxOperationMode::Replay);
}

QMainWindow* MainWindowModel::window() {
    return &_qwindow;
}

void MainWindowModel::loadTrace(std::string path) {
    _db.createOrOpen(path);
    auto commandModel = new CommandTableModel(&_db);
    _window.commandTableView->setModel(commandModel);
    auto selectionModel = _window.commandTableView->selectionModel();
    QObject::connect(selectionModel, &QItemSelectionModel::currentRowChanged, [=] (auto current) {
        if (current == QModelIndex())
            return;
        auto command = _db.getCommand(0, current.row());
        _window.argTableView->setModel(new ArgumentTableModel(command));
    });
}
