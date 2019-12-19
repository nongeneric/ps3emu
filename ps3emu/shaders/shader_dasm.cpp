#include "shader_dasm.h"
#include "../utils.h"
#include <stdexcept>
#include <boost/endian/arithmetic.hpp>
#include <boost/concept_check.hpp>
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
    { 2, 0, 0, 1, "TEX", fragment_op_t::TEX },
    { 2, 0, 0, 1, "TXP", fragment_op_t::TXP },
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
    { 0, 0, 0, 0, "illegal_1" },
    { 3, 0, 0, 1, "TXB", fragment_op_t::TXB },
    { 0, 0, 0, 0, "illegal_2" },
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
    { 0, 0, 0, 0, "illegal_3" },
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
        BitField<8, 11> InputAttr8_10;
        BitField<11, 12> Op0wMask;
        BitField<12, 13> Op0zMask;
        BitField<13, 14> Op0yMask;
        BitField<14, 15> Op0xMask;
        BitField<15, 16> C0;
        BitField<16, 18> Clamp;
        BitField<18, 19> Bx2;
        BitField<19, 23> TexNum;
        BitField<23, 24> InputAttr23;
        BitField<24, 25> Sat;
        BitField<25, 26> RegC;
        BitField<26, 32> Opcode16;
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
        BitField<0, 6> Op2RegNum;
        BitField<6, 8> Op2Type;
        BitField<8, 9> Op2wMask8;
        BitField<9, 11> Op2zMask;
        BitField<11, 13> Op2yMask;
        BitField<13, 15> Op2xMask;
        BitField<15, 16> Op2RegType;
        BitField<18, 21> InputModifier;
        BitField<22, 23> Op2Neg;
        BitField<23, 24> Op2wMask23;
        BitField<24, 25> Opcode0;
        BitField<25, 28> Scale;
        BitField<26, 27> Op2Abs;
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

std::string print_dest_mask(dest_mask_t mask) {
    auto x = mask.val[0];
    auto y = mask.val[1];
    auto z = mask.val[2];
    auto w = mask.val[3];
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

std::string print_swizzle(bool allComponents, int x, int y, int z, int w) {
    if (x == 0 && y == 1 && z == 2 && w == 3)
        return "";
    std::string res = ".";
    if (!allComponents && x == y && y == z && z == w) {
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

void fragment_dasm(FragmentInstr const& i, std::string& res) {
    res += i.opcode.mnemonic;
    const char* clamp = "";
    switch (i.clamp) {
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
    switch (i.control) {
        case control_mod_t::C0:
            control = "C";
            break;
        case control_mod_t::C1:
            control = "C1";
            break;
        case control_mod_t::None: break;
    }
    res += control;
    if (!i.opcode.control) {
        auto scale = "";
        switch (i.scale) {
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
        if (i.is_bx2) {
            res += "_bx2";
        }
        if (i.is_sat) {
            res += "_sat";
        }
        res += " ";
        if (i.reg == reg_type_t::H) {
            res += "H";
        } else {
            res += "R";
        }
        if (i.is_reg_c) {
            res += "C";
        } else {
            res += ssnprintf("%d", i.reg_num);
        }
        res += print_dest_mask(i.dest_mask);
    }
    if (i.condition.relation != cond_t::TR && !i.opcode.nop) {
        if (i.opcode.control)
            res += " ";
        auto swizzle = print_swizzle(i.condition.swizzle, false);
        auto creg = i.condition.is_C1 ? "1" : "";
        res += ssnprintf("(%s%s%s)", print_cond(i.condition.relation), creg, swizzle.c_str());
    }
    // TODO: LOOP, REP, RET, CAL
    for (int n = 0; n < i.opcode.op_count; ++n) {
        auto& arg = i.arguments[n];
        res += ", ";
        if (arg.is_neg)
            res += "-";
        if (arg.is_abs)
            res += "|";
        if (i.opcode.tex && n == i.opcode.op_count - 1) {
            res += ssnprintf("TEX%d", i.tex_num);
        } else if (arg.type == op_type_t::Attr) {
            if (i.persp_corr == persp_corr_t::F) {
                res += "f[";
            } else {
                res += "g[";
            }
            if (i.is_al) {
                res += ssnprintf("aL+%d", i.al_index);
            } else {
                res += print_attr(i.input_attr);
            }
            res += "]";
        } else if (arg.type == op_type_t::Const) {
            auto cu = i.arguments[n].imm_val.u;
            auto cf = i.arguments[n].imm_val.f;
            res += ssnprintf("{0x%08x(%g), 0x%08x(%g), 0x%08x(%g), 0x%08x(%g)}",
                cu[0], cf[0], cu[1], cf[1], cu[2], cf[2], cu[3], cf[3]
            );
        } else {
            auto r = arg.reg_type == reg_type_t::H ? "H" : "R";
            res += ssnprintf("%s%lu", r, arg.reg_num);
        }
        switch (i.input_modifier) {
            case input_modifier_t::_R: break;
            case input_modifier_t::_H: res += "_h";
            case input_modifier_t::_X: res += "_x";
            case input_modifier_t::_B: res += "_b";
            case input_modifier_t::_S: res += "_s";
            case input_modifier_t::_N: res += "_n";
        }
        res += print_swizzle(arg.swizzle, false);
        if (arg.is_abs)
            res += "|";
    }
    if (i.opcode.instr == fragment_op_t::IFE) {
        res += ssnprintf(" [%03x, %03x]", i.elseLabel, i.endifLabel);
    }
    res += ";";
    if (i.is_last) {
        res += " # last instruction";
    }
}

void fragment_scan_instr(const uint8_t* instr, bool& isConst, bool& isLast) {
    uint8_t buf[16];
    read_fragment_instr(instr, buf);
    auto i = (fragment_instr_t*)buf;
    auto opcode = fragment_opcodes[i->Opcode()];
    isConst = false;
    for (int n = 0; n < opcode.op_count; ++n) {
        if (i->OpType(n) == op_type_t::Const) {
            isConst = true;
            break;
        }
    }
    isLast = i->b0.LastInstr.u();
}

int fragment_dasm_instr(const uint8_t* instr, FragmentInstr& res) {
    uint8_t buf[16];
    read_fragment_instr(instr, buf);
    auto i = (fragment_instr_t*)buf;
    auto opcode = fragment_opcodes[i->Opcode()];

    if (opcode.instr == fragment_op_t::IFE) {
        uint8_t buf[16];
        read_fragment_imm_val(instr, buf);
        auto dw = (uint32_t*)&buf[0];
        res.elseLabel = (dw[2] >> 2) & 0x1ffff;
        res.endifLabel = (dw[3] >> 2) & 0x1ffff;
    }
    
    res.opcode = opcode;
    res.clamp = i->Clamp();
    res.input_modifier = (input_modifier_t)i->b2.InputModifier.u();
    res.control = i->Control();
    res.tex_num = i->b0.TexNum.u();
    res.scale = i->Scale();
    res.is_bx2 = i->b0.Bx2.u();
    res.is_sat = i->b0.Sat.u();
    res.reg = i->RegType();
    res.is_reg_c = i->b0.RegC.u();
    res.reg_num = i->b0.RegNum.u();
    res.dest_mask.val[0] = i->b0.Op0xMask.u();
    res.dest_mask.val[1] = i->b0.Op0yMask.u();
    res.dest_mask.val[2] = i->b0.Op0zMask.u();
    res.dest_mask.val[3] = i->b0.Op0wMask.u();
    res.condition.relation = i->Cond();
    res.condition.is_C1 = i->b1.CondC1.u();
    res.condition.swizzle.comp[0] = static_cast<swizzle2bit_t>(i->b1.CxMask.u());
    res.condition.swizzle.comp[1] = static_cast<swizzle2bit_t>(i->CyMask());
    res.condition.swizzle.comp[2] = static_cast<swizzle2bit_t>(i->b1.CzMask.u());
    res.condition.swizzle.comp[3] = static_cast<swizzle2bit_t>(i->b1.CwMask.u());
    res.persp_corr = i->PerspCorr();
    res.is_al = i->b3.AL.u();
    res.al_index = i->ALIndex();
    res.input_attr = i->InputAttr();
    res.is_last = i->b0.LastInstr.u();
    bool has_const = false;
    for (int n = 0; n < opcode.op_count; ++n) {
        auto& arg = res.arguments[n];
        arg.type =  i->OpType(n);
        arg.reg_num = i->OpRegNum(n);
        arg.reg_type = i->OpRegType(n);
        arg.is_neg = i->OpNeg(n);
        arg.is_abs = i->OpAbs(n);
        arg.swizzle.comp[0] = i->OpMaskX(n);
        arg.swizzle.comp[1] = i->OpMaskY(n);
        arg.swizzle.comp[2] = i->OpMaskZ(n);
        arg.swizzle.comp[3] = i->OpMaskW(n);
        if (arg.type == op_type_t::Const) {
            has_const = true;
            alignas(16) uint8_t buf[16];
            read_fragment_imm_val(instr + 16, buf);
            memcpy(&arg.imm_val.f[0], buf, sizeof buf);
        }
    }
    return has_const ? 32 : 16;
}

std::string print_swizzle(swizzle_t swizzle, bool allComponents) {
    return print_swizzle(
        allComponents,
        (int)swizzle.comp[0],
        (int)swizzle.comp[1],
        (int)swizzle.comp[2],
        (int)swizzle.comp[3]
    );
}

struct vertex_raw_instr_t {
    union {
        BitField<1, 2> mask_selector;
        BitField<2, 3> sets_c;
        BitField<3, 4> output_has_comlpex_offset;
        BitField<4, 5> v_index_has_displ;
        BitField<5, 6> is_sat;
        BitField<6, 7> cond_is_c1;
        BitField<7, 8> is_addr_reg;
        BitField<8, 9> is_arg2_abs;
        BitField<9, 10> is_arg1_abs;
        BitField<10, 11> is_arg0_abs;
        BitField<11, 17> addr_or_data_reg_num;
        BitField<17, 18> flag2;
        BitField<18, 19> has_cond;
        BitField<19, 22> cond_relation;
        BitField<22, 24> cond_sw_x;
        BitField<24, 26> cond_sw_y;
        BitField<26, 28> cond_sw_z;
        BitField<28, 30> cond_sw_w;
        BitField<30, 32> displ_component;
    } b0;
    union {
        BitField<0, 5> opcode2;
        BitField<5, 10> opcode1;
        BitField<10, 20> input_v_num;
        BitField<20, 24> v_displ;
        BitField<24, 25> arg0_is_neg;
        BitField<25, 27> arg0_sw_x;
        BitField<27, 29> arg0_sw_y;
        BitField<29, 31> arg0_sw_z;
        BitField<31, 32> arg0_sw_w1;
    } b1;
    union {
        BitField<0, 1> arg0_sw_w0;
        BitField<1, 7> arg0_reg_num;
        BitField<7, 9> arg0_reg_type;
        BitField<9, 10> arg1_is_neg;
        BitField<10, 12> arg1_sw_x;
        BitField<12, 14> arg1_sw_y;
        BitField<14, 16> arg1_sw_z;
        BitField<16, 18> arg1_sw_w;
        BitField<18, 24> arg1_reg_num;
        BitField<24, 26> arg1_reg_type;
        BitField<26, 27> arg2_is_neg;
        BitField<27, 29> arg2_sw_x;
        BitField<29, 31> arg2_sw_y;
        BitField<31, 32> arg2_sw_z1;
    } b2;
    union {
        BitField<0, 1> arg2_sw_z0;
        BitField<1, 3> arg2_sw_w;
        BitField<3, 9> arg2_reg_num;
        BitField<9, 11> arg2_reg_type;
        BitField<11, 15> mask2;
        BitField<15, 19> mask1;
        BitField<19, 25> dest_reg_num;
        BitField<25, 30> output_reg_num;
        BitField<30, 31> is_comp_offset;
        BitField<31, 32> is_last;
    } b3;
};

void vertex_dasm_instr(const uint8_t* instr, vertex_decoded_instr_t& res) {
    uint8_t buf[16];
    for (auto i = 0u; i < sizeof(buf); i += 4) {
        buf[i + 0] = instr[i + 3];
        buf[i + 1] = instr[i + 2];
        buf[i + 2] = instr[i + 1];
        buf[i + 3] = instr[i + 0];
    }
    auto typed = (const vertex_raw_instr_t*)buf;
    
    res.label = typed->b3.arg2_sw_w.u()
              | (((typed->b2.arg2_sw_z1.u() << 1) | typed->b3.arg2_sw_z0.u()) << 2)
              | (typed->b2.arg2_sw_y.u() << 4)
              | (typed->b2.arg2_sw_x.u() << 6)
              | (typed->b2.arg2_is_neg.u() << 8)
              | (typed->b0.is_arg2_abs.u() << 9);
    res.has_cond = typed->b0.has_cond.u();
    res.is_last = typed->b3.is_last.u();
    res.is_complex_offset = typed->b3.is_comp_offset.u();
    res.output_reg_num = typed->b3.output_reg_num.u();
    res.dest_reg_num = typed->b3.dest_reg_num.u();
    res.addr_data_reg_num = typed->b0.addr_or_data_reg_num.u();
    res.mask1 = typed->b3.mask1.u();
    res.mask2 = typed->b3.mask2.u();
    res.v_displ = typed->b1.v_displ.u();
    res.input_v_num = typed->b1.input_v_num.u();
    res.opcode1 = typed->b1.opcode1.u();
    res.opcode2 = typed->b1.opcode2.u();
    res.disp_component = typed->b0.displ_component.u();
    res.cond_swizzle.comp[0] = (swizzle2bit_t)typed->b0.cond_sw_x.u();
    res.cond_swizzle.comp[1] = (swizzle2bit_t)typed->b0.cond_sw_y.u();
    res.cond_swizzle.comp[2] = (swizzle2bit_t)typed->b0.cond_sw_z.u();
    res.cond_swizzle.comp[3] = (swizzle2bit_t)typed->b0.cond_sw_w.u();
    res.cond_relation = (cond_t)typed->b0.cond_relation.u();
    res.flag2 = typed->b0.flag2.u();
    res.is_addr_reg = typed->b0.is_addr_reg.u();
    res.is_cond_c1 = typed->b0.cond_is_c1.u();
    res.is_sat = typed->b0.is_sat.u();
    res.v_index_has_displ = typed->b0.v_index_has_displ.u();
    res.output_has_complex_offset = typed->b0.output_has_comlpex_offset.u();
    res.sets_c = typed->b0.sets_c.u();
    res.mask_selector = typed->b0.mask_selector.u();
    res.args[2].is_neg = typed->b2.arg2_is_neg.u();
    res.args[2].is_abs = typed->b0.is_arg2_abs.u();
    res.args[2].swizzle.comp[0] = (swizzle2bit_t)typed->b2.arg2_sw_x.u();
    res.args[2].swizzle.comp[1] = (swizzle2bit_t)typed->b2.arg2_sw_y.u();
    res.args[2].swizzle.comp[2] = (swizzle2bit_t)((typed->b2.arg2_sw_z1.u() << 1) |
                                                   typed->b3.arg2_sw_z0.u());
    res.args[2].swizzle.comp[3] = (swizzle2bit_t)typed->b3.arg2_sw_w.u();
    res.args[2].reg_num = typed->b3.arg2_reg_num.u();
    res.args[2].reg_type = typed->b3.arg2_reg_type.u();
    res.args[1].is_neg = typed->b2.arg1_is_neg.u();
    res.args[1].is_abs = typed->b0.is_arg1_abs.u();
    res.args[1].swizzle.comp[0] = (swizzle2bit_t)typed->b2.arg1_sw_x.u();
    res.args[1].swizzle.comp[1] = (swizzle2bit_t)typed->b2.arg1_sw_y.u();
    res.args[1].swizzle.comp[2] = (swizzle2bit_t)typed->b2.arg1_sw_z.u();
    res.args[1].swizzle.comp[3] = (swizzle2bit_t)typed->b2.arg1_sw_w.u();
    res.args[1].reg_num = typed->b2.arg1_reg_num.u();
    res.args[1].reg_type = typed->b2.arg1_reg_type.u();
    res.args[0].is_neg = typed->b1.arg0_is_neg.u();
    res.args[0].is_abs = typed->b0.is_arg0_abs.u();
    res.args[0].swizzle.comp[0] = (swizzle2bit_t)typed->b1.arg0_sw_x.u();
    res.args[0].swizzle.comp[1] = (swizzle2bit_t)typed->b1.arg0_sw_y.u();
    res.args[0].swizzle.comp[2] = (swizzle2bit_t)typed->b1.arg0_sw_z.u();
    res.args[0].swizzle.comp[3] = (swizzle2bit_t)((typed->b1.arg0_sw_w1.u() << 1) |
                                                   typed->b2.arg0_sw_w0.u());
    res.args[0].reg_num = typed->b2.arg0_reg_num.u();
    res.args[0].reg_type = typed->b2.arg0_reg_type.u();
}

enum class vertex_argument_type_t {
    none, v, cc, b, s, a, t
};

struct vertex_opcode_t {
    int op_count; // remove
    bool control; // remove
    bool addr_reg; // remove
    bool tex; // remove
    
    bool cond;
    
    const char* mnemonic;
    vertex_op_t instr;
    vertex_argument_type_t output_type;
    vertex_argument_type_t input_type[3];
};

#define OUT(a) vertex_argument_type_t::a
#define IN(a) { vertex_argument_type_t::a }
#define IN2(a,b) { vertex_argument_type_t::a, vertex_argument_type_t::b }
#define IN3(a,b,c) \
    { vertex_argument_type_t::a, \
      vertex_argument_type_t::b, \
      vertex_argument_type_t::c }
#define OP(a) #a, vertex_op_t::a

vertex_opcode_t vertex_opcodes_1[] = {
    { 0, 0, 0, 0, 0, OP(NOP) },
    { 1, 0, 0, 0, 1, OP(MOV), OUT(v), IN(v) },
    { 2, 0, 0, 0, 1, OP(MUL), OUT(v), IN2(v, v) },
    { 2, 0, 0, 0, 1, OP(ADD), OUT(v), IN2(v, v) },
    { 3, 0, 0, 0, 1, OP(MAD), OUT(v), IN3(v, v, v) },
    { 2, 0, 0, 0, 1, OP(DP3), OUT(s), IN2(v, v) },
    { 2, 0, 0, 0, 1, OP(DPH), OUT(s), IN2(v, v) },
    { 2, 0, 0, 0, 1, OP(DP4), OUT(s), IN2(v, v) },
    { 2, 0, 0, 0, 1, OP(DST), OUT(v), IN2(v, v) },
    { 2, 0, 0, 0, 1, OP(MIN), OUT(v), IN2(v, v) },
    { 2, 0, 0, 0, 1, OP(MAX), OUT(v), IN2(v, v) },
    { 2, 0, 0, 0, 1, OP(SLT), OUT(v), IN2(v, v) },
    { 2, 0, 0, 0, 1, OP(SGE), OUT(v), IN2(v, v) },
    { 1, 0, 1, 0, 1, OP(ARL), OUT(a), IN(v) },
    { 1, 0, 0, 0, 1, OP(FRC), OUT(v), IN(v) },
    { 1, 0, 0, 0, 1, OP(FLR), OUT(v), IN(v) },
    { 2, 0, 0, 0, 1, OP(SEQ), OUT(v), IN2(v, v) },
    { 0, 0, 0, 0, 1, OP(SFL), OUT(v) },
    { 2, 0, 0, 0, 1, OP(SGT), OUT(v), IN2(v, v) },
    { 2, 0, 0, 0, 1, OP(SLE), OUT(v), IN2(v, v) },
    { 2, 0, 0, 0, 1, OP(SNE), OUT(v), IN2(v, v) },
    { 0, 0, 0, 0, 1, OP(STR), OUT(v) },
    { 1, 0, 0, 0, 1, OP(SSG), OUT(v), IN(v) },
    { 1, 0, 1, 0, 1, OP(ARR), OUT(a), IN(v) },
    { 1, 0, 1, 0, 1, OP(ARA), OUT(a), IN(v) },
    { 2, 0, 0, 1, 0, OP(TXL), OUT(s), IN2(v, t) },
    { 1, 0, 0, 0, 0, OP(PSH) },
    { 0, 0, 0, 0, 0, OP(POP) }
};

vertex_opcode_t vertex_opcodes_0[] = {
    { 0, 0, 0, 0, 1, OP(NOP) },
    { 1, 0, 0, 0, 1, OP(MOV), OUT(v), IN(v) },
    { 1, 0, 0, 0, 1, OP(RCP), OUT(s), IN(s) },
    { 1, 0, 0, 0, 1, OP(RCC) },
    { 1, 0, 0, 0, 1, OP(RSQ), OUT(s), IN(s) },
    { 1, 0, 0, 0, 1, OP(EXP), OUT(v), IN(s) },
    { 1, 0, 0, 0, 1, OP(LOG), OUT(v), IN(s) },
    { 1, 0, 0, 0, 1, OP(LIT), OUT(v), IN(v) },
    { 1, 1, 0, 0, 1, OP(BRA) },
    { 1, 1, 0, 0, 0, OP(BRI), OUT(none), IN(cc) },
    { 1, 1, 0, 0, 1, OP(CAL) },
    { 1, 1, 0, 0, 0, OP(CLI), OUT(cc) },
    { 0, 1, 0, 0, 0, OP(RET), OUT(none), IN(cc) },
    { 1, 0, 0, 0, 1, OP(LG2), OUT(s), IN(s) },
    { 1, 0, 0, 0, 1, OP(EX2), OUT(s), IN(s) },
    { 1, 0, 0, 0, 1, OP(SIN), OUT(s), IN(s) },
    { 1, 0, 0, 0, 1, OP(COS), OUT(s), IN(s) },
    { 1, 0, 0, 0, 0, OP(BRB), OUT(b) },
    { 1, 0, 0, 0, 0, OP(CLB), OUT(b) },
    { 1, 0, 0, 0, 1, OP(PSH) },
    { 1, 0, 0, 0, 1, OP(POP) }
};


// TODO: add MVA

const char* displ_components = "xyzw";

std::string print_mask(dest_mask_t mask) {
    if (mask.val[0] && mask.val[1] && mask.val[2] && mask.val[3])
        return "";
    std::string res = ".";
    if (mask.val[0])
        res += "x";
    if (mask.val[1])
        res += "y";
    if (mask.val[2])
        res += "z";
    if (mask.val[3])
        res += "w";
    return res;
}

std::string print_mask(uint8_t mask) {
    if (mask == 0xf)
        return "";
    std::string res = ".";
    if (mask & 8)
        res += "x";
    if (mask & 4)
        res += "y";
    if (mask & 2)
        res += "z";
    if (mask & 1)
        res += "w";
    return res;
}

int vertex_dasm_slot(vertex_decoded_instr_t instr, int slot, VertexInstr& res) {
    const vertex_opcode_t* opcode;
    const vertex_decoded_arg_t *args[] = {
        &instr.args[0], &instr.args[1], &instr.args[2]
    };
    if (slot) {
        opcode = &vertex_opcodes_1[instr.opcode1];
        if (opcode->instr == vertex_op_t::ADD) {
            args[1] = &instr.args[2];
        }
    } else {
        opcode = &vertex_opcodes_0[instr.opcode2];
        args[0] = &instr.args[2];
    }
    res.is_last = instr.is_last;
    res.is_branch = opcode->control;
    res.is_sat = instr.is_sat;
    res.operation = opcode->instr;
    res.mnemonic = opcode->mnemonic;
    res.control = control_mod_t::None;
    if (instr.flag2 && !(slot ^ instr.sets_c)) {
        res.control = instr.is_cond_c1 ?
            control_mod_t::C1 :
            control_mod_t::C0;
    }
    int res_arg = 0;
    
    if (opcode->output_type != vertex_argument_type_t::none) {
        if (instr.output_reg_num != 31) {
            if (!(slot ^ instr.mask_selector)) {
                int regnum = instr.output_reg_num & 0x1f;
                vertex_arg_ref_t ref;
                if (instr.output_has_complex_offset) {
                    ref = vertex_arg_address_ref { 
                        instr.is_addr_reg, instr.disp_component, regnum
                    };
                } else {
                    ref = regnum;
                }
                auto mask = instr.mask_selector ? instr.mask1 : instr.mask2;
                res.mask = dest_mask_t::fromInt16(mask);
                res.args[res_arg] = vertex_arg_output_ref_t { ref };
                res_arg++;
            }
        } 
        if (!opcode->control && !slot && opcode->instr != vertex_op_t::PSH) {
            if (instr.addr_data_reg_num != 63 || instr.mask_selector || instr.output_reg_num == 31) {
                auto mask = opcode->instr == vertex_op_t::PSH ? 0xf : instr.mask2;
                if (instr.dest_reg_num == 63) {
                    res.mask = dest_mask_t::fromInt16(mask);
                    res.args[res_arg] = vertex_arg_cond_reg_ref_t { 0 };
                } else if (opcode->addr_reg) {
                    res.args[res_arg] = vertex_arg_address_reg_ref_t { instr.dest_reg_num, mask };
                } else {
                    res.mask = dest_mask_t::fromInt16(mask);
                    res.args[res_arg] = vertex_arg_temp_reg_ref_t { 
                        instr.dest_reg_num,
                        false,
                        false,
                        swizzle_xyzw
                    };
                }
                res_arg++;
            }
        } else if (instr.addr_data_reg_num != 63) {
            auto mask = opcode->instr == vertex_op_t::PSH ? 0xf : instr.mask1;
            if (opcode->addr_reg) {
                res.args[res_arg] = vertex_arg_address_reg_ref_t { instr.addr_data_reg_num, mask };
            } else {
                res.mask = dest_mask_t::fromInt16(mask);
                res.args[res_arg] = vertex_arg_temp_reg_ref_t { 
                    instr.addr_data_reg_num,
                    false,
                    false,
                    swizzle_xyzw
                };
            }
            res_arg++;
        } else if ((!instr.mask_selector || instr.output_reg_num == 31) && !opcode->control) {
            auto mask = opcode->instr == vertex_op_t::PSH ? 0xf : instr.mask1;
            res.mask = dest_mask_t::fromInt16(mask);
            res.args[res_arg] = vertex_arg_cond_reg_ref_t { 0 };
            res_arg++;
        }
    }
    
    res.condition.relation = cond_t::TR;
    if (instr.has_cond || opcode->control) {
        res.condition.relation = instr.cond_relation;
        res.condition.swizzle = instr.cond_swizzle;
        res.condition.is_C1 = instr.is_cond_c1;
    } else if (opcode->instr == vertex_op_t::BRB || opcode->instr == vertex_op_t::CLB) {
//             res += " ";
//             if (!(instr.args[0].reg_num & 0x20)) {
//                 res += "!";
//             }
//             res += ssnprintf("b%d,", instr.args[0].reg_num & 0x1f);
    }
    
    for (auto i = 0; i < opcode->op_count; ++i, ++res_arg) {
        auto arg = args[i];
        if (opcode->control) {
            res.args[res_arg] = vertex_arg_label_ref_t { instr.label };
        } else if (opcode->instr == vertex_op_t::ARA || opcode->instr == vertex_op_t::PSH) {
            res.args[res_arg] = vertex_arg_address_reg_ref_t { instr.is_addr_reg };
        } else {
            if (i == 1 && opcode->tex) {
                res.args[res_arg] = vertex_arg_tex_ref_t { args[1]->reg_num & 3 };
            } else {
                if (arg->reg_type == 2) {
                    vertex_arg_ref_t ref;
                    if (instr.v_index_has_displ) {
                        ref = vertex_arg_address_ref { 
                            instr.is_addr_reg, instr.disp_component, (int)instr.v_displ
                        };
                    } else {
                        ref = instr.v_displ;
                    }
                    res.args[res_arg] = vertex_arg_input_ref_t {
                        ref, arg->is_neg, arg->is_abs, arg->swizzle
                    };
                } else if (arg->reg_type == 3) {
                    vertex_arg_ref_t ref;
                    if (instr.is_complex_offset) {
                        ref = vertex_arg_address_ref { 
                            instr.is_addr_reg, instr.disp_component, instr.input_v_num
                        };
                    } else {
                        ref = instr.input_v_num;
                    }
                    res.args[res_arg] = vertex_arg_const_ref_t {
                        ref, arg->is_neg, arg->is_abs, arg->swizzle
                    };
                } else {
                    res.args[res_arg] = vertex_arg_temp_reg_ref_t {
                        arg->reg_num, arg->is_neg, arg->is_abs, arg->swizzle
                    };
                }
            }
        }
    }
    
    return res_arg;
}

int vertex_dasm_instr(const uint8_t* instr, 
                      std::array<VertexInstr, 2>& res, 
                      vertex_decoded_instr_t* decodedInstr) {
    vertex_decoded_instr_t decoded;
    vertex_dasm_instr(instr, decoded);
    int i = 0;
    if (decoded.opcode2) {
        res[i].op_count = vertex_dasm_slot(decoded, 0, res[i]);
        i++;
    }
    if (decoded.opcode1 || !decoded.opcode2) {
        res[i].op_count = vertex_dasm_slot(decoded, 1, res[i]);
        i++;
    }
    if (decodedInstr) {
        *decodedInstr = decoded;
    }
    if (i == 2 && res[0].is_branch) {
        // combined instructions are always executed together
        // if a branch comes first it may prevent the second instruction
        // so always place the branch last
        std::swap(res[0], res[1]);
    }
    return i;
}

class ref_visitor : public boost::static_visitor<std::string> {
public:
    std::string operator()(vertex_arg_address_ref x) const {
        return ssnprintf("A%d.%c+%d", x.reg, displ_components[x.component], x.displ);
    }
    
    std::string operator()(int x) const {
        return ssnprintf("%d", x);
    }
};

class arg_visitor : public boost::static_visitor<std::string> {
    const VertexInstr* _instr;
    bool _noMask;
    
public:
    arg_visitor(const VertexInstr* instr, bool noMask)
        : _instr(instr), _noMask(noMask) {}
    
    std::string operator()(vertex_arg_output_ref_t x) const {
        auto ref = apply_visitor(ref_visitor(), x.ref);
        return ssnprintf("o[%s]%s", ref, _noMask ? "" : print_mask(_instr->mask));
    }
    
    std::string print(char reg, vertex_arg_ref_t ref, bool neg, bool abs, swizzle_t sw) const {
        auto refstr = apply_visitor(ref_visitor(), ref);
        auto res = ssnprintf("%c[%s]", reg, refstr);
        if (abs)
            res = ssnprintf("|%s|", res);
        if (neg)
            res = "-" + res;
        return res + print_swizzle(sw, false);
    }
    
    std::string operator()(vertex_arg_input_ref_t x) const {
        return print('v', x.ref, x.is_neg, x.is_abs, x.swizzle);
    }
    
    std::string operator()(vertex_arg_const_ref_t x) const {
        return print('c', x.ref, x.is_neg, x.is_abs, x.swizzle);
    }
    
    std::string operator()(vertex_arg_temp_reg_ref_t x) const {
        auto ref = apply_visitor(ref_visitor(), x.ref);
        auto res = ssnprintf("R%s", ref);
        if (x.is_abs)
            res = ssnprintf("|%s|", res);
        if (x.is_neg)
            res = "-" + res;
        auto mask = _noMask ? "" : print_mask(_instr->mask);
        auto swizzle = print_swizzle(x.swizzle, false);
        return res + mask + swizzle;
    }
    
    std::string operator()(vertex_arg_address_reg_ref_t x) const {
        return ssnprintf("A%d%s", x.a, _noMask ? "" : print_mask(_instr->mask));
    }
    
    std::string operator()(vertex_arg_cond_reg_ref_t x) const {
        return ssnprintf("RC%s", _noMask ? "" : print_mask(_instr->mask));
    }
    
    std::string operator()(vertex_arg_label_ref_t x) const {
        return ssnprintf("L%d", x.l);
    }
    
    std::string operator()(vertex_arg_tex_ref_t x) const {
        return ssnprintf("TEX%d", x.tex);
    }
};

std::string vertex_dasm(const VertexInstr& instr) {
    std::string res = instr.mnemonic;
    if (instr.is_branch) {
        res += ssnprintf("(%s%s) %s", 
                         print_cond(instr.condition.relation),
                         print_swizzle(instr.condition.swizzle, false),
                         apply_visitor(arg_visitor(&instr, false), instr.args[0]));
    } else {
        if (instr.control != control_mod_t::None) {
            res += "C";
            if (instr.control == control_mod_t::C1) {
                res += "1";
            }
        }
        res += " ";
        for (int i = 0; i < instr.op_count; ++i) {
            if (i > 0) {
                res += ", ";
            }
            res += apply_visitor(arg_visitor(&instr, i != 0), instr.args[i]);
            if (i == 0 && instr.condition.relation != cond_t::TR) {
                auto cond = print_cond(instr.condition.relation);
                auto sw = print_swizzle(instr.condition.swizzle, false);
                res += ssnprintf("(%s%s)", cond, sw);
            }
        }
    }
    return res + ";";
}

FragmentProgramInfo get_fragment_bytecode_info(const uint8_t* ptr) {
    auto pos = 0u;
    std::bitset<512> map;
    while (pos < 512 * 16) {
        bool isConst, isLast;
        fragment_scan_instr(ptr + pos, isConst, isLast);
        map[pos / 16 + 1] = isConst;
        pos += isConst ? 32 : 16;
        if (isLast) {
            break;
        }
    }
    return { map, pos };
}

uint32_t get_fragment_bytecode_length(const uint8_t* ptr) {
    // TODO: optimize
    return get_fragment_bytecode_info(ptr).length;
}
