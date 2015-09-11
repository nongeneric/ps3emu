#include "shader_dasm.h"
#include "utils.h"
#include <stdexcept>
#include "assert.h"

struct fragment_opcode_t {
    int op_count;
    bool control;
    bool nop;
    bool tex;
    const char* mnemonic;
};

fragment_opcode_t fragment_opcodes[] = {
    { 0, 1, 1, 0, "NOP" },
    { 1, 0, 0, 0, "MOV" },
    { 2, 0, 0, 0, "MUL" },
    { 2, 0, 0, 0, "ADD" },
    { 3, 0, 0, 0, "MAD" },
    { 2, 0, 0, 0, "DP3" },
    { 2, 0, 0, 0, "DP4" },
    { 2, 0, 0, 0, "DST" },
    { 2, 0, 0, 0, "MIN" },
    { 2, 0, 0, 0, "MAX" },
    { 2, 0, 0, 0, "SLT" },
    { 2, 0, 0, 0, "SGE" },
    { 2, 0, 0, 0, "SLE" },
    { 2, 0, 0, 0, "SGT" },
    { 2, 0, 0, 0, "SNE" },
    { 2, 0, 0, 0, "SEQ" },
    { 1, 0, 0, 0, "FRC" },
    { 1, 0, 0, 0, "FLR" },
    { 0, 1, 0, 0, "KIL" },
    { 1, 0, 0, 0, "PK4" },
    { 1, 0, 0, 0, "UP4" },
    { 1, 0, 0, 0, "DDX" },
    { 1, 0, 0, 0, "DDY" },
    { 1, 0, 0, 1, "TEX" },
    { 1, 0, 0, 1, "TXP" },
    { 3, 0, 0, 1, "TXD" },
    { 1, 0, 0, 0, "RCP" },
    { 1, 0, 0, 0, "RSQ" },
    { 1, 0, 0, 0, "EX2" },
    { 1, 0, 0, 0, "LG2" },
    { 1, 0, 0, 0, "LIT" },
    { 3, 0, 0, 0, "LRP" },
    { 2, 0, 0, 0, "STR" },
    { 2, 0, 0, 0, "SFL" },
    { 1, 0, 0, 0, "COS" },
    { 1, 0, 0, 0, "SIN" },
    { 1, 0, 0, 0, "PK2" },
    { 1, 0, 0, 0, "UP2" },
    { 2, 0, 0, 0, "POW" },
    { 1, 0, 0, 0, "PKB" },
    { 1, 0, 0, 0, "UPB" },
    { 1, 0, 0, 0, "PK16" },
    { 1, 0, 0, 0, "UP16" },
    { 3, 0, 0, 0, "BEM" },
    { 1, 0, 0, 0, "PKG" },
    { 1, 0, 0, 0, "UPG" },
    { 3, 0, 0, 0, "DP2A" },
    { 3, 0, 0, 1, "TXL" },
    { 3, 0, 0, 1, "TXB" },
    { 3, 0, 0, 1, "TEXBEM" },
    { 3, 0, 0, 1, "TXPBEM" },
    { 3, 0, 0, 0, "BEMLUM" },
    { 2, 0, 0, 0, "REFL" },
    { 1, 0, 0, 1, "TIMESWTEX" },
    { 2, 0, 0, 0, "DP2" },
    { 1, 0, 0, 0, "NRM" },
    { 2, 0, 0, 0, "DIV" },
    { 2, 0, 0, 0, "DIVSQ" },
    { 1, 0, 0, 0, "LIF" },
    { 0, 1, 0, 0, "FENCT" },
    { 0, 1, 0, 0, "FENCB" },
    { 0, 1, 0, 0, "BRK" },
    { 0, 1, 0, 0, "CAL" },
    { 0, 1, 0, 0, "IFE" },
    { 0, 1, 0, 0, "LOOP" },
    { 0, 1, 0, 0, "REP" },
    { 0, 1, 0, 0, "RET" }
};

std::string print_op0_mask(fragment_instr_t* instr) {
    auto x = instr->b0.Op0xMask.u();
    auto y = instr->b0.Op0yMask.u();
    auto z = instr->b0.Op0zMask.u();
    auto w = instr->b0.Op0wMask.u();
    if (x && y && z && w)
        return "";
    std::string res = ".";
    if (x)
        res += "x";
    if (y)
        res += "y";
    if (z)
        res += "z";
    if (w)
        res += "w";
    return res;
}

const char* print_cond(cond_t cond) {
    switch (cond) {
        case cond_t::EQ: return "EQ";
        case cond_t::FL: return "FL";
        case cond_t::GE: return "GE";
        case cond_t::GT: return "GT";
        case cond_t::LE: return "LE";
        case cond_t::LT: return "LT";
        case cond_t::NE: return "NE";
        case cond_t::TR: return "TR";
        default: assert(false); return "";
    }
}

const char* print_swizzle2bit(int bits) {
    auto s = static_cast<swizzle2bit_t>(bits);
    if (s == swizzle2bit_t::X)
        return "x";
    if (s == swizzle2bit_t::Y)
        return "y";
    if (s == swizzle2bit_t::Z)
        return "z";
    return "w";
}

std::string print_swizzle(int x, int y, int z, int w) {
    if (x == 0 && y == 1 && z == 2 && w == 3)
        return "";
    std::string res = ".";
    if (x == y && y == z && z == w) {
        res += print_swizzle2bit(x);
    } else {
        res += print_swizzle2bit(x);
        res += print_swizzle2bit(y);
        res += print_swizzle2bit(z);
        res += print_swizzle2bit(w);
    }
    return res;
}

const char* print_attr(input_attr_t attr) {
#define p(n) if (attr == input_attr_t::n) return #n;
    p(WPOS); p(COL0); p(COL1); p(FOGC);
    p(TEX0); p(TEX1); p(TEX2); p(TEX3);
    p(TEX4); p(TEX5); p(TEX6); p(TEX7);
    p(TEX8); p(TEX9); p(SSA);
    return "v15";
}

int fragment_dasm(uint8_t* instr, std::string& res) {
    uint8_t buf[16];
    for (auto i = 0u; i < sizeof(buf); i += 4) {
        buf[i + 0] = instr[i + 2];
        buf[i + 1] = instr[i + 3];
        buf[i + 2] = instr[i + 0];
        buf[i + 3] = instr[i + 1];
    }
    auto i = (fragment_instr_t*)buf;
    auto opcode = fragment_opcodes[i->Opcode()];
    res += opcode.mnemonic;
    const char* clamp;
    switch (i->Clamp()) {
        case clamp_t::B:
            clamp = "B";
            break;
        case clamp_t::H:
            clamp = "H";
            break;
        case clamp_t::R:
            clamp = "R";
            break;
        case clamp_t::X:
            clamp = "X";
            break;
    }
    res += clamp;
    auto control = "";
    switch (i->Control()) {
        case control_mod_t::C0:
            control = "C";
            break;
        case control_mod_t::C1:
            control = "C1";
            break;
        case control_mod_t::None: break;
    }
    res += control;
    if (!opcode.control) {
        auto scale = "";
        switch (i->Scale()) {
            case scale_t::None:
                break;
            case scale_t::M2:
                scale = "_m2";
                break;
            case scale_t::M4:
                scale = "_m4";
                break;
            case scale_t::M8:
                scale = "_m8";
                break;
            case scale_t::D2:
                scale = "_d2";
                break;
            case scale_t::D4:
                scale = "_d4";
                break;
            case scale_t::D8:
                scale = "_d8";
                break;
            case scale_t::Illegal:
                scale = "<illegal>";
                break;
            default: assert(false);
        }
        res += scale;
        if (i->b0.Bx2.u()) {
            res += "_bx2";
        }
        if (i->b0.Sat.u()) {
            res += "_sat";
        }
        res += " ";
        if (i->RegType() == reg_type_t::H) {
            res += "H";
        } else {
            res += "R";
        }
        if (i->b0.RegC.u()) {
            res += "C";
        } else {
            res += ssnprintf("%d", i->b0.RegNum.u());
        }
        res += print_op0_mask(i);
    }
    // LOOP, REP, RET, CAL
    auto cond = i->Cond();
    if (cond != cond_t::TR && !opcode.nop) {
        auto x = i->b1.CxMask.u();
        auto y = i->b1.CyMask.u();
        auto z = i->b1.CzMask.u();
        auto w = i->b1.CwMask.u();
        auto swizzle = print_swizzle(x, y, z, w);
        auto creg = i->b1.CondC1.u() ? "1" : "";
        res += ssnprintf("(%s%s%s)", print_cond(cond), creg, swizzle);
    }
    int size = 16;
    for (int n = 0; n < opcode.op_count; ++n) {
        res += ", ";
        auto optype = i->OpType(n);
        auto opregnum = i->OpRegNum(n);
        auto opregtype = i->OpRegType(n);
        auto opneg = i->OpNeg(n);
        auto opabs = i->OpAbs(n);
        auto x = i->OpMaskX(n);
        auto y = i->OpMaskY(n);
        auto z = i->OpMaskZ(n);
        auto w = i->OpMaskW(n);
        if (opneg)
            res += "-";
        if (opabs)
            res += "|";
        if (optype == op_type_t::Attr) {
            if (i->PerspCorr() == persp_corr_t::F) {
                res += "f[";
            } else {
                res += "g[";
            }
            if (i->b3.AL.u()) {
                res += ssnprintf("aL+%d", i->ALIndex());
            } else {
                res += print_attr(i->InputAttr());
            }
            res += "]";
        } else if (optype == op_type_t::Const) {
            // ?
        } else {
            auto r = opregtype == reg_type_t::H ? "H" : "R";
            res += ssnprintf("%s%lu", r, opregnum);
        }
        res += print_swizzle((int)x, (int)y, (int)z, (int)w);
        if (opabs)
            res += "|";
        // TEX
        res += ";";
        if (i->b0.LastInstr.u())
            res += " # last instruction";
    }
    return size;
}
