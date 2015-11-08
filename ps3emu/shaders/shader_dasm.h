#pragma once

#include "../BitField.h"
#include <string>
#include <stdint.h>
#include <array>
#include <vector>
#include <boost/variant.hpp>

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

struct swizzle_t {
    swizzle2bit_t comp[4];
    inline bool is_xyzw() const {
        return comp[0] == swizzle2bit_t::X
            && comp[1] == swizzle2bit_t::Y
            && comp[2] == swizzle2bit_t::Z
            && comp[3] == swizzle2bit_t::W;
    }
};

constexpr swizzle_t swizzle_xyzw = {
    swizzle2bit_t::X,
    swizzle2bit_t::Y,
    swizzle2bit_t::Z,
    swizzle2bit_t::W
};

struct condition_t {
    cond_t relation;
    bool is_C1;
    swizzle_t swizzle;
};

struct fragment_argument_t {
    op_type_t type;
    bool is_neg;
    bool is_abs;
    swizzle_t swizzle;
    int reg_num;
    reg_type_t reg_type;
    uint32_t imm_val[4];
};

struct fragment_opcode_t {
    int op_count;
    bool control;
    bool nop;
    bool tex;
    const char* mnemonic;
    fragment_op_t instr;
};

enum class vertex_op_t {
    MUL, ADD, MAD, DP3, DPH, DP4, DST, MIN, MAX, SLT,
    SGE, ARL, FRC, FLR, SEQ, SFL, SGT, SLE, SNE, STR,
    SSG, ARR, ARA, TXL, NOP, MOV, POP, RCP, RCC, RSQ,
    EXP, LOG, LIT, BRA, BRI, CAL, CLI, RET, LG2, EX2,
    SIN, COS, BRB, CLB, PSH, MVA
};

struct dest_mask_t {
    bool val[4];
};

struct FragmentInstr {
    fragment_opcode_t opcode;
    condition_t condition;
    clamp_t clamp;
    control_mod_t control;
    scale_t scale;
    dest_mask_t dest_mask;
    bool is_bx2;
    bool is_sat;
    reg_type_t reg;
    bool is_reg_c;
    int reg_num;
    persp_corr_t persp_corr;
    bool is_al;
    int al_index;
    input_attr_t input_attr;
    bool is_last;
    fragment_argument_t arguments[3];
};

struct vertex_arg_address_ref {
    int a;
    int component;
    int d;
};

typedef boost::variant<vertex_arg_address_ref, int> vertex_arg_ref_t;

struct vertex_arg_tex_ref_t {
    int tex;
};

struct vertex_arg_label_ref_t {
    int l;
};

struct vertex_arg_cond_reg_ref_t {
    int c;
    int mask;
};

struct vertex_arg_address_reg_ref_t {
    int a;
    int mask;
};

struct vertex_arg_output_ref_t {
    vertex_arg_ref_t ref;
    int mask;
};

struct vertex_arg_input_ref_t {
    vertex_arg_ref_t ref;
    bool is_neg;
    bool is_abs;
    swizzle_t swizzle;
};

struct vertex_arg_const_ref_t {
    vertex_arg_ref_t ref;
    bool is_neg;
    bool is_abs;
    swizzle_t swizzle;
};

struct vertex_arg_temp_reg_ref_t {
    vertex_arg_ref_t ref;
    bool is_neg;
    bool is_abs;
    swizzle_t swizzle;
    int mask;
};

typedef boost::variant<
    vertex_arg_output_ref_t,
    vertex_arg_input_ref_t,
    vertex_arg_const_ref_t,
    vertex_arg_temp_reg_ref_t,
    vertex_arg_address_reg_ref_t,
    vertex_arg_cond_reg_ref_t,
    vertex_arg_label_ref_t,
    vertex_arg_tex_ref_t
> VertexVariantArg;

struct VertexInstr {
    int op_count;
    bool is_sat;
    VertexVariantArg args[4];
    vertex_op_t operation;
    const char* mnemonic;
    bool is_last;
    control_mod_t control;
    condition_t condition;
};

std::string print_dest_mask(dest_mask_t mask);
const char* print_attr(input_attr_t attr);
std::string print_swizzle(swizzle_t swizzle, bool allComponents);
void fragment_dasm(FragmentInstr const& i, std::string& res);
int fragment_dasm_instr(const uint8_t* instr, FragmentInstr& res);
int vertex_dasm_instr(const uint8_t* instr, std::array<VertexInstr, 2>& res);
std::string vertex_dasm(VertexInstr const& instr);
