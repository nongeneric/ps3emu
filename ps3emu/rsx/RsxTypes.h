#pragma once

#include "ps3emu/enum.h"

ENUM(GcmBlendEquation,
    (FUNC_ADD, 0x8006),
    (MIN, 0x8007),
    (MAX, 0x8008),
    (FUNC_SUBTRACT, 0x800A),
    (FUNC_REVERSE_SUBTRACT, 0x800B),
    (FUNC_REVERSE_SUBTRACT_SIGNED, 0x0000F005),
    (FUNC_ADD_SIGNED, 0x0000F006),
    (FUNC_REVERSE_ADD_SIGNED, 0x0000F007)
)

ENUM(GcmBlendFunc,
    (ZERO, 0),
    (ONE, 1),
    (SRC_COLOR, 0x0300),
    (ONE_MINUS_SRC_COLOR, 0x0301),
    (SRC_ALPHA, 0x0302),
    (ONE_MINUS_SRC_ALPHA, 0x0303),
    (DST_ALPHA, 0x0304),
    (ONE_MINUS_DST_ALPHA, 0x0305),
    (DST_COLOR, 0x0306),
    (ONE_MINUS_DST_COLOR, 0x0307),
    (SRC_ALPHA_SATURATE, 0x0308),
    (CONSTANT_COLOR, 0x8001),
    (ONE_MINUS_CONSTANT_COLOR, 0x8002),
    (CONSTANT_ALPHA, 0x8003),
    (ONE_MINUS_CONSTANT_ALPHA, 0x8004)
)

ENUM(GcmLogicOp,
    (CLEAR, 0x1500),
    (AND, 0x1501),
    (AND_REVERSE, 0x1502),
    (COPY, 0x1503),
    (AND_INVERTED, 0x1504),
    (NOOP, 0x1505),
    (XOR, 0x1506),
    (OR, 0x1507),
    (NOR, 0x1508),
    (EQUIV, 0x1509),
    (INVERT, 0x150A),
    (OR_REVERSE, 0x150B),
    (COPY_INVERTED, 0x150C),
    (OR_INVERTED, 0x150D),
    (NAND, 0x150E),
    (SET, 0x150F)
)

ENUMF(GcmColorMask,
    (B, 1 << 0),
    (G, 1 << 8),
    (R, 1 << 16),
    (A, 1 << 24)
)

ENUM(GcmCullFace,
    (FRONT, 0x0404),
    (BACK, 0x0405),
    (FRONT_AND_BACK, 0x0408)
)

ENUM(GcmFrontFace,
    (CW, 0x0900),
    (CCW, 0x0901)
)

ENUM(GcmOperator,
    (NEVER, 0x0200),
    (LESS, 0x0201),
    (EQUAL, 0x0202),
    (LEQUAL, 0x0203),
    (GREATER, 0x0204),
    (NOTEQUAL, 0x0205),
    (GEQUAL, 0x0206),
    (ALWAYS, 0x0207)
)

ENUM(GcmPrimitive,
    (NONE, 0),
    (POINTS, 1),
    (LINES, 2),
    (LINE_LOOP, 3),
    (LINE_STRIP, 4),
    (TRIANGLES, 5),
    (TRIANGLE_STRIP, 6),
    (TRIANGLE_FAN, 7),
    (QUADS, 8),
    (QUAD_STRIP, 9),
    (POLYGON, 10)
)

ENUM(GcmDrawIndexArrayType,
    (_32, 0),
    (_16, 1)
)

ENUMF(GcmClearMask,
    (Z, 1<<0),
    (S, 1<<1),
    (R, 1<<4),
    (G, 1<<5),
    (B, 1<<6),
    (A, 1<<7),
    (M, 0xf3)
)

ENUMF(PointSpriteTex,
    (Tex0, (1<<8)),
    (Tex1, (1<<9)),
    (Tex2, (1<<10)),
    (Tex3, (1<<11)),
    (Tex4, (1<<12)),
    (Tex5, (1<<13)),
    (Tex6, (1<<14)),
    (Tex7, (1<<15)),
    (Tex8, (1<<16)),
    (Tex9, (1<<17))
)

ENUMF(InputMask,
    (VDA15, 1 << 0),
    (VDA14, 1 << 1),
    (VDA13, 1 << 2),
    (VDA12, 1 << 3),
    (VDA11, 1 << 4),
    (VDA10, 1 << 5),
    (VDA9, 1 << 6),
    (VDA8, 1 << 7),
    (VDA7, 1 << 8),
    (VDA6, 1 << 9),
    (VDA5, 1 << 10),
    (VDA4, 1 << 11),
    (VDA3, 1 << 12),
    (VDA2, 1 << 13),
    (VDA1, 1 << 14),
    (VDA0, 1 << 15)
)
