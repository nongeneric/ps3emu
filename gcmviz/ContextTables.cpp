#include "ContextTables.h"
#include "../ps3emu/utils.h"
#include "../ps3emu/rsx/RsxContext.h"

SurfaceContextTreeItem::SurfaceContextTreeItem() : ContextTreeItem("Surface") {}

ContextTreeItem::ContextTreeItem(std::string name)
    : QTreeWidgetItem((QTreeWidget*)nullptr,
                      QStringList(QString::fromStdString(name))) {}

template <typename T> std::string printHex(T num) {
    return sformat("#{:08x}", num);
}

template <typename T> std::string printDec(T num) {
    return sformat("{}", num);
}

std::string printBool(bool value) {
    return value ? "TRUE" : "FALSE";
}

#define DECHEX(x, p) {\
    #p, [=](auto) { return printDec(x. p); },\
        [=](auto) { return printHex(x. p); },\
},

#define HEX(x, p) {\
    #p, [=](auto) { return printHex(x. p); },\
        [=](auto) { return ""; },\
},

#define DEC(x, p) {\
    #p, [=](auto) { return printDec(x. p); },\
        [=](auto) { return ""; },\
},

#define BOOL(x, p) {\
    #p, [=](auto) { return printBool(x. p); },\
        [=](auto) { return ""; },\
},

#define FLOAT(x, p) {\
    #p, [=](auto) { return sformat("{}", x. p); },\
        [=](auto) { return ""; },\
},

#define PRINT_ENUM(x, p) {\
    #p, [=](auto) { return to_string(x. p); },\
        [=](auto) { return ""; },\
},

GenericTableModel<RsxContext>* SurfaceContextTreeItem::getTable(
    RsxContext* context) {
    return new GenericTableModel<RsxContext>(
        context,
        {
            {"Depth location",
             [=](auto c) { return to_string(c->surface.depthLocation); },
             [=](auto c) { return ""; }},
            {"Color location [0]",
             [=](auto c) { return to_string(c->surface.colorLocation[0]); },
             [=](auto c) { return ""; }},
            {"Color location [1]",
             [=](auto c) { return to_string(c->surface.colorLocation[1]); },
             [=](auto c) { return ""; }},
            {"Color location [2]",
             [=](auto c) { return to_string(c->surface.colorLocation[2]); },
             [=](auto c) { return ""; }},
            {"Color location [3]",
             [=](auto c) { return to_string(c->surface.colorLocation[3]); },
             [=](auto c) { return ""; }},
            {
                "Width",
                [=](auto c) { return printDec(c->surface.width); },
                [=](auto c) { return printHex(c->surface.width); },
            },
            {
                "Height",
                [=](auto c) { return printDec(c->surface.height); },
                [=](auto c) { return printHex(c->surface.height); },
            },
            {
                "Color pitch [0]",
                [=](auto c) { return printDec(c->surface.colorPitch[0]); },
                [=](auto c) { return printHex(c->surface.colorPitch[0]); },
            },
            {
                "Color pitch [1]",
                [=](auto c) { return printDec(c->surface.colorPitch[1]); },
                [=](auto c) { return printHex(c->surface.colorPitch[1]); },
            },
            {
                "Color pitch [2]",
                [=](auto c) { return printDec(c->surface.colorPitch[2]); },
                [=](auto c) { return printHex(c->surface.colorPitch[2]); },
            },
            {
                "Color pitch [3]",
                [=](auto c) { return printDec(c->surface.colorPitch[3]); },
                [=](auto c) { return printHex(c->surface.colorPitch[3]); },
            },
            {
                "Color offset [0]",
                [=](auto c) { return printHex(c->surface.colorOffset[0]); },
                [=](auto c) { return ""; },
            },
            {
                "Color offset [1]",
                [=](auto c) { return printHex(c->surface.colorOffset[1]); },
                [=](auto c) { return ""; },
            },
            {
                "Color offset [2]",
                [=](auto c) { return printHex(c->surface.colorOffset[2]); },
                [=](auto c) { return ""; },
            },
            {
                "Color offset [3]",
                [=](auto c) { return printHex(c->surface.colorOffset[3]); },
                [=](auto c) { return ""; },
            },
            {
                "Depth pitch",
                [=](auto c) { return printHex(c->surface.depthPitch); },
                [=](auto c) { return ""; },
            },
            {
                "Depth offset",
                [=](auto c) { return printHex(c->surface.depthOffset); },
                [=](auto c) { return ""; },
            },
            {
                "Window origin X",
                [=](auto c) { return printHex(c->surface.windowOriginX); },
                [=](auto c) { return ""; },
            },
            {
                "Window origin Y",
                [=](auto c) { return printHex(c->surface.windowOriginY); },
                [=](auto c) { return ""; },
            },
            {
                "Depth type",
                [=](auto c) { return to_string(c->surface.depthType); },
                [=](auto c) { return ""; },
            },
            {
                "Color format",
                [=](auto c) { return to_string(c->surface.colorFormat); },
                [=](auto c) { return ""; },
            },
            {
                "Depth format",
                [=](auto c) { return to_string(c->surface.depthFormat); },
                [=](auto c) { return ""; },
            },
            {
                "Color target",
                [=](auto c) {
                    return sformat("{} {} {} {}",
                                     c->surface.colorTarget[0],
                                     c->surface.colorTarget[1],
                                     c->surface.colorTarget[2],
                                     c->surface.colorTarget[3]);
                },
                [=](auto c) { return ""; },
            },
            {
                "Color mask",
                [=](auto c) { return to_string(c->colorMask); },
                [=](auto c) { return ""; },
            },
            {
                "cullFace",
                [=](auto c) { return to_string(c->cullFace); },
                [=](auto c) { return ""; },
            },
        });
}

SamplerContextTreeItem::SamplerContextTreeItem(bool fragment, int index)
    : ContextTreeItem(sformat("s[{}]", index)),
      _fragment(fragment),
      _index(index) {}

GenericTableModel<RsxContext>* SamplerContextTreeItem::getTable(
    RsxContext* context) {
    auto i = [=, this]() -> TextureSamplerInfo& {
        return _fragment ? context->fragmentTextureSamplers[_index]
                         : context->vertexTextureSamplers[_index];
    };
    return new GenericTableModel<RsxContext>(
        context,
        {
            BOOL(i(), enable)
            FLOAT(i(), minlod)
            FLOAT(i(), maxlod)
            HEX(i(), wraps)
            HEX(i(), wrapt)
            HEX(i(), fragmentWrapr)
            HEX(i(), fragmentZfunc)
            HEX(i(), fragmentAnisoBias)
            FLOAT(i(), bias)
            HEX(i(), fragmentMin)
            HEX(i(), fragmentMag)
            HEX(i(), fragmentConv)
            HEX(i(), fragmentAlphaKill)
            FLOAT(i(), borderColor[0])
            FLOAT(i(), borderColor[1])
            FLOAT(i(), borderColor[2])
            FLOAT(i(), borderColor[3])
        });
}

SamplerTextureContextTreeItem::SamplerTextureContextTreeItem(bool fragment, int index)
    : ContextTreeItem(sformat("t[{}]", index)),
      _fragment(fragment),
      _index(index) {}
      
GenericTableModel<RsxContext>* SamplerTextureContextTreeItem::getTable(
    RsxContext* context) {
    auto t = [=, this]() -> RsxTextureInfo& {
        return _fragment ? context->fragmentTextureSamplers[_index].texture
                         : context->vertexTextureSamplers[_index].texture;
    };
    return new GenericTableModel<RsxContext>(
        context,
        {
            DECHEX(t(), pitch)
            DECHEX(t(), width)
            DECHEX(t(), height)
            HEX(t(), offset)
            DEC(t(), mipmap)
            PRINT_ENUM(t(), format)
            PRINT_ENUM(t(), lnUn)
            DEC(t(), dimension)
            PRINT_ENUM(t(), location)
            BOOL(t(), fragmentBorder)
            BOOL(t(), fragmentCubemap)
            HEX(t(), fragmentDepth)
            HEX(t(), fragmentRemapCrossbarSelect)
            HEX(t(), fragmentGamma)
            HEX(t(), fragmentUnsignedRemap)
            HEX(t(), fragmentSignedRemap)
            HEX(t(), fragmentAs)
            HEX(t(), fragmentRs)
            HEX(t(), fragmentGs)
            HEX(t(), fragmentGs)
        });
}

FragmentOperationsTreeItem::FragmentOperationsTreeItem() : ContextTreeItem("Fragment Operations") {}

GenericTableModel<RsxContext>* FragmentOperationsTreeItem::getTable(RsxContext* context) {
    auto& ops = context->fragmentOps;
    return new GenericTableModel<RsxContext>(
        context,
        {
            BOOL(ops, blend)
            PRINT_ENUM(ops, sfcolor)
            PRINT_ENUM(ops, sfalpha)
            PRINT_ENUM(ops, dfcolor)
            PRINT_ENUM(ops, dfalpha)
            PRINT_ENUM(ops, blendColor)
            PRINT_ENUM(ops, blendAlpha)
            BOOL(ops, logic)
            PRINT_ENUM(ops, logicOp)
            PRINT_ENUM(ops, alphaFunc)
            BOOL(ops, depthTest)
            PRINT_ENUM(ops, depthFunc)
            PRINT_ENUM(ops, stencilFunc)
            HEX(ops, clearColor)
            PRINT_ENUM(ops, clearMask)
        });
}

DisplayBufferContextTreeItem::DisplayBufferContextTreeItem(int index)
    : ContextTreeItem(sformat("{}", index)), _index(index) {}

GenericTableModel<RsxContext>* DisplayBufferContextTreeItem::getTable(RsxContext* context) {
    auto b = [=, this]() -> auto& {
        return context->displayBuffers[_index];
    };
    return new GenericTableModel<RsxContext>(
        context,
        {
            HEX(b(), offset)
            DECHEX(b(), pitch)
            DECHEX(b(), width)
            DECHEX(b(), height)
        });
}


ViewPortContextTreeItem::ViewPortContextTreeItem() : ContextTreeItem("ViewPort") { }

GenericTableModel<RsxContext>* ViewPortContextTreeItem::getTable(RsxContext* context) {
    auto& vp = context->viewPort;
    return new GenericTableModel<RsxContext>(
        context,
        {
            FLOAT(vp, x)
            FLOAT(vp, y)
            FLOAT(vp, width)
            FLOAT(vp, height)
            FLOAT(vp, zmin)
            FLOAT(vp, zmax)
            FLOAT(vp, scale[0])
            FLOAT(vp, scale[1])
            FLOAT(vp, scale[2])
            FLOAT(vp, offset[0])
            FLOAT(vp, offset[1])
            FLOAT(vp, offset[2])
        });
}
