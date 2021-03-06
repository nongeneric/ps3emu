#pragma once

#include "ps3emu/BitField.h"
#include "ps3emu/utils.h"
#include <array>
#include <bitset>
#include <boost/align.hpp>
#include <boost/variant.hpp>
#include <stdint.h>
#include <string>
#include <vector>
#include <x86intrin.h>

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

enum class input_modifier_t {
    _R, _H, _X, _B, _S, _N
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
    union {
        std::array<uint32_t, 4> u;
        std::array<float, 4> f;
    } imm_val;
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
    static dest_mask_t fromInt16(uint16_t mask) {
        return { (mask & 8) != 0, (mask & 4) != 0, (mask & 2) != 0, (mask & 1) != 0 };
    }

    std::array<bool, 4> val;
    inline int arity() const {
        return val[0] + val[1] + val[2] + val[3];
    }
};

struct FragmentInstr {
    uint32_t elseLabel;
    uint32_t endifLabel;
    fragment_opcode_t opcode;
    condition_t condition;
    input_modifier_t input_modifier;
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
    int tex_num;
};

struct vertex_arg_address_ref {
    int reg;
    int component;
    int displ;
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
    swizzle_t mask;
};

struct vertex_arg_address_reg_ref_t {
    int a;
    int mask;
};

struct vertex_arg_output_ref_t {
    vertex_arg_ref_t ref;
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
    dest_mask_t mask;
    vertex_op_t operation;
    const char* mnemonic;
    bool is_last;
    control_mod_t control;
    condition_t condition;
    bool is_branch;
};

struct vertex_decoded_arg_t {
    bool is_neg;
    bool is_abs;
    swizzle_t swizzle;
    int reg_num;
    int reg_type;
};

struct vertex_decoded_instr_t {
    bool is_last;
    bool is_complex_offset;
    int output_reg_num;
    int dest_reg_num;
    int addr_data_reg_num;
    uint16_t mask1;
    uint16_t mask2;
    vertex_decoded_arg_t args[3];
    uint32_t v_displ;
    int input_v_num;
    int opcode1;
    int opcode2;
    int disp_component;
    swizzle_t cond_swizzle;
    cond_t cond_relation;
    bool flag2;
    bool has_cond;
    bool is_addr_reg;
    bool is_cond_c1;
    bool is_sat;
    bool v_index_has_displ;
    bool output_has_complex_offset;
    bool sets_c;
    int mask_selector;
    int label;
};

const char* print_cond(cond_t cond);
std::string print_dest_mask(dest_mask_t mask);
const char* print_attr(input_attr_t attr);
std::string print_swizzle(swizzle_t swizzle, bool allComponents);
void fragment_dasm(FragmentInstr const& i, std::string& res);
int fragment_dasm_instr(const uint8_t* instr, FragmentInstr& res);
int vertex_dasm_instr(const uint8_t* instr,
                      std::array<VertexInstr, 2>& res,
                      vertex_decoded_instr_t* decodedInstr = nullptr);
std::string vertex_dasm(VertexInstr const& instr);

inline void rsx_load16(const void* ptr, void* outp, bool twoBytes) {
    auto mask =
        twoBytes
            ? _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2)
            : _mm_set_epi8(14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1);
    auto val = _mm_lddqu_si128((__m128i*)ptr);
    auto res = _mm_shuffle_epi8(val, mask);
    assert(boost::alignment::is_aligned(reinterpret_cast<uintptr_t>(outp), sizeof res));
    _mm_store_si128((__m128i*)outp, res);
}

inline void read_fragment_imm_val(const void* ptr, void* outp) {
    return rsx_load16(ptr, outp, false);
}

inline void read_fragment_instr(const void* ptr, void* outp) {
    return rsx_load16(ptr, outp, true);
}

struct FragmentProgramInfo {
    std::bitset<512> constMap; // 1 means const
    unsigned length;
};

FragmentProgramInfo get_fragment_bytecode_info(const uint8_t* ptr);
uint32_t get_fragment_bytecode_length(const uint8_t* ptr);
