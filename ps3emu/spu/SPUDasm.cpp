#include "SPUDasm.h"

#include "SPUThread.h"
#include "../BitField.h"
#include "../dasm_utils.h"
#include <boost/endian/conversion.hpp>
#include <bitset>

using namespace boost::endian;

struct SPUForm {
    BitField<0, 4> OP4;
    BitField<0, 7> OP7;
    BitField<0, 8> OP8;
    BitField<0, 9> OP9;
    BitField<0, 10> OP10;
    BitField<0, 11> OP11;
    BitField<11, 18, BitFieldType::GPR> RB;
    BitField<18, 25, BitFieldType::GPR> RA;
    BitField<25, 32, BitFieldType::GPR> RC;
    BitField<25, 32, BitFieldType::GPR> RT;
    BitField<11, 18, BitFieldType::Signed> I7;
    BitField<10, 18, BitFieldType::Signed> I8;
    BitField<8, 18, BitFieldType::Signed> I10;
    BitField<9, 25, BitFieldType::Signed> I16;
    BitField<7, 25, BitFieldType::Signed> I18;
};

#define PRINT(name) inline void print##name(SPUForm* i, uint32_t cia, std::string* result)
#define EMU(name) inline void emulate##name(SPUForm* i, uint32_t cia, SPUThread* th)
#define INVOKE(name) invoke_impl<M>(#name, print##name, emulate##name, &x, cia, state); return

PRINT(lqd) {
    *result = format_nn("lqd", i->RT, i->RA);
}

EMU(lqd) {
    auto lsa = th->r(i->RA).w<0>() + (i->I10 << 4);
    th->r(i->RT).load(th->ptr(lsa));
}

PRINT(lqx) {
    *result = format_nnn("lqx", i->RT, i->RA, i->RB);
}

EMU(lqx) {
    auto lsa = (th->r(i->RA).w<0>() + th->r(i->RB).w<0>()) & 0xfffffff0;
    th->r(i->RT).load(th->ptr(lsa));
}

PRINT(lqa) {
    *result = format_nn("lqa", i->RT, i->I16);
}

EMU(lqa) {
    auto lsa = i->I16 << 2;
    th->r(i->RT).load(th->ptr(lsa));
}

PRINT(lqr) {
    *result = format_nn("lqr", i->RT, i->I16);
}

EMU(lqr) {
    auto lsa = (i->I16 << 2) + cia;
    th->r(i->RT).load(th->ptr(lsa));
}

PRINT(stqd) {
    *result = format_nnn("lqr", i->I10, i->RA, i->RT);
}

EMU(stqd) {
    auto lsa = th->r(i->RA).w<0>() + (i->I10 << 4);
    th->r(i->RT).store(th->ptr(lsa));
}

PRINT(stqx) {
    *result = format_nnn("stqx", i->RT, i->RA, i->RB);
}

EMU(stqx) {
    auto lsa = (th->r(i->RA).w<0>() + th->r(i->RB).w<0>()) & 0xfffffff0;
    th->r(i->RT).store(th->ptr(lsa));
}

PRINT(stqa) {
    *result = format_nn("stqa", i->RT, i->I16);
}

EMU(stqa) {
    auto lsa = i->I16 << 2;
    th->r(i->RT).store(th->ptr(lsa));
}

PRINT(stqr) {
    *result = format_nn("stqr", i->RT, i->I16);
}

EMU(stqr) {
    auto lsa = (i->I16 << 2) + cia;
    th->r(i->RT).store(th->ptr(lsa));
}

PRINT(cbd) {
    *result = format_nnn("cbd", i->RT, i->I7, i->RA);
}

EMU(cbd) {
    auto t = i->I7.s() + th->r(i->RA).w<0>();
    const uint8_t mask[] { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
                           0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
    th->r(i->RT).load(mask);
    th->r(i->RT).b(t & 0xf) = 0x03;
}

PRINT(cbx) {
    *result = format_nnn("cbx", i->RT, i->RA, i->RB);
}

EMU(cbx) {
    auto t = th->r(i->RA).w<0>() + th->r(i->RB).w<0>();
    const uint8_t mask[] { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
                           0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
    th->r(i->RT).load(mask);
    th->r(i->RT).b(t & 0xf) = 0x03;
}

PRINT(chd) {
    *result = format_nnn("chd", i->RT, i->I7, i->RA);
}

EMU(chd) {
    auto t = i->I7.s() + th->r(i->RA).w<0>();
    const uint8_t mask[] { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
                           0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
    th->r(i->RT).load(mask);
    th->r(i->RT).hw(t & 0xe) = 0x0203;
}

PRINT(chx) {
    *result = format_nnn("chx", i->RT, i->RA, i->RB);
}

EMU(chx) {
    auto t = th->r(i->RA).w<0>() + th->r(i->RB).w<0>();
    const uint8_t mask[] { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
                           0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
    th->r(i->RT).load(mask);
    th->r(i->RT).hw(t & 0xe) = 0x0203;
}

PRINT(cwd) {
    *result = format_nnn("cwd", i->RT, i->I7, i->RA);
}

EMU(cwd) {
    auto t = i->I7.s() + th->r(i->RA).w<0>();
    const uint8_t mask[] { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
                           0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
    th->r(i->RT).load(mask);
    th->r(i->RT).w(t & 0xc) = 0x00010203;
}

PRINT(cwx) {
    *result = format_nnn("cwx", i->RT, i->RA, i->RB);
}

EMU(cwx) {
    auto t = th->r(i->RA).w<0>() + th->r(i->RB).w<0>();
    const uint8_t mask[] { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
                           0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
    th->r(i->RT).load(mask);
    th->r(i->RT).w(t & 0xc) = 0x0203;
}

PRINT(cdd) {
    *result = format_nnn("cdd", i->RT, i->I7, i->RA);
}

EMU(cdd) {
    auto t = i->I7.s() + th->r(i->RA).w<0>();
    const uint8_t mask[] { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
                           0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
    th->r(i->RT).load(mask);
    th->r(i->RT).dw(t & 0x8) = 0x0001020304050607ll;
}

PRINT(cdx) {
    *result = format_nnn("cdx", i->RT, i->RA, i->RB);
}

EMU(cdx) {
    auto t = th->r(i->RA).w<0>() + th->r(i->RB).w<0>();
    const uint8_t mask[] { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
                           0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
    th->r(i->RT).load(mask);
    th->r(i->RT).dw(t & 0x8) = 0x0001020304050607ll;
}

PRINT(ilh) {
    *result = format_nn("ilh", i->RT, i->I16);
}

EMU(ilh) {
    auto& rt = th->r(i->RT);
    auto val = i->I16.s();
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = val;
    }
}

PRINT(ilhu) {
    *result = format_nn("ilhu", i->RT, i->I16);
}

EMU(ilhu) {
    auto& rt = th->r(i->RT);
    auto val = i->I16 << 16;
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = val;
    }
}

PRINT(il) {
    *result = format_nn("il", i->RT, i->I16);
}

EMU(il) {
    auto& rt = th->r(i->RT);
    auto val = i->I16.s();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = val;
    }
}

PRINT(ila) {
    *result = format_nn("ila", i->RT, i->I18);
}

EMU(ila) {
    auto& rt = th->r(i->RT);
    auto val = i->I18.u();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = val;
    }
}

PRINT(iohl) {
    *result = format_nn("iohl", i->RT, i->I16);
}

EMU(iohl) {
    auto& rt = th->r(i->RT);
    auto val = i->I16.u();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) |= val;
    }
}

PRINT(fsmbi) {
    *result = format_nn("fsmbi", i->RT, i->I16);
}

EMU(fsmbi) {
    auto& rt = th->r(i->RT);
    std::bitset<16> bits(i->I16.u());
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = bits[i] ? 0xff : 0;
    }
}

PRINT(ah) {
    *result = format_nnn("ah", i->RT, i->RA, i->RB);
}

EMU(ah) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = ra.hw(i) + rb.hw(i);
    }
}

PRINT(ahi) {
    *result = format_nnn("ahi", i->RT, i->RA, i->I10);
}

EMU(ahi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto s = i->I10.s();
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = ra.hw(i) + s;
    }
}

PRINT(a) {
    *result = format_nnn("a", i->RT, i->RA, i->RB);
}

EMU(a) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.w(i) + rb.w(i);
    }
}

PRINT(ai) {
    *result = format_nnn("ai", i->RT, i->RA, i->I10);
}

EMU(ai) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto s = i->I10.s();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.w(i) + s;
    }
}

PRINT(sfh) {
    *result = format_nnn("sfh", i->RT, i->RA, i->RB);
}

EMU(sfh) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = rb.hw(i) - ra.hw(i);
    }
}

PRINT(sfhi) {
    *result = format_nnn("sfhi", i->RT, i->RA, i->I10);
}

EMU(sfhi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto s = i->I10.s();
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = s - ra.hw(i);
    }
}

PRINT(sf) {
    *result = format_nnn("sf", i->RT, i->RA, i->RB);
}

EMU(sf) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = rb.w(i) - ra.w(i);
    }
}

PRINT(sfi) {
    *result = format_nnn("sfi", i->RT, i->RA, i->I10);
}

EMU(sfi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto s = i->I10.s();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = s - ra.w(i);
    }
}

PRINT(addx) {
    *result = format_nnn("addx", i->RT, i->RA, i->RB);
}

EMU(addx) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = rb.w(i) + ra.w(i) + (rt.w(i) & 1);
    }
}

PRINT(cg) {
    *result = format_nnn("cg", i->RT, i->RA, i->RB);
}

EMU(cg) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        uint64_t t = (int64_t)rb.w(i) + ra.w(i);
        rt.w(i) = t >> 32;
    }
}

PRINT(cgx) {
    *result = format_nnn("cg", i->RT, i->RA, i->RB);
}

EMU(cgx) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        uint64_t t = (int64_t)rb.w(i) + ra.w(i) + (rt.w(i) & 1);
        rt.w(i) = t >> 32;
    }
}

PRINT(sfx) {
    *result = format_nnn("sfx", i->RT, i->RA, i->RB);
}

EMU(sfx) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = rb.w(i) + ~ra.w(i) + (rt.w(i) & 1);
    }
}

PRINT(bg) {
    *result = format_nnn("bg", i->RT, i->RA, i->RB);
}

EMU(bg) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = (uint32_t)ra.w(i) <= (uint32_t)rb.w(i);
    }
}

PRINT(bgx) {
    *result = format_nnn("bg", i->RT, i->RA, i->RB);
}

EMU(bgx) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        auto t = rb.w(i) + ~ra.w(i) + (rt.w(i) & 1);
        rt.w(i) = t >= 0;
    }
}

PRINT(mpy) {
    *result = format_nnn("mpy", i->RT, i->RA, i->RB);
}

EMU(mpy) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        int32_t t = (int16_t)ra.w(i);
        t *= (int16_t)rb.w(i);
        rt.w(i) = t;
    }
}

PRINT(mpyu) {
    *result = format_nnn("mpyu", i->RT, i->RA, i->RB);
}

EMU(mpyu) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ((uint32_t)ra.w(i) & 0xffff) * ((uint32_t)rb.w(i) & 0xffff);
    }
}

PRINT(mpyi) {
    *result = format_nnn("mpyi", i->RT, i->RA, i->I10);
}

EMU(mpyi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto s = i->I10.s();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = s * (int16_t)ra.w(i);
    }
}

PRINT(mpyui) {
    *result = format_nnn("mpyui", i->RT, i->RA, i->I10);
}

EMU(mpyui) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    uint32_t s = i->I10.s();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ((uint32_t)ra.w(i) & 0xffff) * s;
    }
}

PRINT(mpya) {
    *result = format_nnnn("mpya", i->RT, i->RA, i->RB, i->RC);
}

EMU(mpya) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rc = th->r(i->RC);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        int32_t t = (int16_t)ra.w(i);
        t *= (int16_t)rb.w(i);
        rt.w(i) = t + rc.w(i);
    }
}

PRINT(mpyh) {
    *result = format_nnn("mpyh", i->RT, i->RA, i->RB);
}

EMU(mpyh) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        int32_t t = (int16_t)signed_rshift32(ra.w(i), 16);
        t *= (int16_t)rb.w(i);
        rt.w(i) = signed_lshift32(t, 16);
    }
}

PRINT(mpys) {
    *result = format_nnn("mpys", i->RT, i->RA, i->RB);
}

EMU(mpys) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        int32_t t = (int16_t)ra.w(i);
        t *= (int16_t)rb.w(i);
        rt.w(i) = signed_rshift32(t, 16);
    }
}

PRINT(mpyhh) {
    *result = format_nnn("mpyhh", i->RT, i->RA, i->RB);
}

EMU(mpyhh) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        int32_t t = (int16_t)signed_rshift32(ra.w(i), 16);
        t *= (int16_t)signed_rshift32(rb.w(i), 16);
        rt.w(i) = t;
    }
}

PRINT(mpyhha) {
    *result = format_nnn("mpyhha", i->RT, i->RA, i->RB);
}

EMU(mpyhha) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        int32_t t = (int16_t)signed_rshift32(ra.w(i), 16);
        t *= (int16_t)signed_rshift32(rb.w(i), 16);
        rt.w(i) += t;
    }
}

PRINT(mpyhhu) {
    *result = format_nnn("mpyhhu", i->RT, i->RA, i->RB);
}

EMU(mpyhhu) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ((uint32_t)ra.w(i) >> 16) * ((uint32_t)rb.w(i) >> 16);
    }
}

PRINT(mpyhhau) {
    *result = format_nnn("mpyhhau", i->RT, i->RA, i->RB);
}

EMU(mpyhhau) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) += ((uint32_t)ra.w(i) >> 16) * ((uint32_t)rb.w(i) >> 16);
    }
}

PRINT(clz) {
    *result = format_nn("clz", i->RT, i->RA);
}

EMU(clz) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.w(i) == 0 ? 32 : __builtin_clzll(ra.w(i));
    }
}

PRINT(cntb) {
    *result = format_nn("cntb", i->RT, i->RA);
}

EMU(cntb) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = count_ones32(ra.b(i));
    }
}

PRINT(fsmb) {
    *result = format_nn("fsmb", i->RT, i->RA);
}

EMU(fsmb) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    std::bitset<16> bits(ra.w<0>());
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = bits[i] ? 0xff : 0;
    }
}

PRINT(fsmh) {
    *result = format_nn("fsmh", i->RT, i->RA);
}

EMU(fsmh) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    std::bitset<8> bits(ra.w<0>());
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = bits[i] ? 0xffff : 0;
    }
}

PRINT(fsm) {
    *result = format_nn("fsm", i->RT, i->RA);
}

EMU(fsm) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    std::bitset<4> bits(ra.w<0>());
    for (int i = 0; i < 4; ++i) {
        rt.hw(i) = bits[i] ? 0xffffffff : 0;
    }
}

PRINT(gbb) {
    *result = format_nn("gbb", i->RT, i->RA);
}

EMU(gbb) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    std::bitset<16> bits;
    for (int i = 0; i < 16; ++i) {
        bits[i] = ra.b(i) & 1;
    }
    rt.w<0>() = bits.to_ulong();
    rt.w<1>() = 0;
    rt.w<2>() = 0;
    rt.w<3>() = 0;
}

PRINT(gbh) {
    *result = format_nn("gbh", i->RT, i->RA);
}

EMU(gbh) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    std::bitset<8> bits;
    for (int i = 0; i < 8; ++i) {
        bits[i] = ra.hw(i) & 1;
    }
    rt.w<0>() = bits.to_ulong();
    rt.w<1>() = 0;
    rt.w<2>() = 0;
    rt.w<3>() = 0;
}

PRINT(gb) {
    *result = format_nn("gb", i->RT, i->RA);
}

EMU(gb) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    std::bitset<4> bits;
    for (int i = 0; i < 4; ++i) {
        bits[i] = ra.w(i) & 1;
    }
    rt.w<0>() = bits.to_ulong();
    rt.w<1>() = 0;
    rt.w<2>() = 0;
    rt.w<3>() = 0;
}

PRINT(avgb) {
    *result = format_nnn("avgb", i->RT, i->RA, i->RB);
}

EMU(avgb) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 16; ++i) {
        auto t = (uint16_t)ra.b(i) + rb.b(i) + 1;
        rt.b(i) = signed_rshift32(t, 1);
    }
}

PRINT(absdb) {
    *result = format_nnn("absdb", i->RT, i->RA, i->RB);
}

EMU(absdb) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = rb.b(i) - ra.b(i);
    }
}

PRINT(sumb) {
    *result = format_nnn("sumb", i->RT, i->RA, i->RB);
}

EMU(sumb) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        auto bsum = (uint16_t)
                    rb.b(4 * i + 0)
                  + rb.b(4 * i + 1)
                  + rb.b(4 * i + 2)
                  + rb.b(4 * i + 3);
        auto asum = (uint16_t)
                    ra.b(4 * i + 0)
                  + ra.b(4 * i + 1)
                  + ra.b(4 * i + 2)
                  + ra.b(4 * i + 3);
        rt.w(i) = (bsum << 16) | asum;
    }
}

PRINT(xsbh) {
    *result = format_nn("xsbh", i->RT, i->RA);
}

EMU(xsbh) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = (int8_t)ra.b(2 * i + 1);
    }
}

PRINT(xshw) {
    *result = format_nn("xshw", i->RT, i->RA);
}

EMU(xshw) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.hw(2 * i + 1);
    }
}

PRINT(xswd) {
    *result = format_nn("xswd", i->RT, i->RA);
}

EMU(xswd) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.dw(i) = ra.w(2 * i + 1);
    }
}

PRINT(and_) {
    *result = format_nnn("and", i->RT, i->RA, i->RB);
}

EMU(and_) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.dw(i) = rb.dw(i) & ra.dw(i);
    }
}

PRINT(andc) {
    *result = format_nnn("andc", i->RT, i->RA, i->RB);
}

EMU(andc) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.dw(i) = rb.dw(i) & ~ra.dw(i);
    }
}

PRINT(andbi) {
    *result = format_nnn("andbi", i->RT, i->RA, i->I10);
}

EMU(andbi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto b = i->I10.u() & 0xff;
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = ra.b(i) & b;
    }
}

PRINT(andhi) {
    *result = format_nnn("andhi", i->RT, i->RA, i->I10);
}

EMU(andhi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    int16_t t = i->I10.s();
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = ra.hw(i) & t;
    }
}

PRINT(andi) {
    *result = format_nnn("andi", i->RT, i->RA, i->I10);
}

EMU(andi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    int32_t t = i->I10.s();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.w(i) & t;
    }
}

PRINT(or_) {
    *result = format_nnn("or", i->RT, i->RA, i->RB);
}

EMU(or_) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.dw(i) = rb.dw(i) | ra.dw(i);
    }
}

PRINT(orc) {
    *result = format_nnn("orc", i->RT, i->RA, i->RB);
}

EMU(orc) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.dw(i) = rb.dw(i) | ~ra.dw(i);
    }
}

PRINT(orbi) {
    *result = format_nnn("orbi", i->RT, i->RA, i->I10);
}

EMU(orbi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto b = i->I10.u() & 0xff;
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = ra.b(i) | b;
    }
}

PRINT(orhi) {
    *result = format_nnn("orhi", i->RT, i->RA, i->I10);
}

EMU(orhi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    int16_t t = i->I10.s();
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = ra.hw(i) | t;
    }
}

PRINT(ori) {
    *result = format_nnn("ori", i->RT, i->RA, i->I10);
}

EMU(ori) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    int32_t t = i->I10.s();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.w(i) | t;
    }
}

PRINT(orx) {
    *result = format_nn("orx", i->RT, i->RA);
}

EMU(orx) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    rt.w<0>() = ra.w<0>() | ra.w<1>() | ra.w<2>() | ra.w<3>();
    rt.w<1>() = 0;
    rt.dw<1>() = 0;
}

PRINT(xor_) {
    *result = format_nnn("xor", i->RT, i->RA, i->RB);
}

EMU(xor_) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.dw(i) = rb.dw(i) ^ ra.dw(i);
    }
}

PRINT(xorbi) {
    *result = format_nnn("xorbi", i->RT, i->RA, i->I10);
}

EMU(xorbi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto b = i->I10.u() & 0xff;
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = ra.b(i) ^ b;
    }
}

PRINT(xorhi) {
    *result = format_nnn("xorhi", i->RT, i->RA, i->I10);
}

EMU(xorhi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    int16_t t = i->I10.s();
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = ra.hw(i) ^ t;
    }
}

PRINT(xori) {
    *result = format_nnn("xori", i->RT, i->RA, i->I10);
}

EMU(xori) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    int32_t t = i->I10.s();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.w(i) ^ t;
    }
}

PRINT(nand) {
    *result = format_nnn("nand", i->RT, i->RA, i->RB);
}

EMU(nand) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ~(rb.w(i) & ra.w(i));
    }
}

PRINT(nor) {
    *result = format_nnn("nor", i->RT, i->RA, i->RB);
}

EMU(nor) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ~(rb.w(i) | ra.w(i));
    }
}

PRINT(eqv) {
    *result = format_nnn("eqv", i->RT, i->RA, i->RB);
}

EMU(eqv) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = rb.w(i) ^ ~ra.w(i);
    }
}

PRINT(selb) {
    *result = format_nnnn("selb", i->RT, i->RA, i->RB, i->RC);
}

EMU(selb) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rc = th->r(i->RC);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.dw(i) = (rc.dw(i) & rb.dw(i)) | (~rc.dw(i) & ra.dw(i));
    }
}

PRINT(shufb) {
    *result = format_nnnn("shufb", i->RT, i->RA, i->RB, i->RC);
}

EMU(shufb) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rc = th->r(i->RC);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 16; ++i) {
        auto c = rc.b(i);
        auto idx = c & 0b11111;
        rt.b(i) = c >> 6 == 0b10 ? 0
                : c >> 5 == 0b110 ? 0xff
                : c >> 5 == 0b111 ? 0x80
                : (idx < 16 ? rb.b(idx) : ra.b(idx % 16));
    }
}

PRINT(shlh) {
    *result = format_nnn("shlh", i->RT, i->RA, i->RB);
}

EMU(shlh) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 8; ++i) {
        auto sh = (uint16_t)rb.hw(i) & 0b11111;
        rt.hw(i) = sh > 15 ? 0 : signed_lshift32(ra.hw(i), sh);
    }
}

PRINT(shlhi) {
    *result = format_nnn("shlhi", i->RT, i->RA, i->I7);
}

EMU(shlhi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto sh = i->I7.u() & 0b11111;
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = sh > 15 ? 0 : signed_lshift32(ra.hw(i), sh);
    }
}

PRINT(shl) {
    *result = format_nnn("shl", i->RT, i->RA, i->RB);
}

EMU(shl) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        auto sh = (uint32_t)rb.w(i) & 0b111111;
        rt.w(i) = sh > 31 ? 0 : signed_lshift32(ra.w(i), sh);
    }
}

PRINT(shli) {
    *result = format_nnn("shli", i->RT, i->RA, i->I7);
}

EMU(shli) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto sh = i->I7.u() & 0b111111;
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = sh > 31 ? 0 : signed_lshift32(ra.w(i), sh);
    }
}

PRINT(shlqbi) {
    *result = format_nnn("shlqbi", i->RT, i->RA, i->RB);
}

EMU(shlqbi) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    u128 <<= (uint32_t)rb.w<0>() & 0b111;
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(shlqbii) {
    *result = format_nnn("shlqbii", i->RT, i->RA, i->I7);
}

EMU(shlqbii) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    u128 <<= i->I7.u() & 0b111;
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(shlqby) {
    *result = format_nnn("shlqby", i->RT, i->RA, i->RB);
}

EMU(shlqby) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    auto sh = (uint32_t)rb.w<0>() & 0b11111;
    u128 = sh > 15 ? 0 : u128 << sh * 8;
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(shlqbyi) {
    *result = format_nnn("shlqbyi", i->RT, i->RA, i->I7);
}

EMU(shlqbyi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    auto sh = i->I7.u() & 0b11111;
    u128 = sh > 15 ? 0 : u128 << sh * 8;
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(shlqbybi) {
    *result = format_nnn("shlqbybi", i->RT, i->RA, i->RB);
}

EMU(shlqbybi) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    auto sh = ((uint32_t)rb.w<0>() >> 3) & 0b11111;
    u128 = sh > 15 ? 0 : u128 << sh * 8;
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(roth) {
    *result = format_nnn("roth", i->RT, i->RA, i->RB);
}

EMU(roth) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 8; ++i) {
        auto sh = (uint16_t)rb.hw(i) & 0b1111;
        rt.hw(i) = rol<16>(ra.hw(i), sh);
    }
}

PRINT(rothi) {
    *result = format_nnn("rothi", i->RT, i->RA, i->I7);
}

EMU(rothi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto sh = i->I7.u() & 0b1111;
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = rol<16>(ra.hw(i), sh);
    }
}

PRINT(rot) {
    *result = format_nnn("rot", i->RT, i->RA, i->RB);
}

EMU(rot) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        auto sh = (uint32_t)rb.w(i) & 0b11111;
        rt.w(i) = rol<32>(ra.w(i), sh);
    }
}

PRINT(roti) {
    *result = format_nnn("roti", i->RT, i->RA, i->I7);
}

EMU(roti) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto sh = i->I7.u() & 0b11111;
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = rol<16>(ra.w(i), sh);
    }
}

PRINT(rotqby) {
    *result = format_nnn("rotqby", i->RT, i->RA, i->RB);
}

EMU(rotqby) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    auto sh = (uint32_t)rb.w<0>() & 0b1111;
    u128 = rol<128, uint128_t>(u128, sh * 8);
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(rotqbyi) {
    *result = format_nnn("rotqbyi", i->RT, i->RA, i->I7);
}

EMU(rotqbyi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    auto sh = i->I7.u() & 0b1111;
    u128 = rol<128, uint128_t>(u128, sh * 8);
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(rotqbybi) {
    *result = format_nnn("rotqbybi", i->RT, i->RA, i->RB);
}

EMU(rotqbybi) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    auto sh = ((uint32_t)rb.w<0>() >> 3) & 0b11111;
    u128 = rol<128, uint128_t>(u128, sh * 8);
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(rotqbi) {
    *result = format_nnn("rotqbi", i->RT, i->RA, i->RB);
}

EMU(rotqbi) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    auto sh = (uint32_t)rb.w<0>() & 0b111;
    u128 = rol<128, uint128_t>(u128, sh);
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(rotqbii) {
    *result = format_nnn("rotqbii", i->RT, i->RA, i->I7);
}

EMU(rotqbii) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    auto sh = i->I7.u() & 0b111;
    u128 = rol<128, uint128_t>(u128, sh);
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(rothm) {
    *result = format_nnn("rothm", i->RT, i->RA, i->RB);
}

EMU(rothm) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 8; ++i) {
        auto sh = -rb.hw(i) & 0x1f;
        rt.hw(i) = sh > 15 ? 0 : (uint32_t)ra.hw(i) >> sh;
    }
}

PRINT(rothmi) {
    *result = format_nnn("rothmi", i->RT, i->RA, i->I7);
}

EMU(rothmi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto sh = -i->I7.s() & 0x1f;
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = sh > 15 ? 0 : (uint16_t)ra.hw(i) >> sh;
    }
}

PRINT(rotm) {
    *result = format_nnn("rotm", i->RT, i->RA, i->RB);
}

EMU(rotm) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        auto sh = -rb.w(i) & 0x3f;
        rt.w(i) = sh > 31 ? 0 : (uint32_t)ra.w(i) >> sh;
    }
}

PRINT(rotmi) {
    *result = format_nnn("rotmi", i->RT, i->RA, i->I7);
}

EMU(rotmi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto sh = -i->I7.s() & 0x3f;
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = sh > 31 ? 0 : (uint32_t)ra.w(i) >> sh;
    }
}

PRINT(rotqmby) {
    *result = format_nnn("rotqmby", i->RT, i->RA, i->RB);
}

EMU(rotqmby) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    auto sh = -rb.w<0>() & 0x1f;
    u128 = sh > 15 ? 0 : u128 >> sh * 8;
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(rotqmbyi) {
    *result = format_nnn("rotqmbyi", i->RT, i->RA, i->I7);
}

EMU(rotqmbyi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    auto sh = -i->I7.s() & 0x1f;
    u128 = sh > 15 ? 0 : u128 >> sh * 8;
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(rotqmbybi) {
    *result = format_nnn("rotqmbybi", i->RT, i->RA, i->RB);
}

EMU(rotqmbybi) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    auto sh = ((uint32_t)rb.w<0>() >> 3) & 0b11111;
    u128 = sh > 15 ? 0 : u128 >> sh * 8;
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(rotqmbi) {
    *result = format_nnn("rotqmbi", i->RT, i->RA, i->RB);
}

EMU(rotqmbi) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    u128 >>= -rb.w<0>() & 7;
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(rotqmbii) {
    *result = format_nnn("rotqmbii", i->RT, i->RA, i->I7);
}

EMU(rotqmbii) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto u128 = make128(ra.dw<1>(), ra.dw<0>());
    u128 >>= -i->I7.s() & 7;
    rt.dw<0>() = u128 >> 64;
    rt.dw<1>() = u128;
}

PRINT(rotmah) {
    *result = format_nnn("rotmah", i->RT, i->RA, i->RB);
}

EMU(rotmah) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 8; ++i) {
        auto sh = -rb.hw(i) & 0x1f;
        rt.hw(i) = sh > 15 ? 0 : signed_rshift32(ra.hw(i), sh);
    }
}

PRINT(rotmahi) {
    *result = format_nnn("rotmahi", i->RT, i->RA, i->I7);
}

EMU(rotmahi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto sh = -i->I7.s() & 0x1f;
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = sh > 15 ? 0 : signed_rshift32(ra.hw(i), sh);
    }
}

PRINT(rotma) {
    *result = format_nnn("rotma", i->RT, i->RA, i->RB);
}

EMU(rotma) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        auto sh = -rb.w(i) & 0x3f;
        rt.w(i) = sh > 31 ? 0 : signed_rshift32(ra.w(i), sh);
    }
}

PRINT(rotmai) {
    *result = format_nnn("rotmai", i->RT, i->RA, i->I7);
}

EMU(rotmai) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto sh = -i->I7.s() & 0x3f;
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = sh > 31 ? 0 : signed_rshift32(ra.w(i), sh);
    }
}

PRINT(heq) {
    *result = format_nnn("heq", i->RT, i->RA, i->RB);
}

EMU(heq) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    if (ra.w<0>() == rb.w<0>())
        throw BreakpointException();
}

PRINT(heqi) {
    *result = format_nnn("heqi", i->RT, i->RA, i->I10);
}

EMU(heqi) {
    auto& ra = th->r(i->RA);
    if (ra.w<0>() == i->I10.s())
        throw BreakpointException();
}

PRINT(hgt) {
    *result = format_nnn("hgt", i->RT, i->RA, i->RB);
}

EMU(hgt) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    if (ra.w<0>() > rb.w<0>())
        throw BreakpointException();
}

PRINT(hgti) {
    *result = format_nnn("hgti", i->RT, i->RA, i->I10);
}

EMU(hgti) {
    auto& ra = th->r(i->RA);
    if (ra.w<0>() > i->I10.s())
        throw BreakpointException();
}

PRINT(hlgt) {
    *result = format_nnn("hlgt", i->RT, i->RA, i->RB);
}

EMU(hlgt) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    if ((uint32_t)ra.w<0>() > (uint32_t)rb.w<0>())
        throw BreakpointException();
}

PRINT(hlgti) {
    *result = format_nnn("hlgti", i->RT, i->RA, i->I10);
}

EMU(hlgti) {
    auto& ra = th->r(i->RA);
    if ((uint32_t)ra.w<0>() > (uint32_t)i->I10.s())
        throw BreakpointException();
}

PRINT(ceqb) {
    *result = format_nnn("ceqb", i->RT, i->RA, i->RB);
}

EMU(ceqb) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = ra.b(i) == rb.b(i) ? 0xff : 0;
    }
}

PRINT(ceqbi) {
    *result = format_nnn("ceqbi", i->RT, i->RA, i->I10);
}

EMU(ceqbi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    uint8_t imm = i->I10.u();
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = ra.b(i) == imm ? 0xff : 0;
    }
}

PRINT(ceqh) {
    *result = format_nnn("ceqh", i->RT, i->RA, i->RB);
}

EMU(ceqh) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = ra.hw(i) == rb.hw(i) ? 0xffff : 0;
    }
}

PRINT(ceqhi) {
    *result = format_nnn("ceqhi", i->RT, i->RA, i->I10);
}

EMU(ceqhi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    int16_t imm = i->I10.s();
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = ra.hw(i) == imm ? 0xffff : 0;
    }
}

PRINT(ceq) {
    *result = format_nnn("ceq", i->RT, i->RA, i->RB);
}

EMU(ceq) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.w(i) == rb.w(i) ? 0xffffffff : 0;
    }
}

PRINT(ceqi) {
    *result = format_nnn("ceqi", i->RT, i->RA, i->I10);
}

EMU(ceqi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    int32_t imm = i->I10.s();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.w(i) == imm ? 0xffffffff : 0;
    }
}

PRINT(cgtb) {
    *result = format_nnn("cgtb", i->RT, i->RA, i->RB);
}

EMU(cgtb) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = (int8_t)ra.b(i) > (int8_t)rb.b(i) ? 0xff : 0;
    }
}

PRINT(cgtbi) {
    *result = format_nnn("cgtbi", i->RT, i->RA, i->I10);
}

EMU(cgtbi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    int8_t imm = i->I10.u() & 0xff;
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = (int8_t)ra.b(i) > imm ? 0xff : 0;
    }
}

PRINT(cgth) {
    *result = format_nnn("cgth", i->RT, i->RA, i->RB);
}

EMU(cgth) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = ra.hw(i) > rb.hw(i) ? 0xffff : 0;
    }
}

PRINT(cgthi) {
    *result = format_nnn("cgthi", i->RT, i->RA, i->I10);
}

EMU(cgthi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    int16_t imm = i->I10.s();
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = ra.hw(i) > imm ? 0xffff : 0;
    }
}

PRINT(cgt) {
    *result = format_nnn("cgt", i->RT, i->RA, i->RB);
}

EMU(cgt) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.w(i) > rb.w(i) ? 0xffffffff : 0;
    }
}

PRINT(cgti) {
    *result = format_nnn("cgti", i->RT, i->RA, i->I10);
}

EMU(cgti) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    int32_t imm = i->I10.s();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.w(i) > imm ? 0xffffffff : 0;
    }
}

PRINT(clgtb) {
    *result = format_nnn("clgtb", i->RT, i->RA, i->RB);
}

EMU(clgtb) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = ra.b(i) > rb.b(i) ? 0xff : 0;
    }
}

PRINT(clgtbi) {
    *result = format_nnn("cgtbi", i->RT, i->RA, i->I10);
}

EMU(clgtbi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    uint8_t imm = i->I10.u();
    for (int i = 0; i < 16; ++i) {
        rt.b(i) = ra.b(i) > imm ? 0xff : 0;
    }
}

PRINT(clgth) {
    *result = format_nnn("clgth", i->RT, i->RA, i->RB);
}

EMU(clgth) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = (uint16_t)ra.hw(i) > (uint16_t)rb.hw(i) ? 0xffff : 0;
    }
}

PRINT(clgthi) {
    *result = format_nnn("clgthi", i->RT, i->RA, i->I10);
}

EMU(clgthi) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    uint16_t imm = i->I10.u();
    for (int i = 0; i < 8; ++i) {
        rt.hw(i) = (uint16_t)ra.hw(i) > imm ? 0xffff : 0;
    }
}

PRINT(clgt) {
    *result = format_nnn("clgt", i->RT, i->RA, i->RB);
}

EMU(clgt) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = (uint32_t)ra.w(i) > (uint32_t)rb.w(i) ? 0xffffffff : 0;
    }
}

PRINT(clgti) {
    *result = format_nnn("clgti", i->RT, i->RA, i->I10);
}

EMU(clgti) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    uint32_t imm = i->I10.u();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = (uint32_t)ra.w(i) > imm ? 0xffffffff : 0;
    }
}

PRINT(br) {
    int32_t offset = signed_lshift32(i->I16.s(), 2);
    *result = format_u("br", (cia + offset) & LSLR);
}

EMU(br) {
    int32_t offset = signed_lshift32(i->I16.s(), 2);
    th->setNip( (cia + offset) & LSLR );
}

PRINT(bra) {
    int32_t address = signed_lshift32(i->I16.s(), 2);
    *result = format_u("bra", address & LSLR);
}

EMU(bra) {
    int32_t address = signed_lshift32(i->I16.s(), 2);
    th->setNip( address & LSLR );
}

PRINT(brsl) {
    int32_t offset = signed_lshift32(i->I16.s(), 2);
    *result = format_u("brsl", (cia + offset) & LSLR);
}

EMU(brsl) {
    int32_t offset = signed_lshift32(i->I16.s(), 2);
    th->setNip( (cia + offset) & LSLR );
    auto& rt = th->r(i->RT);
    rt.w<0>() = (cia + 4) & LSLR;
    rt.w<1>() = 0;
    rt.dw<1>() = 0;
}

PRINT(brasl) {
    int32_t address = signed_lshift32(i->I16.s(), 2);
    *result = format_u("brasl", address & LSLR);
}

EMU(brasl) {
    int32_t address = signed_lshift32(i->I16.s(), 2);
    th->setNip( address & LSLR );
    auto& rt = th->r(i->RT);
    rt.w<0>() = (cia + 4) & LSLR;
    rt.w<1>() = 0;
    rt.dw<1>() = 0;
}

PRINT(bi) {
    *result = format_n("bi", i->RA);
}

EMU(bi) {
    th->setNip( th->r(i->RA).w<0>() & LSLR & 0xfffffffc );
}

PRINT(iret) {
    *result = format_n("iret", i->RA);
}

EMU(iret) {
    th->setNip( th->getSrr0() );
}

PRINT(bisl) {
    *result = format_nn("bisl", i->RT, i->RA);
}

EMU(bisl) {
    th->setNip( th->r(i->RA).w<0>() & LSLR & 0xfffffffc );
    auto& rt = th->r(i->RT);
    rt.w<0>() = LSLR & (cia + 4);
    rt.w<1>() = 0;
    rt.dw<1>() = 0;
}

PRINT(brnz) {
    *result = format_nn("brnz", i->RT, i->I16);
}

EMU(brnz) {
    if (th->r(i->RT).w<0>() != 0) {
        auto address = signed_lshift32(i->I16.s(), 2) & LSLR & 0xfffffffc;
        th->setNip(address);
    }
}

PRINT(brz) {
    *result = format_nn("brz", i->RT, i->I16);
}

EMU(brz) {
    if (th->r(i->RT).w<0>() == 0) {
        auto address = signed_lshift32(i->I16.s(), 2) & LSLR & 0xfffffffc;
        th->setNip(address);
    }
}

PRINT(brhnz) {
    *result = format_nn("brhnz", i->RT, i->I16);
}

EMU(brhnz) {
    if (th->r(i->RT).hw<0>() != 0) {
        auto address = signed_lshift32(i->I16.s(), 2) & LSLR & 0xfffffffc;
        th->setNip(address);
    }
}

PRINT(brhz) {
    *result = format_nn("brhz", i->RT, i->I16);
}

EMU(brhz) {
    if (th->r(i->RT).hw<0>() == 0) {
        auto address = signed_lshift32(i->I16.s(), 2) & LSLR & 0xfffffffc;
        th->setNip(address);
    }
}

PRINT(biz) {
    *result = format_nn("biz", i->RT, i->RA);
}

EMU(biz) {
    if (th->r(i->RT).w<0>() == 0) {
        auto address = th->r(i->RA).w<0>() & LSLR & 0xfffffffc;
        th->setNip(address);
    }
}

PRINT(binz) {
    *result = format_nn("binz", i->RT, i->RA);
}

EMU(binz) {
    if (th->r(i->RT).w<0>() != 0) {
        auto address = th->r(i->RA).w<0>() & LSLR & 0xfffffffc;
        th->setNip(address);
    }
}

PRINT(bihz) {
    *result = format_nn("bihz", i->RT, i->RA);
}

EMU(bihz) {
    if (th->r(i->RT).hw<0>() == 0) {
        auto address = th->r(i->RA).w<0>() & LSLR & 0xfffffffc;
        th->setNip(address);
    }
}

PRINT(bihnz) {
    *result = format_nn("bihnz", i->RT, i->RA);
}

EMU(bihnz) {
    if (th->r(i->RT).hw<0>() != 0) {
        auto address = th->r(i->RA).w<0>() & LSLR & 0xfffffffc;
        th->setNip(address);
    }
}

PRINT(hbr) {
    *result = "hbr";
}

EMU(hbr) { }

PRINT(hbra) {
    *result = "hbra";
}

EMU(hbra) { }

PRINT(hbrr) {
    *result = "hbrr";
}

EMU(hbrr) { }

PRINT(fa) {
    *result = format_nnn("fa", i->RT, i->RA, i->RB);
}

EMU(fa) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.fs(i) = ra.fs(i) + rb.fs(i);
    }
}

PRINT(dfa) {
    *result = format_nnn("dfa", i->RT, i->RA, i->RB);
}

EMU(dfa) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.fd(i) = ra.fd(i) + rb.fd(i);
    }
}

PRINT(fs) {
    *result = format_nnn("fs", i->RT, i->RA, i->RB);
}

EMU(fs) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.fs(i) = ra.fs(i) - rb.fs(i);
    }
}

PRINT(dfs) {
    *result = format_nnn("dfs", i->RT, i->RA, i->RB);
}

EMU(dfs) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.fd(i) = ra.fd(i) - rb.fd(i);
    }
}

PRINT(fm) {
    *result = format_nnn("fm", i->RT, i->RA, i->RB);
}

EMU(fm) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.fs(i) = ra.fs(i) * rb.fs(i);
    }
}

PRINT(dfm) {
    *result = format_nnn("dfm", i->RT, i->RA, i->RB);
}

EMU(dfm) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.fd(i) = ra.fd(i) * rb.fd(i);
    }
}

PRINT(fma) {
    *result = format_nnnn("fma", i->RT, i->RA, i->RB, i->RC);
}

EMU(fma) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rc = th->r(i->RC);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.fs(i) = ra.fs(i) * rb.fs(i) + rc.fs(i);
    }
}

PRINT(dfma) {
    *result = format_nnnn("dfma", i->RT, i->RA, i->RB, i->RC);
}

EMU(dfma) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rc = th->r(i->RC);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.fd(i) = ra.fd(i) * rb.fd(i) + rc.fd(i);
    }
}

PRINT(fnms) {
    *result = format_nnnn("fnms", i->RT, i->RA, i->RB, i->RC);
}

EMU(fnms) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rc = th->r(i->RC);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.fs(i) = rc.fs(i) - ra.fs(i) * rb.fs(i);
    }
}

PRINT(dfnms) {
    *result = format_nnnn("dfnms", i->RT, i->RA, i->RB, i->RC);
}

EMU(dfnms) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rc = th->r(i->RC);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.fd(i) = rc.fd(i) - ra.fd(i) * rb.fd(i);
    }
}

PRINT(fms) {
    *result = format_nnnn("fms", i->RT, i->RA, i->RB, i->RC);
}

EMU(fms) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rc = th->r(i->RC);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.fs(i) = ra.fs(i) * rb.fs(i) - rc.fs(i);
    }
}

PRINT(dfms) {
    *result = format_nnnn("dfms", i->RT, i->RA, i->RB, i->RC);
}

EMU(dfms) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rc = th->r(i->RC);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.fd(i) = ra.fd(i) * rb.fd(i) - rc.fd(i);
    }
}

PRINT(dfnma) {
    *result = format_nnn("dfnma", i->RT, i->RA, i->RB);
}

EMU(dfnma) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.fd(i) = -(ra.fd(i) * rb.fd(i) + rt.fd(i));
    }
}

PRINT(frest) {
    *result = format_nn("frest", i->RT, i->RA);
}

EMU(frest) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.fs(i) = 1.f / ra.fs(i);
    }
}

PRINT(frsqest) {
    *result = format_nn("frsqest", i->RT, i->RA);
}

EMU(frsqest) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.fs(i) = 1.f / std::sqrt(std::abs(ra.fs(i)));
    }
}

PRINT(fi) {
    *result = format_nnn("fi", i->RT, i->RA, i->RB);
}

EMU(fi) {
    // skip Newton-Raphson's second step
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.fs(i) = rb.fs(i);
    }
}

PRINT(csflt) {
    *result = format_nnu("csflt", i->RT, i->RA, 155 - i->I8.u());
}

EMU(csflt) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    uint128_t scale = 1;
    scale <<= 155 - i->I8.u();
    for (int i = 0; i < 4; ++i) {
        rt.fs(i) = (float)ra.w(i) / scale;
    }
}

PRINT(cflts) {
    *result = format_nnu("cflts", i->RT, i->RA, 173 - i->I8.u());
}

EMU(cflts) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    uint128_t scale = 1;
    scale <<= 173 - i->I8.u();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.fs(i) * scale;
    }
}

PRINT(cuflt) {
    *result = format_nnu("cuflt", i->RT, i->RA, 155 - i->I8.u());
}

EMU(cuflt) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    uint128_t scale = 1;
    scale <<= 155 - i->I8.u();
    for (int i = 0; i < 4; ++i) {
        rt.fs(i) = (float)(uint32_t)ra.w(i) / scale;
    }
}

PRINT(cfltu) {
    *result = format_nnu("cfltu", i->RT, i->RA, 173 - i->I8.u());
}

EMU(cfltu) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    uint128_t scale = 1;
    scale <<= 173 - i->I8.u();
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = uint32_t(ra.fs(i) * scale);
    }
}

PRINT(frds) {
    *result = format_nn("frds", i->RT, i->RA);
}

EMU(frds) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.fs(i * 2) = ra.fd(i);
        rt.w(i * 2 + 1) = 0;
    }
}

PRINT(fesd) {
    *result = format_nn("fesd", i->RT, i->RA);
}

EMU(fesd) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.fd(i) = ra.fs(2 * i);
    }
}

PRINT(dfceq) {
    *result = format_nnn("dfceq", i->RT, i->RA, i->RB);
}

EMU(dfceq) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.dw(i) = ra.fd(i) == rb.fd(i) ? ~0ull : 0;
    }
}

PRINT(dfcmeq) {
    *result = format_nnn("dfcmeq", i->RT, i->RA, i->RB);
}

EMU(dfcmeq) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.dw(i) = std::abs(ra.fd(i)) == std::abs(rb.fd(i)) ? ~0ull : 0;
    }
}

PRINT(dfcgt) {
    *result = format_nnn("dfcgt", i->RT, i->RA, i->RB);
}

EMU(dfcgt) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.dw(i) = ra.fd(i) > rb.fd(i) ? ~0ull : 0;
    }
}

PRINT(dfcmgt) {
    *result = format_nnn("dfcmgt", i->RT, i->RA, i->RB);
}

EMU(dfcmgt) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 2; ++i) {
        rt.dw(i) = std::abs(ra.fd(i)) > std::abs(rb.fd(i)) ? ~0ull : 0;
    }
}

PRINT(dftsv) {
    *result = format_nnn("dftsv", i->RT, i->RA, i->I7);
}

EMU(dftsv) {
    auto& ra = th->r(i->RA);
    auto& rt = th->r(i->RT);
    auto i7 = i->I7.u();
    for (int i = 0; i < 2; ++i) {
        auto fd = ra.fd(i);
        auto c = std::fpclassify(fd);
        auto set = ((i7 & 0b1000000) && (c == FP_NAN))
                || ((i7 & 0b0100000) && (c == FP_INFINITE && copysign(1.f, fd) > 0.f))
                || ((i7 & 0b0010000) && (c == FP_INFINITE && copysign(1.f, fd) < 0.f))
                || ((i7 & 0b0001000) && (fd == +0.f))
                || ((i7 & 0b0000100) && (fd == -0.f))
                || ((i7 & 0b0000010) && (c == FP_SUBNORMAL && copysign(1.f, fd) > 0.f))
                || ((i7 & 0b0000001) && (c == FP_SUBNORMAL && copysign(1.f, fd) > 0.f));
        rt.dw(i) = set ? -0ull : 0;
    }
}

PRINT(fceq) {
    *result = format_nnn("fceq", i->RT, i->RA, i->RB);
}

EMU(fceq) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.fs(i) == rb.fs(i) ? ~0ul : 0;
    }
}

PRINT(fcmeq) {
    *result = format_nnn("fcmeq", i->RT, i->RA, i->RB);
}

EMU(fcmeq) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = std::abs(ra.fs(i)) == std::abs(rb.fs(i)) ? ~0ul : 0;
    }
}

PRINT(fcgt) {
    *result = format_nnn("fcgt", i->RT, i->RA, i->RB);
}

EMU(fcgt) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = ra.fs(i) > rb.fs(i) ? ~0ul : 0;
    }
}

PRINT(fcmgt) {
    *result = format_nnn("fcmgt", i->RT, i->RA, i->RB);
}

EMU(fcmgt) {
    auto& ra = th->r(i->RA);
    auto& rb = th->r(i->RB);
    auto& rt = th->r(i->RT);
    for (int i = 0; i < 4; ++i) {
        rt.w(i) = std::abs(ra.fs(i)) > std::abs(rb.fs(i)) ? ~0ul : 0;
    }
}

PRINT(fscrwr) {
    *result = format_n("fscrwr", i->RA);
}

EMU(fscrwr) {
    throw IllegalInstructionException();
}

PRINT(fscrrd) {
    *result = format_n("fscrrd", i->RT);
}

EMU(fscrrd) {
    throw IllegalInstructionException();
}

PRINT(stop) {
    *result = "stop";
}

EMU(stop) {
    throw StopSignalException();
}

PRINT(stopd) {
    *result = "stopd";
}

EMU(stopd) {
    throw StopSignalException();
}

PRINT(lnop) {
    *result = "lnop";
}

EMU(lnop) { }

PRINT(nop) {
    *result = "nop";
}

EMU(nop) { }

PRINT(sync) {
    *result = "sync";
}

EMU(sync) {
    __sync_synchronize();
}

PRINT(dsync) {
    *result = "dsync";
}

EMU(dsync) {
    __sync_synchronize();
}

template <DasmMode M, typename S>
void SPUDasm(void* instr, uint32_t cia, S* state) {
    uint32_t x = big_to_native<uint32_t>(*reinterpret_cast<uint32_t*>(instr));
    auto i = reinterpret_cast<SPUForm*>(&x);
    switch (i->OP11.u()) {
        case 0b00111000100: INVOKE(lqx);
        case 0b00101000100: INVOKE(stqx);
        case 0b00111110100: INVOKE(cbd);
        case 0b00111010100: INVOKE(cbx);
        case 0b00111110101: INVOKE(chd);
        case 0b00111010101: INVOKE(chx);
        case 0b00111110110: INVOKE(cwd);
        case 0b00111010110: INVOKE(cwx);
        case 0b00111110111: INVOKE(cdd);
        case 0b00111010111: INVOKE(cdx);
        case 0b00011001000: INVOKE(ah);
        case 0b00011000000: INVOKE(a);
        case 0b00001000000: INVOKE(sf);
        case 0b01101000000: INVOKE(addx);
        case 0b00011000010: INVOKE(cg);
        case 0b01101000010: INVOKE(cgx);
        case 0b00001000010: INVOKE(bg);
        case 0b01101000011: INVOKE(bgx);
        case 0b01111000100: INVOKE(mpy);
        case 0b01111001100: INVOKE(mpyu);
        case 0b01111000101: INVOKE(mpyh);
        case 0b01111000111: INVOKE(mpys);
        case 0b01111000110: INVOKE(mpyhh);
        case 0b01101000110: INVOKE(mpyhha);
        case 0b01111001110: INVOKE(mpyhhu);
        case 0b01101001110: INVOKE(mpyhhau);
        case 0b01010100101: INVOKE(clz);
        case 0b01010110100: INVOKE(cntb);
        case 0b00110110110: INVOKE(fsmb);
        case 0b00110110101: INVOKE(fsmh);
        case 0b00110110100: INVOKE(fsm);
        case 0b00110110010: INVOKE(gbb);
        case 0b00110110001: INVOKE(gbh);
        case 0b00110110000: INVOKE(gb);
        case 0b00011010011: INVOKE(avgb);
        case 0b00001010011: INVOKE(absdb);
        case 0b01001010011: INVOKE(sumb);
        case 0b01010110110: INVOKE(xsbh);
        case 0b01010101110: INVOKE(xshw);
        case 0b00011000001: INVOKE(and_);
        case 0b01011000001: INVOKE(andc);
        case 0b00001000001: INVOKE(or_);
        case 0b01011001001: INVOKE(orc);
        case 0b00111110000: INVOKE(orx);
        case 0b01001000001: INVOKE(xor_);
        case 0b00011001001: INVOKE(nand);
        case 0b00001001001: INVOKE(nor);
        case 0b01001001001: INVOKE(eqv);
        case 0b00001011111: INVOKE(shlh);
        case 0b00001111111: INVOKE(shlhi);
        case 0b00001011011: INVOKE(shl);
        case 0b00001111011: INVOKE(shli);
        case 0b00111011011: INVOKE(shlqbi);
        case 0b00111111011: INVOKE(shlqbii);
        case 0b00111011111: INVOKE(shlqby);
        case 0b00111111111: INVOKE(shlqbyi);
        case 0b00111001111: INVOKE(shlqbybi);
        case 0b00001011100: INVOKE(roth);
        case 0b00001111100: INVOKE(rothi);
        case 0b00001011000: INVOKE(rot);
        case 0b00001111000: INVOKE(roti);
        case 0b00111011100: INVOKE(rotqby);
        case 0b00111111100: INVOKE(rotqbyi);
        case 0b00111001100: INVOKE(rotqbybi);
        case 0b00111011000: INVOKE(rotqbi);
        case 0b00111111000: INVOKE(rotqbii);
        case 0b00001011101: INVOKE(rothm);
        case 0b00001111101: INVOKE(rothmi);
        case 0b00001011001: INVOKE(rotm);
        case 0b00001111001: INVOKE(rotmi);
        case 0b00111011101: INVOKE(rotqmby);
        case 0b00111111101: INVOKE(rotqmbyi);
        case 0b00111001101: INVOKE(rotqmbybi);
        case 0b00111011001: INVOKE(rotqmbi);
        case 0b00111111001: INVOKE(rotqmbii);
        case 0b00001011110: INVOKE(rotmah);
        case 0b00001111110: INVOKE(rotmahi);
        case 0b00001011010: INVOKE(rotma);
        case 0b00001111010: INVOKE(rotmai);
        case 0b01111011000: INVOKE(heq);
        case 0b01001011000: INVOKE(hgt);
        case 0b01011011000: INVOKE(hlgt);
        case 0b01111010000: INVOKE(ceqb);
        case 0b01111001000: INVOKE(ceqh);
        case 0b01111000000: INVOKE(ceq);
        case 0b01001010000: INVOKE(cgtb);
        case 0b01001001000: INVOKE(cgth);
        case 0b01001000000: INVOKE(cgt);
        case 0b01011010000: INVOKE(clgtb);
        case 0b01011001000: INVOKE(clgth);
        case 0b01011000000: INVOKE(clgt);
        case 0b00110101000: INVOKE(bi);
        case 0b00110101010: INVOKE(iret);
        case 0b00110101001: INVOKE(bisl);
        case 0b00100101000: INVOKE(biz);
        case 0b00100101001: INVOKE(binz);
        case 0b00100101010: INVOKE(bihz);
        case 0b00100101011: INVOKE(bihnz);
        case 0b00110101100: INVOKE(hbr);
        case 0b01011000100: INVOKE(fa);
        case 0b01011001100: INVOKE(dfa);
        case 0b01011000101: INVOKE(fs);
        case 0b01011001101: INVOKE(dfs);
        case 0b01011000110: INVOKE(fm);
        case 0b01011001110: INVOKE(dfm);
        case 0b01101011100: INVOKE(dfma);
        case 0b01101011110: INVOKE(dfnms);
        case 0b01101011101: INVOKE(dfms);
        case 0b01101011111: INVOKE(dfnma);
        case 0b00110111000: INVOKE(frest);
        case 0b00110111001: INVOKE(frsqest);
        case 0b01111010100: INVOKE(fi);
        case 0b01110111001: INVOKE(frds);
        case 0b01110111000: INVOKE(fesd);
        case 0b01111000011: INVOKE(dfceq);
        case 0b01111001011: INVOKE(dfcmeq);
        case 0b01011000011: INVOKE(dfcgt);
        case 0b01011001011: INVOKE(dfcmgt);
        case 0b01110111111: INVOKE(dftsv);
        case 0b01111000010: INVOKE(fceq);
        case 0b01111001010: INVOKE(fcmeq);
        case 0b01011000010: INVOKE(fcgt);
        case 0b01011001010: INVOKE(fcmgt);
        case 0b01110111010: INVOKE(fscrwr);
        case 0b01110011000: INVOKE(fscrrd);
        case 0b00000000000: INVOKE(stop);
        case 0b00101000000: INVOKE(stopd);
        case 0b00000000001: INVOKE(lnop);
        case 0b01000000001: INVOKE(nop);
        case 0b00000000010: INVOKE(sync);
        case 0b00000000011: INVOKE(dsync);
    }
    switch (i->OP10.u()) {
        case 0b0111011010: INVOKE(csflt);
        case 0b0111011000: INVOKE(cflts);
        case 0b0111011011: INVOKE(cuflt);
        case 0b0111011001: INVOKE(cfltu);
    }
    switch (i->OP9.u()) {
        case 0b001100001: INVOKE(lqa);
        case 0b001100111: INVOKE(lqr);
        case 0b001000001: INVOKE(stqa);
        case 0b001000111: INVOKE(stqr);
        case 0b010000011: INVOKE(ilh);
        case 0b010000010: INVOKE(ilhu);
        case 0b010000001: INVOKE(il);
        case 0b011000001: INVOKE(iohl);
        case 0b001100101: INVOKE(fsmbi);
        case 0b001100100: INVOKE(br);
        case 0b001100000: INVOKE(bra);
        case 0b001100110: INVOKE(brsl);
        case 0b001100010: INVOKE(brasl);
        case 0b001000010: INVOKE(brnz);
        case 0b001000000: INVOKE(brz);
        case 0b001000110: INVOKE(brhnz);
        case 0b001000100: INVOKE(brhz);
    }
    switch (i->OP8.u()) {
        case 0b00110100: INVOKE(lqd);
        case 0b00100100: INVOKE(stqd);
        case 0b00011101: INVOKE(ahi);
        case 0b00011100: INVOKE(ai);
        case 0b00001101: INVOKE(sfhi);
        case 0b00001100: INVOKE(sfi);
        case 0b01110100: INVOKE(mpyi);
        case 0b01110101: INVOKE(mpyui);
        case 0b00010110: INVOKE(andbi);
        case 0b00010101: INVOKE(andhi);
        case 0b00010100: INVOKE(andi);
        case 0b00000110: INVOKE(orbi);
        case 0b00000101: INVOKE(orhi);
        case 0b00000100: INVOKE(ori);
        case 0b01000110: INVOKE(xorbi);
        case 0b01000101: INVOKE(xorhi);
        case 0b01000100: INVOKE(xori);
        case 0b01111111: INVOKE(heqi);
        case 0b01001111: INVOKE(hgti);
        case 0b01011111: INVOKE(hlgti);
        case 0b01111110: INVOKE(ceqbi);
        case 0b01111101: INVOKE(ceqhi);
        case 0b01111100: INVOKE(ceqi);
        case 0b01001110: INVOKE(cgtbi);
        case 0b01001101: INVOKE(cgthi);
        case 0b01001100: INVOKE(cgti);
        case 0b01011110: INVOKE(clgtbi);
        case 0b01011101: INVOKE(clgthi);
        case 0b01011100: INVOKE(clgti);
    }
    switch (i->OP7.u()) {
        case 0b0100001: INVOKE(ila);
        case 0b0001000: INVOKE(hbra);
        case 0b0001001: INVOKE(hbrr);
    }
    switch (i->OP4.u()) {
        case 0b1100: INVOKE(mpya);
        case 0b1000: INVOKE(selb);
        case 0b1011: INVOKE(shufb);
        case 0b1110: INVOKE(fma);
        case 0b1101: INVOKE(fnms);
        case 0b1111: INVOKE(fms);
    }
    throw IllegalInstructionException();
}

template void SPUDasm<DasmMode::Print, std::string>(
    void* instr, uint32_t cia, std::string* state);

template void SPUDasm<DasmMode::Emulate, SPUThread>(
    void* instr, uint32_t cia, SPUThread* th);

template void SPUDasm<DasmMode::Name, std::string>(
    void* instr, uint32_t cia, std::string* name);