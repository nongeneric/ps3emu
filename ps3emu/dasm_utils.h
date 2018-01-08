#pragma once

#include "utils.h"
#include "ps3emu/BitField.h"
#include <boost/type_traits.hpp>
#include <string>
#include <vector>
#include <optional>
#include <tuple>

enum class DasmMode {
    Print, Emulate, Name, Rewrite
};

inline std::string format_u(const char* mnemonic, uint64_t u) {
    return ssnprintf("%s %" PRIx64, mnemonic, u);
}

template <typename OP1>
inline std::string format_n(const char* mnemonic, OP1 op1) {
    return ssnprintf("%s %s%d", mnemonic, op1.prefix(), op1.native());
}

template <typename OP1, typename OP2>
inline std::string format_nn(const char* mnemonic, OP1 op1, OP2 op2) {
    return ssnprintf("%s %s%d,%s%d", mnemonic, 
                     op1.prefix(), op1.native(), 
                     op2.prefix(), op2.native());
}

template <typename OP1>
inline std::string format_nu(const char* mnemonic, OP1 op1, uint64_t u) {
    return ssnprintf("%s %s%d,0x%" PRIx64, mnemonic, 
                     op1.prefix(), op1.native(), 
                     u);
}

template <typename OP1, typename OP2, typename OP3>
inline std::string format_nnn(const char* mnemonic, OP1 op1, OP2 op2, OP3 op3) {
    return ssnprintf("%s %s%d,%s%d,%s%d", mnemonic, 
                     op1.prefix(), op1.native(), 
                     op2.prefix(), op2.native(),
                     op3.prefix(), op3.native()
                    );
}

template <typename OP1, typename OP2>
inline std::string format_nnu(const char* mnemonic, OP1 op1, OP2 op2, uint64_t u) {
    return ssnprintf("%s %s%d,%s%d,%" PRId64, mnemonic, 
                     op1.prefix(), op1.native(), 
                     op2.prefix(), op2.native(),
                     u
    );
}

template <typename OP1, typename OP2>
inline std::string format_nnuu(const char* mnemonic, OP1 op1, OP2 op2, uint64_t u1, uint64_t u2) {
    return ssnprintf("%s %s%d,%s%d,%" PRId64 ",%" PRId64, mnemonic, 
                     op1.prefix(), op1.native(), 
                     op2.prefix(), op2.native(),
                     u1, u2
    );
}

template <typename OP1, typename OP2, typename OP3>
inline std::string format_br_nnn(const char* mnemonic, OP1 op1, OP2 op2, OP3 op3) {
    return ssnprintf("%s %s%d,%s%d(%s%d)", mnemonic, 
                     op1.prefix(), op1.native(), 
                     op2.prefix(), op2.native(),
                     op3.prefix(), op3.native()
    );
}

template <typename OP1, typename OP2, typename OP3, typename OP4>
inline std::string format_nnnn(const char* mnemonic, OP1 op1, OP2 op2, OP3 op3, OP4 op4) {
    return ssnprintf("%s %s%d,%s%d,%s%d,%s%d", mnemonic, 
                     op1.prefix(), op1.native(), 
                     op2.prefix(), op2.native(),
                     op3.prefix(), op3.native(),
                     op4.prefix(), op4.native()
    );
}

template <typename OP1, typename OP2, typename OP3, typename OP4, typename OP5>
inline std::string format_nnnnn(const char* mnemonic, OP1 op1, OP2 op2, OP3 op3, OP4 op4, OP5 op5) {
    return ssnprintf("%s %s%d,%s%d,%s%d,%s%d,%s%d", mnemonic, 
                     op1.prefix(), op1.native(), 
                     op2.prefix(), op2.native(),
                     op3.prefix(), op3.native(),
                     op4.prefix(), op4.native(),
                     op5.prefix(), op5.native()
    );
}

template <typename T>
std::string printSorU(T v) {
    if (std::is_signed<T>::value) {
        return ssnprintf("%d", v);
    }
    return ssnprintf("0x%xu", v);
}

inline std::string print_args(std::vector<std::string> vec) {
    std::string res;
    for (auto i = 0u; i < vec.size(); ++i) {
        if (i) {
            res += ",";
        }
        res += vec[i];
    }
    return res;
}

template <typename... Ts>
std::string rewrite_print(const char* mnemonic, Ts... ts) {
    return ssnprintf("_%s(%s)", mnemonic, print_args(std::vector<std::string>{ printSorU(ts)... }));
}

template <DasmMode M, typename P, typename E, typename S, typename R>
void invoke_impl(const char* name, P* phandler, E* ehandler, R* rhandler, void* instr, uint64_t cia, S* s) {
    typedef typename boost::function_traits<P>::arg1_type F;
    typedef typename boost::function_traits<P>::arg3_type PS;
    typedef typename boost::function_traits<E>::arg3_type ES;
    typedef typename boost::function_traits<R>::arg3_type RS;
    if (M == DasmMode::Print)
        phandler(reinterpret_cast<F>(instr), cia, reinterpret_cast<PS>(s));
    if (M == DasmMode::Emulate)
        ehandler(reinterpret_cast<F>(instr), cia, reinterpret_cast<ES>(s));
    if (M == DasmMode::Name)
        *reinterpret_cast<std::string*>(s) = name;
    if (M == DasmMode::Rewrite)
        rhandler(reinterpret_cast<F>(instr), cia, reinterpret_cast<RS>(s));
}

enum class OperandType {
    none, imm, ppu_r, ppu_v, spu_r
};

struct OperandInfo {
    OperandType type;
    uint32_t num = 0;
    uint32_t imm = 0;
};

inline bool operator<(OperandInfo const& left, OperandInfo const& right) {
    return std::tie(left.type, left.num, left.imm) <
           std::tie(right.type, right.num, right.imm);
}

struct InstructionInfo {
    bool flow = false;
    bool passthrough = false;
    std::optional<uint32_t> target;
    bool ncall = 0;
    bool extInfo = false; // TODO: provide extended info for all instructions
    std::vector<OperandInfo> inputs;
    std::vector<OperandInfo> outputs;
};

#define NCALL_OPCODE 1
#define BB_CALL_OPCODE 0b111101
#define SPU_BB_CALL_OPCODE 0b100100

union BBCallForm {
    uint32_t val;
    BitField<0, 6> OPCD;
    BitField<6, 16> Segment;
    BitField<16, 32> Label;
};

inline uint32_t asm_bb_call(unsigned opcode, uint32_t segment, uint32_t label) {
    assert((1u << decltype(BBCallForm::Segment)::W) > segment);
    assert((1u << decltype(BBCallForm::Label)::W) > label);
    BBCallForm instr { 0 };
    instr.OPCD.set(opcode);
    instr.Segment.set(segment);
    instr.Label.set(label);
    return instr.val;
}

inline bool dasm_bb_call(unsigned opcode, uint32_t instr, uint32_t& segment, uint32_t& label) {
    BBCallForm form { instr };
    if (form.OPCD.u() == opcode) {
        segment = form.Segment.u();
        label = form.Label.u();
        return true;
    }
    return false;
}
