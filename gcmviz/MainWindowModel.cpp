#include "MainWindowModel.h"

#include "ui_ImageView.h"
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QAction>
#include <QDialog>
#include <QImage>
#include <QSize>
#include "../ps3emu/utils.h"
#include "../ps3emu/rsx/Rsx.h"
#include "../ps3emu/rsx/RsxContext.h"
#include "../ps3emu/rsx/Tracer.h"
#include "../ps3emu/Process.h"
#include "../ps3emu/shaders/ShaderGenerator.h"
#include "../ps3emu/rsx/FragmentShaderUpdateFunctor.h"

GcmCommandReplayInfo makeNopCommand() {
    return GcmCommandReplayInfo{GcmCommand{0, 0, (int)CommandId::waitForIdle}, true};
}

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
        auto command = _db->getCommand(0, index.row());
        auto id = (CommandId)command.id;
        if (role == Qt::BackgroundColorRole) {
           if (id == CommandId::DrawArrays || id == CommandId::DrawIndexArray) {
               return QColor(Qt::green);
           }
           if (id == CommandId::addBufferToCache ||
               id == CommandId::UpdateBufferCache ||
               id == CommandId::addFragmentShaderToCache ||
               id == CommandId::UpdateFragmentCache ||
               id == CommandId::addTextureToCache ||
               id == CommandId::UpdateTextureCache ||
               id == CommandId::updateOffsetTableForReplay)
               return QColor(Qt::yellow);
           return QVariant();
        }
        if (role != Qt::DisplayRole)
            return QVariant();
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

class TextureTableModel : public QAbstractItemModel {
    Rsx* _rsx;
public:
    TextureTableModel(Rsx* rsx) : _rsx(rsx) { }
    
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return QVariant();
        switch (section) {
            case 0: return "Offset";
            case 1: return "Location";
            case 2: return "Width";
            case 3: return "Height";
            case 4: return "Kind";
            case 5: return "Format";
        }
        return QVariant();
    }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 6;
    }
    
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole)
            return QVariant();
        auto regular = _rsx->context()->textureCache.cacheSnapshot();
        auto framebuffer = _rsx->context()->framebuffer->cacheSnapshot();
        if (index.row() < regular.size()) {
            auto texture = regular[index.row()];
            switch (index.column()) {
                case 0: return QString::fromStdString(ssnprintf("#%08x", texture.key.offset));
                case 1: return (MemoryLocation)texture.key.location == MemoryLocation::Local ? "Local" : "Main";
                case 2: return QString::fromStdString(ssnprintf("%d", texture.key.width));
                case 3: return QString::fromStdString(ssnprintf("%d", texture.key.height));
                case 4: return "Regular";
                case 5: return "GL_RGBA32F";
            }
        } else {
            auto entry = framebuffer[index.row() - regular.size()];
            auto format = entry.texture->format() == GL_DEPTH_COMPONENT16 ? "GL_DEPTH_COMPONENT16"
                        : entry.texture->format() == GL_DEPTH24_STENCIL8 ? "GL_DEPTH24_STENCIL8"
                        : entry.texture->format() == GL_RGBA32F ? "GL_RGBA32F"
                        : "unknown";
            switch (index.column()) {
                case 0: return QString::fromStdString(ssnprintf("#%08x", entry.va));
                case 1: return "Local";
                case 2: return QString::fromStdString(ssnprintf("%d", entry.texture->width()));
                case 3: return QString::fromStdString(ssnprintf("%d", entry.texture->height()));
                case 4: return "Framebuffer";
                case 5: return format;
            }
        }
        return QVariant();
    }
    
    QModelIndex parent(const QModelIndex& child) const override {
        return QModelIndex();
    }
    
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override {
        return createIndex(row, column);
    }
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        auto regular = _rsx->context()->textureCache.cacheSnapshot();
        auto framebuffer = _rsx->context()->framebuffer->cacheSnapshot();
        return regular.size() + framebuffer.size();
    }
    
    void showPreview(unsigned row) {
        auto regular = _rsx->context()->textureCache.cacheSnapshot();
        auto framebuffer = _rsx->context()->framebuffer->cacheSnapshot();
        
        uint32_t width, height;
        GLuint handle;
        QImage::Format qtFormat;
        
        if (row < regular.size()) {
            auto info = regular[row];
            handle = info.value->handle();
            width = info.key.width;
            height = info.key.height;
            qtFormat = QImage::Format_RGBA8888;
        } else {
            row -= regular.size();
            auto entry = framebuffer[row];
            handle = entry.texture->handle();
            width = entry.texture->width();
            height = entry.texture->height();
            qtFormat = QImage::Format_RGBA8888;
        }
        
        QImage image(width, height, qtFormat);
        
        auto command = makeNopCommand();
        command.action = [&] {
            assert(glIsTexture(handle));
            glGetTextureImage(handle, 
                              0,
                              GL_RGBA, 
                              GL_UNSIGNED_INT_8_8_8_8_REV, 
                              image.byteCount(), 
                              image.bits());
        };
        
        _rsx->sendCommand(command);
        _rsx->receiveCommandCompletion();
        
        auto dialog = new QDialog();
        auto imageView = new Ui::ImageView();
        imageView->setupUi(dialog);
        auto pixmap = QPixmap::fromImage(image);
        imageView->labelImage->setPixmap(pixmap);
        
        dialog->show();
    }
};

MainWindowModel::MainWindowModel() : _currentCommand(0) {
    _window.setupUi(&_qwindow);
    QObject::connect(_window.actionRun, &QAction::triggered, [=] { onRun(); });
    Rsx::setOperationMode(RsxOperationMode::Replay);
    QObject::connect(_window.commandTableView, &QTableView::doubleClicked, [=] (auto index) {
        runTo(index.row());
    });
}

MainWindowModel::~MainWindowModel() = default;

QMainWindow* MainWindowModel::window() {
    return &_qwindow;
}

void MainWindowModel::loadTrace(std::string path) {
    _db.createOrOpen(path);
    auto commandModel = new CommandTableModel(&_db);
    _window.commandTableView->setModel(commandModel);
    _window.commandTableView->resizeColumnsToContents();
    auto selectionModel = _window.commandTableView->selectionModel();
    QObject::connect(selectionModel, &QItemSelectionModel::currentRowChanged, [=] (auto current) {
        if (current == QModelIndex())
            return;
        auto command = _db.getCommand(0, current.row());
        _window.twArgs->setModel(new ArgumentTableModel(command));
    });
}

void MainWindowModel::onRun() {
    runTo(_db.commands(0) - 1);
}

void MainWindowModel::update() {
    auto context = _rsx->context();
    if (context->vertexShader) {
        auto instructions = context->vertexInstructions.data();
        std::array<int, 4> samplerSizes = { 
            context->vertexTextureSamplers[0].texture.dimension,
            context->vertexTextureSamplers[1].texture.dimension,
            context->vertexTextureSamplers[2].texture.dimension,
            context->vertexTextureSamplers[3].texture.dimension
        };
        auto asmText = PrintVertexProgram(instructions);
        auto glslText = GenerateVertexShader(context->vertexInstructions.data(),
                                         context->vertexInputs,
                                         samplerSizes,
                                         0); // TODO: loadAt
        _window.teVertexAsm->setText(QString::fromStdString(asmText));
        _window.teVertexGlsl->setText(QString::fromStdString(glslText));
    }
    
    if (context->fragmentShader) {
        FragmentShaderCacheKey key { context->fragmentVa, FragmentProgramSize };
        FragmentShaderUpdateFunctor* updater;
        FragmentShader* shader;
        std::tie(shader, updater) = context->fragmentShaderCache.retrieveWithUpdater(key);
        auto const& bytecode = updater->bytecode();
        
        // TODO: handle sizes
        std::array<int, 16> sizes = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
        auto asmText = PrintFragmentProgram(&bytecode[0]);
        auto glslText = GenerateFragmentShader(bytecode, sizes, context->isFlatShadeMode);
        _window.teFragmentAsm->setText(QString::fromStdString(asmText));
        _window.teFragmentGlsl->setText(QString::fromStdString(glslText));
    }
    
    auto textureModel = new TextureTableModel(_rsx.get());
    _window.twTextures->setModel(textureModel);
    QObject::disconnect(_window.twTextures, &QTableView::doubleClicked, 0, 0);
    QObject::connect(_window.twTextures, &QTableView::doubleClicked, [=] (auto index) {
        textureModel->showPreview(index.row());
    });
}

void MainWindowModel::runTo(int last) {
    if (!_proc) {
        _proc.reset(new Process());
        _rsx.reset(new Rsx());
        _rsx->init(_proc.get());
    }
    
    for (auto i = _currentCommand; i <= last; ++i) {
        auto command = _db.getCommand(0, i);
        _rsx->sendCommand({command, false});
    }
    
    _rsx->sendCommand(makeNopCommand());
    _rsx->receiveCommandCompletion();
    
    update();
    
    _currentCommand = last + 1;
}
