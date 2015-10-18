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

enum class fragment_op_t {
    NOP, MOV, MUL, ADD, MAD, DP3, DP4, DST, MIN, MAX, SLT,
    SGE, SLE, SGT, SNE, SEQ, FRC, FLR, KIL, PK4, UP4, DDX,
    DDY, TEX, TXP, TXD, RCP, RSQ, EX2, LG2, LIT, LRP, STR,
    SFL, COS, SIN, PK2, UP2, POW, PKB, UPB, PK16, UP16,
    BEM, PKG, UPG, DP2A, TXL, TXB, TEXBEM, TXPBEM, BEMLUM,
    REFL, TIMESWTEX, DP2, NRM, DIV, DIVSQ, LIF, FENCT,
    FENCB, BRK, CAL, IFE, LOOP, REP, RET
};

enum class swizzle_type_t {
    rgba, xyzw
};

struct swizzle_t {
    swizzle_type_t type;
    int components[4];
};

struct FragmentCondition {
    bool is_present;
    cond_t relation;
    bool is_C1;
    swizzle_t swizzle;
};

struct fragment_argument_t {
    op_type_t type;
    swizzle_t swizzle;
    int id;
};

struct fragment_opcode_t {
    int op_count;
    bool control;
    bool nop;
    bool tex;
    const char* mnemonic;
    fragment_op_t instr;
};

struct FragmentInstr {
    fragment_opcode_t opcode;
    FragmentCondition condition;
    clamp_t clamp;
    fragment_argument_t arguments[3];
};

int fragment_dasm(uint8_t* instr, std::string& res);
FragmentInstr fragment_dasm_instr(uint8_t* instr);
