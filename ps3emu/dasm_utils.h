#pragma once

#include "utils.h"
#include "ppu/ppu_dasm.h"
#include <boost/type_traits.hpp>
#include <string>

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

template <DasmMode M, typename P, typename E, typename S>
void invoke_impl(const char* name, P* phandler, E* ehandler, void* instr, uint64_t cia, S* s) {
    typedef typename boost::function_traits<P>::arg1_type F;
    typedef typename boost::function_traits<P>::arg3_type PS;
    typedef typename boost::function_traits<E>::arg3_type ES;
    if (M == DasmMode::Print)
        phandler(reinterpret_cast<F>(instr), cia, reinterpret_cast<PS>(s));
    if (M == DasmMode::Emulate)
        ehandler(reinterpret_cast<F>(instr), cia, reinterpret_cast<ES>(s));
    if (M == DasmMode::Name)
        *reinterpret_cast<std::string*>(s) = name;
}