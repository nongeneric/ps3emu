#include "MainWindowModel.h"

#include "ContextTables.h"
#include "ui_ImageView.h"
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QAction>
#include <QDialog>
#include <QImage>
#include <QSize>
#include <QPainter>
#include <boost/endian/conversion.hpp>
#include <boost/range/numeric.hpp>
#include <boost/algorithm/string/join.hpp>
#include "ps3emu/utils.h"
#include "ps3emu/rsx/Rsx.h"
#include "ps3emu/rsx/RsxContext.h"
#include "ps3emu/rsx/Tracer.h"
#include "ps3emu/Process.h"
#include "ps3emu/shaders/ShaderGenerator.h"
#include "ps3emu/state.h"
#include "ps3emu/shaders/shader_dasm.h"
#include "ps3emu/libs/graphics/gcm.h"
#include "OpenGLPreview.h"
#include "OpenGLPreviewWidget.h"

#define GCM_EMU
#include <gcm_tool.h>

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
    int _frame;
    
public:
    CommandTableModel(GcmDatabase* db, int frame) : _db(db), _frame(frame) { }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 1;
    }
    
    QModelIndex index(int row,
                      int column,
                      const QModelIndex& parent = QModelIndex()) const override {
        return createIndex(row, column);
    }
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return _db->commands(_frame);
    }
    
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override {
        auto command = _db->getCommand(_frame, index.row());
        auto id = (CommandId)command.id;
        if (role == Qt::BackgroundColorRole) {
           if (id == CommandId::DrawArrays || id == CommandId::DrawIndexArray ||
               id == CommandId::ClearSurface) {
               return QColor(Qt::green);
           }
           if (id == CommandId::UpdateBufferCache ||
               id == CommandId::updateOffsetTableForReplay)
               return QColor(Qt::yellow);
           return QVariant();
        }
        if (role != Qt::DisplayRole)
            return QVariant();
        auto display = printCommandId((CommandId)command.id);
        if (command.blob.empty())
            return display;
        return QString::fromStdString(
            ssnprintf("%s (blob %d)", display, command.blob.size()));
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
            case 4: return "Comment";
        }
        return QVariant();
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
        return _command.args.size();
    }
    
    QString printComment(GcmCommandArg arg) const {
        std::string res;
        if ((CommandId)_command.id == CommandId::ClearSurface && arg.name == "mask") {
            std::vector<std::string> flags;
            if (arg.value & CELL_GCM_CLEAR_R)
                flags.push_back("CELL_GCM_CLEAR_R");
            if (arg.value & CELL_GCM_CLEAR_G)
                flags.push_back("CELL_GCM_CLEAR_G");
            if (arg.value & CELL_GCM_CLEAR_B)
                flags.push_back("CELL_GCM_CLEAR_B");
            if (arg.value & CELL_GCM_CLEAR_A)
                flags.push_back("CELL_GCM_CLEAR_A");
            if (arg.value & CELL_GCM_CLEAR_Z)
                flags.push_back("CELL_GCM_CLEAR_Z");
            if (arg.value & CELL_GCM_CLEAR_S)
                flags.push_back("CELL_GCM_CLEAR_S");
            res = boost::algorithm::join(flags, " | ");
        }
        return QString::fromStdString(res);;
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
             case 4: return printComment(arg);
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
        if ((unsigned)index.row() < regular.size()) {
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
                        : entry.texture->format() == GL_RGB8 ? "GL_RGB8"
                        : entry.texture->format() == GL_RGBA8 ? "GL_RGBA8"
                        : "unknown";
            switch (index.column()) {
                case 0: return QString::fromStdString(ssnprintf("#%08x", entry.key.offset));
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
        for (auto x = 0u; x < width; ++x) {
            for (auto y = 0u; y < height; ++y) {
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
        
        if (!isRegular) {
            image = image.mirrored();
        }
        
        auto dialog = new QDialog();
        auto imageView = new Ui::ImageView();
        imageView->setupUi(dialog);
        
        auto setPixmap = [=](bool alpha1, bool alphaOnly) {
            auto background_copy = background;
            for (auto x = 0u; x < width; ++x) {
                for (auto y = 0u; y < height; ++y) {
                    auto pixel = image.pixel(x, y);
                    if (alphaOnly) {
                        auto alpha = pixel = 0xff000000;
                        pixel = alpha << 24 | alpha << 16 | alpha << 8 | alpha;
                    } else if (alpha1) {
                        pixel |= 0xff000000;
                    }
                    background_copy.setPixel(x, y, pixel);
                }
            }
            QPainter p(&background_copy);
            p.setCompositionMode(QPainter::CompositionMode_SourceOver);
            p.drawImage(0, 0, image);
            p.end();
            auto pixmap = QPixmap::fromImage(background_copy);
            imageView->labelImage->setPixmap(pixmap);
        };
        setPixmap(false, false);
        
        QObject::connect(imageView->cbAlpha1, &QAbstractButton::clicked, [=] {
            setPixmap(imageView->cbAlpha1->isChecked(), imageView->cbAlphaOnly->isChecked());
        });
        
        QObject::connect(imageView->cbAlphaOnly, &QAbstractButton::clicked, [=] {
            setPixmap(imageView->cbAlpha1->isChecked(), imageView->cbAlphaOnly->isChecked());
        });
        
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
        (void)_rsx;
        //auto cache = _rsx->context()->bufferCache.cacheSnapshot();
        //auto key = cache[index.row()].key;
        switch (index.column()) {
//             case 0: return key.location == MemoryLocation::Local ? "Local" : "Main";
//             case 1: return QString::fromStdString(ssnprintf("#%x", key.offset));
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
        return 0;
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
            case 7: return "Divider Op";
        }
        return QVariant();
    }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 8;
    }
    
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole)
            return QVariant();
        
        if ((unsigned)index.row() == _rsx->context()->vertexDataArrays.size()) {
            switch (index.column()) {
                case 0: return "TRANSFORM";
                case 1: return "FEEDBACK";
                case 2: return "";
                case 3: return 0;
                case 4: return 12;
                case 5: return _rsx->context()->feedbackCount;
                case 6: return "float";
                case 7: return "";
            }
        }
        
        auto vda = _rsx->context()->vertexDataArrays[index.row()];
        auto input = _rsx->context()->vertexInputs[index.row()];
        auto op = _rsx->context()->frequencyDividerOperation & (1 << index.row());
        switch (index.column()) {
            case 0: return QString::fromStdString(ssnprintf("%d", input.rank != 0));
            case 1: return QString::fromStdString(ssnprintf("#%08x", vda.offset));
            case 2: return vda.location == MemoryLocation::Local ? "Local" : "Main";
            case 3: return QString::fromStdString(ssnprintf("%d", vda.frequency));
            case 4: return QString::fromStdString(ssnprintf("%d", vda.stride));
            case 5: return QString::fromStdString(ssnprintf("%d", vda.size));
            case 6: return vda.type == CELL_GCM_VERTEX_UB ? "uint8_t"
                         : vda.type == CELL_GCM_VERTEX_F ? "float"
                         : "unknown";
            case 7: return op ? "MODULO" : "DIVIDE";
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
        return _rsx->context()->vertexDataArrays.size() + 1;
    }
};

class VDABufferTableModel : public QAbstractItemModel {
    Rsx* _rsx;
    uint8_t* _buffer;
    unsigned _size;
    VertexDataArrayFormatInfo _info;
    bool _be;
    std::unique_ptr<OpenGLPreview> _openglPreview;
    bool isFeedback;
    
public:
    VDABufferTableModel(Rsx* rsx,
                        std::uint8_t* buffer,
                        unsigned vdaIndex,
                        unsigned size,
                        VertexDataArrayFormatInfo info,
                        bool be)
        : _rsx(rsx), _buffer(buffer), _size(size), _info(info), _be(be) {
        isFeedback = vdaIndex == 16;
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
            if (!_be) {
                fValue = union_cast<uint32_t, float>(*(uint32_t*)&_buffer[valueOffset]);
            }
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
        if (_info.frequency == 1)
            return _size;
        if (_info.frequency != 0)
            return _info.frequency;
        return _size;
    }
    
    void showPreview() {
        std::vector<PreviewVertex> vertices;
        auto ptr = _buffer;
        for (auto i = 0; i < rowCount(); ++i) {
            auto component = (uint32_t*)ptr;
            PreviewVertex v = {0};
            for (auto j = 0; j < std::min<int>(3, _info.size); ++j) {
                if (_be) {
                    v.xyz[j] = union_cast<uint32_t, float>(endian_reverse(component[j]));
                } else {
                    v.xyz[j] = union_cast<uint32_t, float>(component[j]);
                }
            }
            ptr += _info.stride;
            vertices.push_back(v);
        }
        
        _openglPreview.reset(new OpenGLPreview());
        _openglPreview->widget()->setVertices(vertices);
        _openglPreview->widget()->setMode(isFeedback ? _rsx->context()->feedbackMode
                                                     : _rsx->context()->glVertexArrayMode);
        _openglPreview->setWindowTitle(QString::fromStdString(ssnprintf(
            "%s, %d", to_string(_rsx->context()->vertexArrayMode), vertices.size())));
        _openglPreview->show();
    }
};

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

void MainWindowModel::onVisualizeFeedback() {
//     auto buf = (std::array<float, 4>*)_rsx->context()->feedbackBuffer.mapped();
//     std::vector<PreviewVertex> vertices;
//     for (auto i = 0u; i < _rsx->context()->feedbackCount; ++i) {
//         PreviewVertex v;
//         v.xyz[0] = buf[i][0];
//         v.xyz[1] = buf[i][1];
//         v.xyz[2] = buf[i][2];
//         vertices.push_back(v);
//     }
//     _openglPreview.reset(new OpenGLPreview());
//     _openglPreview->widget()->setVertices(vertices);
//     _openglPreview->widget()->setMode(_rsx->context()->feedbackMode);
//     _openglPreview->setWindowTitle(QString::fromStdString(ssnprintf("%s, %d",
//                                    to_string(_rsx->context()->vertexArrayMode),
//                                    vertices.size())));
//     _openglPreview->show();
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
        
        auto vertexBytecodeText = PrintVertexBytecode(&context->vertexInstructions[0]);
        _window.teVertexBytecode->setText(QString::fromStdString(vertexBytecodeText));
    }
    
    if (context->fragmentShader) {
        FragmentShaderCacheKey key{context->fragmentBytecode, isMrt(context->surface)};
        auto shader = context->fragmentShaderCache.retrieve(key);
        if (shader) {
            auto sizes = getFragmentSamplerSizes(context);
            auto asmText = PrintFragmentProgram(&context->fragmentBytecode[0]);
            bool mrt = boost::accumulate(context->surface.colorTarget, 0) > 1;
            auto glslText = GenerateFragmentShader(context->fragmentBytecode, sizes, context->isFlatShadeMode, mrt);
            _window.teFragmentAsm->setText(QString::fromStdString(asmText));
            _window.teFragmentGlsl->setText(QString::fromStdString(glslText));
            
            auto bytecodeText = PrintFragmentBytecode(&context->fragmentBytecode[0]);
            _window.teFragmentBytecode->setText(QString::fromStdString(bytecodeText));
            
            std::vector<std::tuple<unsigned, std::array<uint32_t, 4>>> values;
            auto ptr = (std::array<uint32_t, 4>*)context->fragmentConstBuffer.mapped();
            for (auto i = 0u; i < context->fragmentConstCount; ++i) {
                values.push_back(std::make_tuple(i, ptr[i]));
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
        if (input.rank == 0)
            return;
        uint8_t* mapped;
        VertexDataArrayFormatInfo info = {};
        bool be;
        if ((unsigned)index.row() == _rsx->context()->vertexDataArrays.size()) {
            info.frequency = 0;
            info.stride = 16;
            info.size = 4;
            info.type = CELL_GCM_VERTEX_F;
            mapped = _rsx->context()->feedbackBuffer.mapped();
            be = false;
        } else {
            info = _rsx->context()->vertexDataArrays[index.row()];
            auto buffer = _rsx->getBuffer(info.location);
            mapped = (uint8_t*)buffer->mapped() + info.offset;
            be = true;
        }
        auto vdaBufferModel = new VDABufferTableModel(
            _rsx.get(), mapped, index.row(), _lastDrawCount, info, be);
        _window.twVertexDataArraysBuffer->setModel(vdaBufferModel);
        
        QObject::disconnect(_window.pbVisualizeGeometry, &QPushButton::clicked, 0, 0);
        QObject::connect(_window.pbVisualizeGeometry, &QPushButton::clicked, [=] { 
            vdaBufferModel->showPreview();
        });
    });
    _window.twVertexDataArrays->setModel(vdaModel);
    updateContextTable();
}

void MainWindowModel::runTo(unsigned lastCommand, unsigned frame) {
    if (frame >= (unsigned)_db.frames())
        return;
    
    if (!_proc) {
        _proc.reset(new Process());
        _rsx.reset(new Rsx());
        _rsx->init();
        _rsx->resetContext();
        uint32_t last = gcmInitCommandsSize - 4; // except the reset call
        auto read = [&](uint32_t get) {
            auto ptr = &gcmInitCommands[get];
            return *(big_uint32_t*)ptr;
        };
        for (auto get = 0u; get != last;) {
            get += _rsx->interpret(get, read);
        }
    }
    
    std::vector<GcmCommand> commands;
    for (auto i = _currentCommand; i <= lastCommand; ++i) {
        auto command = _db.getCommand(frame, i);
        commands.push_back(command);
    }
    
    for (auto& c : commands) {
        auto id = (CommandId)c.id;
        if (id == CommandId::DrawArrays || id == CommandId::DrawIndexArray) {
            // first + count (first is actually in bytes, not attributes, so it is an overfetch
            _lastDrawCount = c.args[0].value + c.args[1].value;
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
    
    auto commandModel = new CommandTableModel(&_db, _currentFrame);
    _window.commandTableView->setModel(commandModel);
    _window.commandTableView->resizeColumnsToContents();
    auto selectionModel = _window.commandTableView->selectionModel();
    QObject::connect(selectionModel, &QItemSelectionModel::currentRowChanged, [=] (auto current) {
        if (current == QModelIndex())
            return;
        auto command = _db.getCommand(_currentFrame, current.row());
        _window.twArgs->setModel(new ArgumentTableModel(command));
    });
    
    QObject::disconnect(_window.leSearchCommand, &QLineEdit::textChanged, 0, 0);
    QObject::connect(_window.leSearchCommand, &QLineEdit::textChanged, [=] (auto text) {
        
    });
}

MainWindowModel::MainWindowModel() : _lastDrawCount(0), _currentCommand(0), _currentFrame(0) {
    _window.setupUi(&_qwindow);
    QObject::connect(_window.actionRun, &QAction::triggered, [=] { onRun(); });
    QObject::connect(_window.actionVisualizeFeedback, &QAction::triggered, [=] {
        onVisualizeFeedback();
    });
    Rsx::setOperationMode(RsxOperationMode::Replay);
    QObject::connect(_window.commandTableView, &QTableView::doubleClicked, [=] (auto index) {
        this->runTo(index.row(), _currentFrame);
    });
    _window.contextTree->setColumnCount(1);
    QList<QTreeWidgetItem*> items;
    items.append(new SurfaceContextTreeItem());
    items.append(new FragmentOperationsTreeItem());
    
    auto vertexSamplers = new QTreeWidgetItem();
    vertexSamplers->setText(0, "Vertex Samplers");
    for (auto i = 0u; i < 4; ++i) {
        vertexSamplers->addChild(new SamplerContextTreeItem(false, i));
        vertexSamplers->addChild(new SamplerTextureContextTreeItem(false, i));
    }
    
    auto fragmentSamplers = new QTreeWidgetItem();
    fragmentSamplers->setText(0, "Fragment Samplers");
    for (auto i = 0u; i < 16; ++i) {
        fragmentSamplers->addChild(new SamplerContextTreeItem(true, i));
        fragmentSamplers->addChild(new SamplerTextureContextTreeItem(true, i));
    }
    
    items.append(vertexSamplers);
    items.append(fragmentSamplers);
    
    _window.contextTree->insertTopLevelItems(0, items);
    QObject::connect(_window.contextTree, &QTreeWidget::currentItemChanged, [=] (auto item) {
        this->updateContextTable();
    });
    _window.contextTree->expandAll();
}

void MainWindowModel::updateContextTable() {
    if (!_rsx)
        return;
    auto item = _window.contextTree->currentItem();
    auto typed = dynamic_cast<ContextTreeItem*>(item);
    if (typed) {
        _window.contextTable->setModel(typed->getTable(_rsx->context()));
        _window.contextTable->resizeColumnsToContents();
    }
}
