#include "MainWindowModel.h"

#include "ui_ImageView.h"
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QAction>
#include <QDialog>
#include <QImage>
#include <QSize>
#include <QPainter>
#include <boost/endian/conversion.hpp>
#include "../ps3emu/utils.h"
#include "../ps3emu/rsx/Rsx.h"
#include "../ps3emu/rsx/RsxContext.h"
#include "../ps3emu/rsx/Tracer.h"
#include "../ps3emu/Process.h"
#include "../ps3emu/shaders/ShaderGenerator.h"
#include "../ps3emu/rsx/FragmentShaderUpdateFunctor.h"

using namespace boost::endian;

GcmCommandReplayInfo makeNopCommand() {
    return GcmCommandReplayInfo{GcmCommand{0, 0, (int)CommandId::waitForIdle}, true};
}

void execInRsxThread(Rsx* rsx, std::function<void()> action) {
    auto command = makeNopCommand();
    command.action = action;
    rsx->sendCommand(command);
    rsx->receiveCommandCompletion();
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
           if (id == CommandId::UpdateBufferCache ||
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
                        : entry.texture->format() == GL_RGB32F ? "GL_RGB32F"
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
        
        bool isRegular = row < regular.size();
        
        if (isRegular) {
            auto info = regular[row];
            handle = info.value->handle();
            width = info.key.width;
            height = info.key.height;
        } else {
            row -= regular.size();
            auto entry = framebuffer[row];
            handle = entry.texture->handle();
            width = entry.texture->width();
            height = entry.texture->height();
        }
        
        QImage background(width, height, QImage::Format_RGBA8888);
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                bool dark = (x / 10 + y / 10) % 2;
                background.setPixel(x, y, dark ? 0xff666666 : 0xff999999);
            }
        }
        
        QImage image(width, height, QImage::Format_RGBA8888);
        execInRsxThread(_rsx, [&] {
            assert(glIsTexture(handle));
            glGetTextureImage(handle, 
                              0,
                              GL_RGBA, 
                              GL_UNSIGNED_INT_8_8_8_8_REV, 
                              image.byteCount(), 
                              image.bits());
        });
        
        QPainter p(&background);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        p.drawImage(0, 0, image);
        p.end();
        if (!isRegular) {
            background = background.mirrored();
        }
        
        auto dialog = new QDialog();
        auto imageView = new Ui::ImageView();
        imageView->setupUi(dialog);
        auto pixmap = QPixmap::fromImage(background);
        imageView->labelImage->setPixmap(pixmap);
        
        dialog->show();
    }
};

class BufferTableModel : public QAbstractItemModel {
    Rsx* _rsx;
public:
    BufferTableModel(Rsx* rsx) : _rsx(rsx) { }
    
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return QVariant();
        switch (section) {
            case 0: return "Location";
            case 1: return "Offset";
        }
        return QVariant();
    }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 2;
    }
    
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole)
            return QVariant();
        auto cache = _rsx->context()->bufferCache.cacheSnapshot();
        auto key = cache[index.row()].key;
        switch (index.column()) {
            case 0: return key.location == MemoryLocation::Local ? "Local" : "Main";
            case 1: return QString::fromStdString(ssnprintf("#%x", key.offset));
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
        auto cache = _rsx->context()->bufferCache.cacheSnapshot();
        return cache.size();
    }
};

class ConstTableModel : public QAbstractItemModel {
    std::vector<std::tuple<unsigned, std::array<uint32_t, 4>>> _values;
public:
    ConstTableModel(std::vector<std::tuple<unsigned, std::array<uint32_t, 4>>> values) 
        : _values(values) { }
    
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return QVariant();
        switch (section) {
            case 0: return "#";
            case 1: return "x";
            case 2: return "y";
            case 3: return "z";
            case 4: return "w";
        }
        return QVariant();
    }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 5;
    }
    
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole)
            return QVariant();
        
        auto i = index.row() / 2;
        if (index.row() % 2 == 0) {
            switch (index.column()) {
                case 0: return QString::fromStdString(ssnprintf("%d", std::get<0>(_values[i])));
                case 1: return QString::fromStdString(ssnprintf("#%08x", std::get<1>(_values[i])[0]));
                case 2: return QString::fromStdString(ssnprintf("#%08x", std::get<1>(_values[i])[1]));
                case 3: return QString::fromStdString(ssnprintf("#%08x", std::get<1>(_values[i])[2]));
                case 4: return QString::fromStdString(ssnprintf("#%08x", std::get<1>(_values[i])[3]));
            }
        } else {
            switch (index.column()) {
                case 1: return QString::fromStdString(ssnprintf("%g", 
                    (float)union_cast<uint32_t, float>( std::get<1>(_values[i])[0] )));
                case 2: return QString::fromStdString(ssnprintf("%g", 
                    (float)union_cast<uint32_t, float>( std::get<1>(_values[i])[1] )));
                case 3: return QString::fromStdString(ssnprintf("%g", 
                    (float)union_cast<uint32_t, float>( std::get<1>(_values[i])[2] )));
                case 4: return QString::fromStdString(ssnprintf("%g", 
                    (float)union_cast<uint32_t, float>( std::get<1>(_values[i])[3] )));
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
        return _values.size() * 2;
    }
};

class VDATableModel : public QAbstractItemModel {
    Rsx* _rsx;
public:
    VDATableModel(Rsx* rsx) : _rsx(rsx) { }
    
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return QVariant();
        switch (section) {
            case 0: return "Enabled";
            case 1: return "Offset";
            case 2: return "Location";
            case 3: return "Frequency";
            case 4: return "Stride";
            case 5: return "Size";
            case 6: return "Type";
        }
        return QVariant();
    }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 7;
    }
    
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole)
            return QVariant();
        auto vda = _rsx->context()->vertexDataArrays[index.row()];
        auto input = _rsx->context()->vertexInputs[index.row()];
        switch (index.column()) {
            case 0: return QString::fromStdString(ssnprintf("%d", input.enabled));
            case 1: return QString::fromStdString(ssnprintf("#%08x", vda.offset));
            case 2: return vda.location == MemoryLocation::Local ? "Local" : "Main";
            case 3: return QString::fromStdString(ssnprintf("%d", vda.frequency));
            case 4: return QString::fromStdString(ssnprintf("%d", vda.stride));
            case 5: return QString::fromStdString(ssnprintf("%d", vda.size));
            case 6: return vda.type == CELL_GCM_VERTEX_UB ? "uint8_t"
                         : vda.type == CELL_GCM_VERTEX_F ? "float"
                         : "unknown";
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
        return _rsx->context()->vertexDataArrays.size();
    }
};

class VDABufferTableModel : public QAbstractItemModel {
    Rsx* _rsx;
    VertexDataArrayFormatInfo _info;
    uint8_t* _buffer;
    unsigned _size;
    
public:
    VDABufferTableModel(Rsx* rsx, std::uint8_t* buffer, unsigned vdaIndex, unsigned size) 
        : _rsx(rsx), _buffer(buffer), _size(size) {
        _info = _rsx->context()->vertexDataArrays[vdaIndex];
    }
    
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return QVariant();
        switch (section) {
            case 0: return "Offset";
            case 1: return "x";
            case 2: return "y";
            case 3: return "z";
            case 4: return "w";
        }
        return QVariant();
    }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return _info.size + 1;
    }
    
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole)
            return QVariant();
        
        auto offset = _info.stride * index.row();
        if (index.column() == 0) {
            return QString::fromStdString(ssnprintf("#%08x", offset));
        }
        
        auto typeSize = _info.type == CELL_GCM_VERTEX_UB ? 1 : 4;
        auto valueOffset = offset + (index.column() - 1) * typeSize;
        
        if (_info.type == CELL_GCM_VERTEX_UB) {
            uint8_t u8Value = *(uint8_t*)&_buffer[valueOffset];    
            return QString::fromStdString(ssnprintf("#%02x", u8Value));
        } else {
            float fValue = union_cast<uint32_t, float>(endian_reverse(*(uint32_t*)&_buffer[valueOffset]));
            return QString::fromStdString(ssnprintf("%g", fValue));
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
        return _size;
    }
};

class ContextTableModel : public QAbstractItemModel {
    Rsx* _rsx;
public:
    ContextTableModel(Rsx* rsx) : _rsx(rsx) { }
    
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return QVariant();
        switch (section) {
            case 0: return "Name";
            case 1: return "Value";
        }
        return QVariant();
    }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 2;
    }
    
    const char* printVertexArrayMode() const {
        switch (_rsx->context()->glVertexArrayMode) {
            case GL_QUADS: return "GL_QUADS";
            case GL_QUAD_STRIP: return "GL_QUAD_STRIP";
            case GL_POLYGON: return "GL_POLYGON";
            case GL_POINTS: return "GL_POINTS";
            case GL_LINES: return "GL_LINES";
            case GL_LINE_LOOP: return "GL_LINE_LOOP";
            case GL_LINE_STRIP: return "GL_LINE_STRIP";
            case GL_TRIANGLES: return "GL_TRIANGLES";
            case GL_TRIANGLE_STRIP: return "GL_TRIANGLE_STRIP";
            case GL_TRIANGLE_FAN: return "GL_TRIANGLE_FAN";
        }
        return "unknown";
    }
    
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole)
            return QVariant();
        
        if (index.column() == 0) {
            switch (index.row()) {
                case 0: return "glVertexArrayMode";
            }   
        }
        
        switch (index.row()) {
            case 0: return printVertexArrayMode();
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
        return 1;
    }
};

MainWindowModel::MainWindowModel() : _lastDrawCount(0), _currentCommand(0), _currentFrame(0) {
    _window.setupUi(&_qwindow);
    QObject::connect(_window.actionRun, &QAction::triggered, [=] { onRun(); });
    Rsx::setOperationMode(RsxOperationMode::Replay);
    QObject::connect(_window.commandTableView, &QTableView::doubleClicked, [=] (auto index) {
        runTo(index.row(), _currentFrame);
    });
}

MainWindowModel::~MainWindowModel() = default;

QMainWindow* MainWindowModel::window() {
    return &_qwindow;
}

void MainWindowModel::loadTrace(std::string path) {
    _db.createOrOpen(path);
    changeFrame();
}

void MainWindowModel::onRun() {
    runTo(_db.commands(_currentFrame) - 1, _currentFrame);
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
        std::vector<unsigned> usedConsts;
        auto glslText = GenerateVertexShader(context->vertexInstructions.data(),
                                         context->vertexInputs,
                                         samplerSizes,
                                         0, // TODO: loadAt
                                         &usedConsts);
        _window.teVertexAsm->setText(QString::fromStdString(asmText));
        _window.teVertexGlsl->setText(QString::fromStdString(glslText));
        
        std::vector<std::tuple<unsigned, std::array<uint32_t, 4>>> values;
        auto& buffer = _rsx->context()->vertexConstBuffer;
        auto uints = (std::array<uint32_t, 4>*)buffer.mapped();
        for (auto constIndex : usedConsts) {
            values.push_back(std::make_tuple(constIndex, uints[constIndex]));
        }
        auto vertexConstModel = new ConstTableModel(values);
        _window.twVertexConsts->setModel(vertexConstModel);
    }
    
    if (context->fragmentShader) {
        FragmentShaderCacheKey key { context->fragmentVa, FragmentProgramSize };
        FragmentShaderUpdateFunctor* updater;
        FragmentShader* shader;
        std::tie(shader, updater) = context->fragmentShaderCache.retrieveWithUpdater(key);
        if (updater) {
            auto const& bytecode = updater->bytecode();
            // TODO: handle sizes
            std::array<int, 16> sizes = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
            auto asmText = PrintFragmentProgram(&bytecode[0]);
            auto glslText = GenerateFragmentShader(bytecode, sizes, context->isFlatShadeMode);
            _window.teFragmentAsm->setText(QString::fromStdString(asmText));
            _window.teFragmentGlsl->setText(QString::fromStdString(glslText));
            
            auto info = get_fragment_bytecode_info(&bytecode[0]);
            
            std::vector<std::tuple<unsigned, std::array<uint32_t, 4>>> values;
            auto buffer = updater->constBuffer();
            auto ptr = (std::array<uint32_t, 4>*)buffer->mapped();
            auto index = 0u;
            for (auto i = 0u; i < info.constMap.size(); ++i) {
                if (info.constMap[i]) {
                    values.push_back(std::make_tuple(index, ptr[index]));
                    index++;
                }
            }
            auto vertexConstModel = new ConstTableModel(values);
            _window.twFragmentConsts->setModel(vertexConstModel);
        }
    }
    
    auto textureModel = new TextureTableModel(_rsx.get());
    _window.twTextures->setModel(textureModel);
    QObject::disconnect(_window.twTextures, &QTableView::doubleClicked, 0, 0);
    QObject::connect(_window.twTextures, &QTableView::doubleClicked, [=] (auto index) {
        textureModel->showPreview(index.row());
    });
    
    auto bufferModel = new BufferTableModel(_rsx.get());
    _window.twBuffers->setModel(bufferModel);
    
    auto vdaModel = new VDATableModel(_rsx.get());
    QObject::disconnect(_window.twVertexDataArrays, &QTableView::clicked, 0, 0);
    QObject::connect(_window.twVertexDataArrays, &QTableView::clicked, [&] (auto index) {
        auto input = _rsx->context()->vertexInputs[index.row()];
        if (!input.enabled)
            return;
        auto info = _rsx->context()->vertexDataArrays[index.row()];
        auto buffer = _rsx->getBuffer(info.location);
        auto mapped = (uint8_t*)buffer->mapped() + info.offset;
        auto vdaBufferModel = new VDABufferTableModel(_rsx.get(), mapped, index.row(), _lastDrawCount);
        _window.twVertexDataArraysBuffer->setModel(vdaBufferModel);
    });
    _window.twVertexDataArrays->setModel(vdaModel);
    
    auto contextModel = new ContextTableModel(_rsx.get());
    _window.twContext->setModel(contextModel);
    _window.twContext->resizeColumnsToContents();
}

void MainWindowModel::runTo(unsigned lastCommand, unsigned frame) {
    if (frame >= _db.frames())
        return;
    
    if (!_proc) {
        _proc.reset(new Process());
        _rsx.reset(new Rsx());
        _rsx->init(_proc.get());
    }
    
    std::vector<GcmCommand> commands;
    for (auto i = _currentCommand; i <= lastCommand; ++i) {
        auto command = _db.getCommand(frame, i);
        commands.push_back(command);
    }
    
    for (auto& c : commands) {
        auto id = (CommandId)c.id;
        if (id == CommandId::DrawArrays || id == CommandId::DrawIndexArray) {
            _lastDrawCount = c.args[0].value + c.args[1].value; // first + count
        }
        _rsx->sendCommand({c, false});
    }
    
    _rsx->sendCommand(makeNopCommand());
    _rsx->receiveCommandCompletion();
    
    update();
    
    _currentCommand = (lastCommand + 1) % _db.commands(frame);
    if (_currentCommand == 0) {
        _currentFrame++;
        changeFrame();
    }
}

void MainWindowModel::changeFrame() {
    auto text = ssnprintf("Frame: %d/%d", _currentFrame, _db.frames());
    _window.labelFrame->setText(QString::fromStdString(text));
    
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
    
    QObject::disconnect(_window.leSearchCommand, &QLineEdit::textChanged, 0, 0);
    QObject::connect(_window.leSearchCommand, &QLineEdit::textChanged, [=] (auto text) {
        
    });
}
