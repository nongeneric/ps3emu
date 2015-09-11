#pragma once

#include "BitField.h"
#include <string>
#include <stdint.h>

enum class clamp_t {
    R, H, X, B
};

enum class control_mod_t {
    None, C0, C1
};

enum class scale_t {
    None, M2, M4, M8, Illegal, D2, D4, D8
};

enum class reg_type_t {
    R, H
};

enum class cond_t {
    FL, LT, EQ, LE, GT, NE, GE, TR
};

enum class persp_corr_t {
    F, G
};

enum class swizzle2bit_t {
    X, Y, Z, W
};

enum class input_attr_t {
   WPOS, COL0, COL1, FOGC,
   TEX0, TEX1, TEX2, TEX3,
   TEX4, TEX5, TEX6, TEX7,
   TEX8, TEX9, SSA, v15
};

enum class op_type_t {
    Temp, Attr, Const, Illegal
};

struct fragment_instr_t {
    union {
        BitField<0, 1> RegType;
        BitField<1, 7> RegNum;
        BitField<7, 8> LastInstr;
        BitField<11, 12> Op0xMask;
        BitField<12, 13> Op0yMask;
        BitField<13, 14> Op0zMask;
        BitField<14, 15> Op0wMask;
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
        
        BitField<23, 25> CyMask;
        BitField<25, 27> CxMask;
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
};

int fragment_dasm(uint8_t* instr, std::string& res);
