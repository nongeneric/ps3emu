#include "shader_dasm.h"
#include "utils.h"
#include <stdexcept>
#include <boost/endian/arithmetic.hpp>
#include "assert.h"

fragment_opcode_t fragment_opcodes[] = {
    { 0, 1, 1, 0, "NOP", fragment_op_t::NOP },
    { 1, 0, 0, 0, "MOV", fragment_op_t::MOV },
    { 2, 0, 0, 0, "MUL", fragment_op_t::MUL },
    { 2, 0, 0, 0, "ADD", fragment_op_t::ADD },
    { 3, 0, 0, 0, "MAD", fragment_op_t::MAD },
    { 2, 0, 0, 0, "DP3", fragment_op_t::DP3 },
    { 2, 0, 0, 0, "DP4", fragment_op_t::DP4 },
    { 2, 0, 0, 0, "DST", fragment_op_t::DST },
    { 2, 0, 0, 0, "MIN", fragment_op_t::MIN },
    { 2, 0, 0, 0, "MAX", fragment_op_t::MAX },
    { 2, 0, 0, 0, "SLT", fragment_op_t::SLT },
    { 2, 0, 0, 0, "SGE", fragment_op_t::SGE },
    { 2, 0, 0, 0, "SLE", fragment_op_t::SLE },
    { 2, 0, 0, 0, "SGT", fragment_op_t::SGT },
    { 2, 0, 0, 0, "SNE", fragment_op_t::SNE },
    { 2, 0, 0, 0, "SEQ", fragment_op_t::SEQ },
    { 1, 0, 0, 0, "FRC", fragment_op_t::FRC },
    { 1, 0, 0, 0, "FLR", fragment_op_t::FLR },
    { 0, 1, 0, 0, "KIL", fragment_op_t::KIL },
    { 1, 0, 0, 0, "PK4", fragment_op_t::PK4 },
    { 1, 0, 0, 0, "UP4", fragment_op_t::UP4 },
    { 1, 0, 0, 0, "DDX", fragment_op_t::DDX },
    { 1, 0, 0, 0, "DDY", fragment_op_t::DDY },
    { 1, 0, 0, 1, "TEX", fragment_op_t::TEX },
    { 1, 0, 0, 1, "TXP", fragment_op_t::TXP },
    { 3, 0, 0, 1, "TXD", fragment_op_t::TXD },
    { 1, 0, 0, 0, "RCP", fragment_op_t::RCP },
    { 1, 0, 0, 0, "RSQ", fragment_op_t::RSQ },
    { 1, 0, 0, 0, "EX2", fragment_op_t::EX2 },
    { 1, 0, 0, 0, "LG2", fragment_op_t::LG2 },
    { 1, 0, 0, 0, "LIT", fragment_op_t::LIT },
    { 3, 0, 0, 0, "LRP", fragment_op_t::LRP },
    { 2, 0, 0, 0, "STR", fragment_op_t::STR },
    { 2, 0, 0, 0, "SFL", fragment_op_t::SFL },
    { 1, 0, 0, 0, "COS", fragment_op_t::COS },
    { 1, 0, 0, 0, "SIN", fragment_op_t::SIN },
    { 1, 0, 0, 0, "PK2", fragment_op_t::PK2 },
    { 1, 0, 0, 0, "UP2", fragment_op_t::UP2 },
    { 2, 0, 0, 0, "POW", fragment_op_t::POW },
    { 1, 0, 0, 0, "PKB", fragment_op_t::PKB },
    { 1, 0, 0, 0, "UPB", fragment_op_t::UPB },
    { 1, 0, 0, 0, "PK16", fragment_op_t::PK16 },
    { 1, 0, 0, 0, "UP16", fragment_op_t::UP16 },
    { 3, 0, 0, 0, "BEM", fragment_op_t::BEM },
    { 1, 0, 0, 0, "PKG", fragment_op_t::PKG },
    { 1, 0, 0, 0, "UPG", fragment_op_t::UPG },
    { 3, 0, 0, 0, "DP2A", fragment_op_t::DP2A },
    { 3, 0, 0, 1, "TXL", fragment_op_t::TXL },
    { 0, 0, 0, 0, "illegal" },
    { 3, 0, 0, 1, "TXB", fragment_op_t::TXB },
    { 0, 0, 0, 0, "illegal" },
    { 3, 0, 0, 1, "TEXBEM", fragment_op_t::TEXBEM },
    { 3, 0, 0, 1, "TXPBEM", fragment_op_t::TXPBEM },
    { 3, 0, 0, 0, "BEMLUM", fragment_op_t::BEMLUM },
    { 2, 0, 0, 0, "REFL", fragment_op_t::REFL },
    { 1, 0, 0, 1, "TIMESWTEX", fragment_op_t::TIMESWTEX },
    { 2, 0, 0, 0, "DP2", fragment_op_t::DP2 },
    { 1, 0, 0, 0, "NRM", fragment_op_t::NRM },
    { 2, 0, 0, 0, "DIV", fragment_op_t::DIV },
    { 2, 0, 0, 0, "DIVSQ", fragment_op_t::DIVSQ },
    { 1, 0, 0, 0, "LIF", fragment_op_t::LIF },
    { 0, 1, 0, 0, "FENCT", fragment_op_t::FENCT },
    { 0, 1, 0, 0, "FENCB", fragment_op_t::FENCB },
    { 0, 0, 0, 0, "illegal" },
    { 0, 1, 0, 0, "BRK", fragment_op_t::BRK },
    { 0, 1, 0, 0, "CAL", fragment_op_t::CAL },
    { 0, 1, 0, 0, "IFE", fragment_op_t::IFE },
    { 0, 1, 0, 0, "LOOP", fragment_op_t::LOOP },
    { 0, 1, 0, 0, "REP", fragment_op_t::REP },
    { 0, 1, 0, 0, "RET", fragment_op_t::RET },
};

struct fragment_instr_t {
    union {
        BitField<0, 1> RegType;
        BitField<1, 7> RegNum;
        BitField<7, 8> LastInstr;
        BitField<11, 12> Op0wMask;
        BitField<12, 13> Op0zMask;
        BitField<13, 14> Op0yMask;
        BitField<14, 15> Op0xMask;
        BitField<15, 16> C0;
        BitField<16, 18> Clamp;
        BitField<18, 19> Bx2;
        BitField<24, 25> Sat;
        BitField<25, 26> RegC;
        BitField<26, 32> Opcode16;
        
        BitField<8, 11> InputAttr8_10;
        BitField<23, 24> InputAttr23;
    } b0;
    union {
        BitField<0, 6> Op1RegNum;
        BitField<6, 8> Op1Type;
        BitField<15, 16> Op1RegType;
        BitField<19, 22> Cond;
        BitField<22, 23> Op1Neg;
        BitField<24, 25> C1;
        BitField<25, 26> CondC1;
        BitField<26, 27> Op1Abs;
        
        BitField<17, 19> CxMask;
        BitField<16, 17> CyMask16;
        BitField<31, 32> CyMask31;
        BitField<27, 29> CwMask;
        BitField<29, 31> CzMask;
        
        BitField<9, 11> Op1zMask;
        BitField<11, 13> Op1yMask;
        BitField<13, 15> Op1xMask;
        BitField<8, 9> Op1wMask8;
        BitField<23, 24> Op1wMask23;
    } b1;
    union {
        BitField<24, 25> Opcode0;
        BitField<25, 28> Scale;
        
        BitField<0, 6> Op2RegNum;
        BitField<6, 8> Op2Type;
        BitField<15, 16> Op2RegType;
        BitField<22, 23> Op2Neg;
        BitField<26, 27> Op2Abs;
        
        BitField<9, 11> Op2zMask;
        BitField<11, 13> Op2yMask;
        BitField<13, 15> Op2xMask;
        BitField<8, 9> Op2wMask8;
        BitField<23, 24> Op2wMask23;
    } b2;
    union {
        BitField<24, 25> PerspCorr;
        BitField<25, 26> AL;
        BitField<18, 29> ALIndex;
        
        BitField<0, 6> Op3RegNum;
        BitField<6, 8> Op3Type;
        BitField<15, 16> Op3RegType;
        BitField<22, 23> Op3Neg;
        BitField<26, 27> Op3Abs;
        
        BitField<9, 11> Op3zMask;
        BitField<11, 13> Op3yMask;
        BitField<13, 15> Op3xMask;
        BitField<8, 9> Op3wMask8;
        BitField<23, 24> Op3wMask23;
    } b3;
    
    inline unsigned Opcode() {
        return (b2.Opcode0.u() << 6) | b0.Opcode16.u();
    }
    
    inline clamp_t Clamp() {
        return static_cast<clamp_t>(b0.Clamp.u());
    }
    
    inline control_mod_t Control() {
        if (b0.C0.u()) {
            if (b1.C1.u()) {
                return control_mod_t::C1;
            } else {
                return control_mod_t::C0;
            }
        }
        return control_mod_t::None;
    }
    
    inline scale_t Scale() {
        return static_cast<scale_t>(b2.Scale.u());
    }
    
    inline cond_t Cond() {
        return static_cast<cond_t>(b1.Cond.u());
    }
    
    inline persp_corr_t PerspCorr() {
        return static_cast<persp_corr_t>(b3.PerspCorr.u());
    }
    
    inline int ALIndex() {
        return b3.ALIndex.u() + 4;
    }
    
    inline input_attr_t InputAttr() {
        auto v = b0.InputAttr8_10.u() | (b0.InputAttr23.u() << 3);
        return static_cast<input_attr_t>(v);
    }
    
    inline reg_type_t RegType() {
        return static_cast<reg_type_t>(b0.RegType.u());
    }
    
    inline op_type_t OpType(int n) {
        int t;
        if (n == 0) {
            t = b1.Op1Type.u();
        } else if (n == 1) {
            t = b2.Op2Type.u();
        } else {
            t = b3.Op3Type.u();
        }
        return static_cast<op_type_t>(t);
    }
    
    inline bool OpNeg(int n) {
        if (n == 0) {
            return b1.Op1Neg.u();
        } else if (n == 1) {
            return b2.Op2Neg.u();
        }
        return b3.Op3Neg.u();
    }
    
    inline bool OpAbs(int n) {
        if (n == 0) {
            return b1.Op1Abs.u();
        } else if (n == 1) {
            return b2.Op2Abs.u();
        }
        return b3.Op3Abs.u();
    }
    
    inline int OpRegNum(int n) {
        if (n == 0) {
            return b1.Op1RegNum.u();
        } else if (n == 1) {
            return b2.Op2RegNum.u();
        }
        return b3.Op3RegNum.u();
    }
    
    inline reg_type_t OpRegType(int n) {
        int t;
        if (n == 0) {
            t = b1.Op1RegType.u();
        } else if (n == 1) {
            t = b2.Op2RegType.u();
        } else {
            t = b3.Op3RegType.u();
        }
        return static_cast<reg_type_t>(t);
    }
    
    inline swizzle2bit_t OpMaskX(int n) {
        if (n == 0)
            return static_cast<swizzle2bit_t>(b1.Op1xMask.u());
        if (n == 1)
            return static_cast<swizzle2bit_t>(b2.Op2xMask.u());
        return static_cast<swizzle2bit_t>(b3.Op3xMask.u());   
    }
    
    inline swizzle2bit_t OpMaskY(int n) {
        if (n == 0)
            return static_cast<swizzle2bit_t>(b1.Op1yMask.u());
        if (n == 1)
            return static_cast<swizzle2bit_t>(b2.Op2yMask.u());
        return static_cast<swizzle2bit_t>(b3.Op3yMask.u());   
    }
    
    inline swizzle2bit_t OpMaskZ(int n) {
        if (n == 0)
            return static_cast<swizzle2bit_t>(b1.Op1zMask.u());
        if (n == 1)
            return static_cast<swizzle2bit_t>(b2.Op2zMask.u());
        return static_cast<swizzle2bit_t>(b3.Op3zMask.u());   
    }
    
    inline swizzle2bit_t OpMaskW(int n) {
        auto _23 = 0u;
        auto _8 = 0u;
        if (n == 0) {
            _23 = b1.Op1wMask23.u();
            _8 = b1.Op1wMask8.u();
        } else if (n == 1) {
            _23 = b2.Op2wMask23.u();
            _8 = b2.Op2wMask8.u();
        } else {
            _23 = b3.Op3wMask23.u();
            _8 = b3.Op3wMask8.u();
        }
        return static_cast<swizzle2bit_t>((_23 << 1) | _8);
    }
    
    inline int CyMask() {
        return b1.CyMask16.u() | (b1.CyMask31.u() << 1);
    }
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
        auto y = i->CyMask();
        auto z = i->b1.CzMask.u();
        auto w = i->b1.CwMask.u();
        auto swizzle = print_swizzle(x, y, z, w);
        auto creg = i->b1.CondC1.u() ? "1" : "";
        res += ssnprintf("(%s%s%s)", print_cond(cond), creg, swizzle.c_str());
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
            size += 16;
            uint8_t cbuf[16];
            // exch words and then reverse bytes to make le dwords
            for (auto i = 0u; i < sizeof(cbuf); i += 4) {
                cbuf[i + 0] = instr[i + 16 + 1];
                cbuf[i + 1] = instr[i + 16 + 0];
                cbuf[i + 2] = instr[i + 16 + 3];
                cbuf[i + 3] = instr[i + 16 + 2];
            }
            auto cu = (uint32_t*)cbuf;
            auto cf = (float*)cbuf;
            res += ssnprintf("{0x%08x(%g), 0x%08x(%g), 0x%08x(%g), 0x%08x(%g)}",
                cu[0], cf[0], cu[1], cf[1], cu[2], cf[2], cu[3], cf[3]
            );
        } else {
            auto r = opregtype == reg_type_t::H ? "H" : "R";
            res += ssnprintf("%s%lu", r, opregnum);
        }
        res += print_swizzle((int)x, (int)y, (int)z, (int)w);
        if (opabs)
            res += "|";
        // TEX
    }
    res += ";";
    if (i->b0.LastInstr.u())
        res += " # last instruction";
    return size;
}

FragmentInstr fragment_dasm_instr(uint8_t* instr) {
    uint8_t buf[16];
    for (auto i = 0u; i < sizeof(buf); i += 4) {
        buf[i + 0] = instr[i + 2];
        buf[i + 1] = instr[i + 3];
        buf[i + 2] = instr[i + 0];
        buf[i + 3] = instr[i + 1];
    }
    
    auto i = (fragment_instr_t*)buf;
    auto opcode = fragment_opcodes[i->Opcode()];
    
    FragmentInstr res;
    res.opcode = opcode;
    res.clamp = i->Clamp();
    
    return res;
}


