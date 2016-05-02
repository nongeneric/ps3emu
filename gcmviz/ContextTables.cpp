#include "ContextTables.h"
#include "../ps3emu/utils.h"
#include "../ps3emu/rsx/RsxContext.h"

SurfaceContextTreeItem::SurfaceContextTreeItem() : ContextTreeItem("Surface") {}

template <typename T> std::string printHex(T num) {
    return ssnprintf("#%08x", num);
}

template <typename T> std::string printDec(T num) {
    return ssnprintf("%d", num);
}

std::string printLocation(MemoryLocation location) {
    return location == MemoryLocation::Local ? "Local" : "Main";
}

GenericTableModel<RsxContext>* SurfaceContextTreeItem::getTable(
    RsxContext* context) {
    return new GenericTableModel<RsxContext>(
        context,
        {
            {"Depth location",
             [=](auto c) { return printLocation(c->surface.depthLocation); },
             [=](auto c) { return ""; }},
            {"Color location [0]",
             [=](auto c) { return printLocation(c->surface.colorLocation[0]); },
             [=](auto c) { return ""; }},
            {"Color location [1]",
             [=](auto c) { return printLocation(c->surface.colorLocation[1]); },
             [=](auto c) { return ""; }},
            {"Color location [2]",
             [=](auto c) { return printLocation(c->surface.colorLocation[2]); },
             [=](auto c) { return ""; }},
            {"Color location [3]",
             [=](auto c) { return printLocation(c->surface.colorLocation[3]); },
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
                [=](auto c) {
                    return c->surface.depthType == SurfaceDepthType::Fixed ? "Fixed"
                                                                           : "Float";
                },
                [=](auto c) { return ""; },
            },
            {
                "Depth format",
                [=](auto c) {
                    return c->surface.depthFormat == SurfaceDepthFormat::z24s8
                               ? "z24s8"
                               : "z16";
                },
                [=](auto c) { return ""; },
            },
            {
                "Color target",
                [=](auto c) {
                    return ssnprintf("%d %d %d %d",
                                     c->surface.colorTarget[0],
                                     c->surface.colorTarget[1],
                                     c->surface.colorTarget[2],
                                     c->surface.colorTarget[3]);
                },
                [=](auto c) { return ""; },
            },
        });
}