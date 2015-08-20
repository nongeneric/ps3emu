#pragma once

#include "../ps3emu/PPU.h"
#include "ppu_dasm_forms.h"

#include <boost/type_traits.hpp>
#include <boost/endian/conversion.hpp>
#include <string>
#include <stdint.h>
#include <bitset>
#include <stdio.h>
#include <cinttypes>

using namespace boost::endian;

#define PRINT(name, form) inline void print##name(form* i, uint64_t cia, std::string* result)
#define EMU(name, form) inline void emulate##name(form* i, uint64_t cia, PPU* ppu)

template <typename... Args>
std::string ssnprintf(const char* f, Args... args) {
    char buf[30];
    snprintf(buf, sizeof buf, f, args...);
    return std::string(buf);
}

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
    return ssnprintf("%s %s%d,%s%d,%s%d", mnemonic, 
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

// Branch I-form, p24

inline uint64_t getNIA(IForm* i, uint64_t cia) {
    auto ext = i->LI.native();
    return i->AA.u() ? ext : (cia + ext);
}

PRINT(B, IForm) {
    const char* mnemonics[][2] = {
        { "b", "ba" }, { "bl", "bla" }  
    };
    auto mnemonic = mnemonics[i->LK.u()][i->AA.u()];
    *result = format_u(mnemonic, getNIA(i, cia));
}

EMU(B, IForm) {
    ppu->setNIP(getNIA(i, cia));
    if (i->LK.u())
        ppu->setLR(cia + 4);
}

// Branch Conditional B-form, p24

enum class BranchMnemonicType {
    ExtSimple, ExtCondition, Generic
};

BranchMnemonicType getExtBranchMnemonic(
    bool lr, bool abs, bool tolr, bool toctr, BO_t btbo, BI_t bi, std::string& mnemonic);
std::string formatCRbit(BI_t bi);

inline uint64_t getNIA(BForm* i, uint64_t cia) {
    auto ext = i->BD << 2;
    return i->AA.u() ? ext : (ext + cia);
}

PRINT(BC, BForm) {
    std::string extMnemonic;
    auto mtype = getExtBranchMnemonic(
        i->LK.u(), i->AA.u(), false, false, i->BO, i->BI, extMnemonic);
    if (mtype == BranchMnemonicType::Generic) {
        const char* mnemonics[][2] = {
            { "bc", "bca" }, { "bcl", "bcla" }  
        };
        auto mnemonic = mnemonics[i->LK.u()][i->AA.u()];
        *result = ssnprintf("%s %d,%s,%x", 
                            mnemonic, i->BO.u(), formatCRbit(i->BI).c_str(), getNIA(i, cia));
    } else if (i->BI.u() > 3) {
        if (mtype == BranchMnemonicType::ExtCondition) {
            *result = ssnprintf("%s cr%d,%" PRIx64,
                                extMnemonic.c_str(), i->BI.u() / 4, getNIA(i, cia));
        } else if (mtype == BranchMnemonicType::ExtSimple) {
            *result = ssnprintf("%s %s,%" PRIx64,
                                extMnemonic.c_str(), formatCRbit(i->BI).c_str(), getNIA(i, cia));
        }
    } else {
        *result = format_u(extMnemonic.c_str(), getNIA(i, cia));
    }
}

template <int P1, int P2, int P3, int P4>
inline bool isTaken(BitField<P1, P1 + 1> bo0, 
                    BitField<P2, P2 + 1> bo1,
                    BitField<P3, P3 + 1> bo2,
                    BitField<P4, P4 + 1> bo3,
                    PPU* ppu,
                    BI_t bi)
{
    auto ctr_ok = bo2.u() | ((ppu->getCTR() != 0) ^ bo3.u());
    auto cond_ok = bo0.u() | (bit_test(ppu->getCR(), 32, bi) == bo1.u());
    return ctr_ok && cond_ok;
}

EMU(BC, BForm) {
    if (!i->BO2.u())
        ppu->setCTR(ppu->getCTR() - 1);
    if (isTaken(i->BO0, i->BO1, i->BO2, i->BO3, ppu, i->BI))
        ppu->setNIP(getNIA(i, cia));
    if (i->LK.u())
        ppu->setLR(cia + 4);
}

// Branch Conditional to Link Register XL-form, p25

template <typename BF>
inline int64_t getB(BF ra, PPU* ppu) {
    return ra.u() == 0 ? 0 : ppu->getGPR(ra);
}

PRINT(BCLR, XLForm_2) {
    std::string extMnemonic;
    auto mtype = getExtBranchMnemonic(i->LK.u(), 0, true, false, i->BO, i->BI, extMnemonic);
    if (mtype == BranchMnemonicType::Generic) {
        auto mnemonic = i->LK.u() ? "bclrl" : "bclr";
        *result = ssnprintf("%s %d,%s,%d",
                            mnemonic, i->BO.u(), formatCRbit(i->BI).c_str(), i->BH.u());
    } else if (i->BI.u() > 3) {
        *result = ssnprintf("%s cr%d", extMnemonic.c_str(), i->BI.u() / 4);
    } else {
        *result = extMnemonic;
    }
}

EMU(BCLR, XLForm_2) {
    if (!i->BO2.u())
        ppu->setCTR(ppu->getCTR() - 1);
    if (isTaken(i->BO0, i->BO1, i->BO2, i->BO3, ppu, i->BI))
        ppu->setNIP(ppu->getLR() & ~3);
    if (i->LK.u())
        ppu->setLR(cia + 4);
}

// Branch Conditional to Count Register, p25

PRINT(BCCTR, XLForm_2) {
    std::string extMnemonic;
    auto mtype = getExtBranchMnemonic(i->LK.u(), 0, true, false, i->BO, i->BI, extMnemonic);
    if (mtype == BranchMnemonicType::Generic) {
        auto mnemonic = i->LK.u() ? "bcctrl" : "bcctr";
        *result = ssnprintf("%s %d,%s,%d",
                            mnemonic, i->BO.u(), formatCRbit(i->BI).c_str(), i->BH.u());
    } else if (i->BI.u() > 3) {
        *result = ssnprintf("%s cr%d", extMnemonic.c_str(), i->BI.u() / 4);
    } else {
        *result = extMnemonic;
    }
}

EMU(BCCTR, XLForm_2) {
    auto cond_ok = i->BO0.u() || bit_test(ppu->getCR(), 32, i->BI) == i->BO1.u();
    if (cond_ok)
        ppu->setNIP(ppu->getCTR() & ~3);
    if (i->LK.u())
        ppu->setLR(cia + 4);
}

// Condition Register AND, p28

PRINT(CRAND, XLForm_1) {
    *result = format_nnn("crand", i->BT, i->BA, i->BB);
}

EMU(CRAND, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) & bit_test(cr, i->BB));
    ppu->setCR(cr);
}

// Condition Register OR, p28

PRINT(CROR, XLForm_1) {
    *result = format_nnn("cror", i->BT, i->BA, i->BB);
}

EMU(CROR, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) | bit_test(cr, i->BB));
    ppu->setCR(cr);
}

// Condition Register XOR, p28

PRINT(CRXOR, XLForm_1) {
    *result = format_nnn("crxor", i->BT, i->BA, i->BB);
}

EMU(CRXOR, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) ^ bit_test(cr, i->BB));
    ppu->setCR(cr);
}

// Condition Register NAND, p28

PRINT(CRNAND, XLForm_1) {
    *result = format_nnn("crnand", i->BT, i->BA, i->BB);
}

EMU(CRNAND, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, !(bit_test(cr, i->BA) & bit_test(cr, i->BB)));
    ppu->setCR(cr);
}

// Condition Register NOR, p29

PRINT(CRNOR, XLForm_1) {
    *result = format_nnn("crnor", i->BT, i->BA, i->BB);
}

EMU(CRNOR, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, (!bit_test(cr, i->BA)) | bit_test(cr, i->BB));
    ppu->setCR(cr);
}

// Condition Register Equivalent, p29

PRINT(CREQV, XLForm_1) {
    *result = format_nnn("creqv", i->BT, i->BA, i->BB);
}

EMU(CREQV, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) == bit_test(cr, i->BB));
    ppu->setCR(cr);
}

// Condition Register AND with Complement, p29

PRINT(CRANDC, XLForm_1) {
    *result = format_nnn("crandc", i->BT, i->BA, i->BB);
}

EMU(CRANDC, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) & (!bit_test(cr, i->BB)));
    ppu->setCR(cr);
}

// Condition Register OR with Complement, p29

PRINT(CRORC, XLForm_1) {
    *result = format_nnn("crorc", i->BT, i->BA, i->BB);
}

EMU(CRORC, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) | (!bit_test(cr, i->BB)));
    ppu->setCR(cr);
}

// Condition Register Field Instruction, p30

PRINT(MCRF, XLForm_3) {
    *result = format_nn("mcrf", i->BF, i->BFA);
}

EMU(MCRF, XLForm_3) {
    auto cr = ppu->getCR();
    std::bitset<64> bs(cr);
    std::bitset<64> new_bs(cr);
    for (int j = 0; j <= 3; ++j) {
        new_bs[j*i->BF.u()] = bs[j*i->BFA.u()];
    }
    ppu->setCR(new_bs.to_ullong());
}

// Load Byte and Zero, p34

PRINT(LBZ, DForm_1) {
    *result = format_br_nnn("lbz", i->RT, i->D, i->RA);
}

EMU(LBZ, DForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + i->D;
    ppu->setGPR(i->RT, ppu->load<1>(ea));
}

// Load Byte and Zero, p34

PRINT(LBZX, XForm_1) {
    *result = format_nnn("lbzx", i->RT, i->RA, i->RB);
}

EMU(LBZX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RA);
    ppu->setGPR(i->RT, ppu->load<1>(ea));
}

// Load Byte and Zero with Update, p34

PRINT(LBZU, DForm_1) {
    *result = format_br_nnn("lbzu", i->RT, i->D, i->RA);
}

EMU(LBZU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + i->D;
    ppu->setGPR(i->RT, ppu->load<1>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Byte and Zero with Update Indexed, p34

PRINT(LBZUX, XForm_1) {
    *result = format_nnn("lbzux", i->RT, i->RA, i->RB);
}

EMU(LBZUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<1>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Halfword and Zero, p35

PRINT(LHZ, DForm_1) {
    *result = format_br_nnn("lhz", i->RT, i->D, i->RA);
}

EMU(LHZ, DForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + i->D;
    ppu->setGPR(i->RT, ppu->load<2>(ea));
}

// Load Halfword and Zero Indexed, p35

PRINT(LHZX, XForm_1) {
    *result = format_nnn("lhzx", i->RT, i->RA, i->RB);
}

EMU(LHZX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<2>(ea));
}

// Load Halfword and Zero with Update, p35

PRINT(LHZU, DForm_1) {
    *result = format_br_nnn("lhzu", i->RT, i->D, i->RA);
}

EMU(LHZU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + i->D;
    ppu->setGPR(i->RT, ppu->load<2>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Halfword and Zero with Update Indexed, p35

PRINT(LHZUX, XForm_1) {
    *result = format_nnn("lhzux", i->RT, i->RA, i->RB);
}

EMU(LHZUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<2>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Halfword Algebraic, p36

PRINT(LHA, DForm_1) {
    *result = format_br_nnn("lha", i->RT, i->D, i->RA);
}

EMU(LHA, DForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + i->D;
    ppu->setGPR(i->RT, ppu->loads<2>(ea));
}

// Load Halfword Algebraic Indexed, p36

PRINT(LHAX, XForm_1) {
    *result = format_nnn("LHAX", i->RT, i->RA, i->RB);
}

EMU(LHAX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->loads<2>(ea));
}

// Load Halfword Algebraic with Update, p36

PRINT(LHAU, DForm_1) {
    *result = format_br_nnn("lhaux", i->RT, i->D, i->RA);
}

EMU(LHAU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + i->D;
    ppu->setGPR(i->RT, ppu->loads<2>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Halfword Algebraic with Update Indexed, p36

PRINT(LHAUX, XForm_1) {
    *result = format_nnn("lhaux", i->RT, i->RA, i->RB);
}

EMU(LHAUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->loads<2>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Word and Zero, p37

PRINT(LWZ, DForm_1) {
    *result = format_br_nnn("lwz", i->RT, i->D, i->RA);
}

EMU(LWZ, DForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + i->D;
    ppu->setGPR(i->RT, ppu->load<4>(ea));
}

// Load Word and Zero Indexed, p37

PRINT(LWZX, XForm_1) {
    *result = format_nnn("lwzx", i->RT, i->RA, i->RB);
}

EMU(LWZX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<4>(ea));
}

// Load Word and Zero with Update, p37

PRINT(LWZU, DForm_1) {
    *result = format_br_nnn("lwzu", i->RT, i->D, i->RA);
}

EMU(LWZU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + i->D;
    ppu->setGPR(i->RT, ppu->load<4>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Word and Zero with Update Indexed, p37

PRINT(LWZUX, XForm_1) {
    *result = format_nnn("lwzux", i->RT, i->RA, i->RB);
}

EMU(LWZUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<4>(ea));
}

// Load Word Algebraic, p38

PRINT(LWA, DSForm_1) {
    *result = format_br_nnn("lwa", i->RT, i->DS, i->RA);
}

EMU(LWA, DSForm_1) {
    auto b = i->RA.u() == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + i->DS.native();
    ppu->setGPR(i->RT, ppu->loads<4>(ea));
}

// Load Word Algebraic Indexed, p38

PRINT(LWAX, XForm_1) {
    *result = format_nnn("lwax", i->RT, i->RA, i->RB);
}

EMU(LWAX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->loads<4>(ea));
}

// Load Word Algebraic with Update Indexed, p38

PRINT(LWAUX, XForm_1) {
    *result = format_nnn("lwaux", i->RT, i->RA, i->RB);
}

EMU(LWAUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->loads<4>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Doubleword, p39

PRINT(LD, DSForm_1) {
    *result = format_br_nnn("ld", i->RT, i->DS, i->RA);
}

EMU(LD, DSForm_1) {
    auto b = i->RA.u() == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + i->DS.native();
    ppu->setGPR(i->RT, ppu->load<8>(ea));
}

// Load Doubleword Indexed, p39

PRINT(LDX, XForm_1) {
    *result = format_nnn("ldx", i->RT, i->RA, i->RB);
}

EMU(LDX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<8>(ea));
}

// Load Doubleword with Update, p39

PRINT(LDU, DSForm_1) {
    *result = format_br_nnn("ldu", i->RT, i->DS, i->RA);
}

EMU(LDU, DSForm_1) {
    auto ea = ppu->getGPR(i->RA) + i->DS.native();
    ppu->setGPR(i->RT, ppu->load<8>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Doubleword with Update Indexed, p39

PRINT(LDUX, XForm_1) {
    *result = format_nnn("ldux", i->RT, i->RA, i->RB);
}

EMU(LDUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<8>(ea));
    ppu->setGPR(i->RA, ea);
}

// STORES
//

template <int Bytes>
void EmuStore(DForm_3* i, PPU* ppu) {
    auto b = getB(i->RA, ppu);
    auto ea = b + i->D;
    ppu->store<Bytes>(ea, ppu->getGPR(i->RS));
}

template <int Bytes>
void EmuStoreIndexed(XForm_8* i, PPU* ppu) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->store<1>(ea, ppu->getGPR(i->RS));
}

template <int Bytes>
void EmuStoreUpdate(DForm_3* i, PPU* ppu) {
    auto ea = ppu->getGPR(i->RA) + i->D;
    ppu->store<Bytes>(ea, ppu->getGPR(i->RS));
    ppu->setGPR(i->RA, ea);
}

template <int Bytes>
void EmuStoreUpdateIndexed(XForm_8* i, PPU* ppu) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->store<Bytes>(ea, ppu->getGPR(i->RS));
    ppu->setGPR(i->RA, ea);
}

inline void PrintStore(const char* mnemonic, DForm_3* i, std::string* result) {
    *result = format_br_nnn(mnemonic, i->RS, i->D, i->RA);
}

inline void PrintStoreIndexed(const char* mnemonic, XForm_8* i, std::string* result) {
    *result = format_nnn(mnemonic, i->RS, i->RA, i->RB);
}

PRINT(STB, DForm_3) { PrintStore("stbx", i, result); }
EMU(STB, DForm_3) { EmuStore<1>(i, ppu); }
PRINT(STBX, XForm_8) { PrintStoreIndexed("stbx", i, result); }
EMU(STBX, XForm_8) { EmuStoreIndexed<1>(i, ppu); }
PRINT(STBU, DForm_3) { PrintStore("stbu", i, result); }
EMU(STBU, DForm_3) { EmuStoreUpdate<1>(i, ppu); }
PRINT(STBUX, XForm_8) { PrintStoreIndexed("stbux", i, result); }
EMU(STBUX, XForm_8) { EmuStoreUpdateIndexed<1>(i, ppu); }

PRINT(STH, DForm_3) { PrintStore("sth", i, result); }
EMU(STH, DForm_3) { EmuStore<2>(i, ppu); }
PRINT(STHX, XForm_8) { PrintStoreIndexed("sthx", i, result); }
EMU(STHX, XForm_8) { EmuStoreIndexed<2>(i, ppu); }
PRINT(STHU, DForm_3) { PrintStore("sthu", i, result); }
EMU(STHU, DForm_3) { EmuStoreUpdate<2>(i, ppu); }
PRINT(STHUX, XForm_8) { PrintStoreIndexed("sthux", i, result); }
EMU(STHUX, XForm_8) { EmuStoreUpdateIndexed<2>(i, ppu); }

PRINT(STW, DForm_3) { PrintStore("stw", i, result); }
EMU(STW, DForm_3) { EmuStore<4>(i, ppu); }
PRINT(STWX, XForm_8) { PrintStoreIndexed("stwx", i, result); }
EMU(STWX, XForm_8) { EmuStoreIndexed<4>(i, ppu); }
PRINT(STWU, DForm_3) { PrintStore("stwu", i, result); }
EMU(STWU, DForm_3) { EmuStoreUpdate<4>(i, ppu); }
PRINT(STWUX, XForm_8) { PrintStoreIndexed("stwux", i, result); }
EMU(STWUX, XForm_8) { EmuStoreUpdateIndexed<4>(i, ppu); }

PRINT(STD, DSForm_2) { 
    *result = format_br_nnn("std", i->RS, i->DS, i->RA);
}

EMU(STD, DSForm_2) { 
    auto b = getB(i->RA, ppu);
    auto ea = b + i->DS.native();
    ppu->store<8>(ea, ppu->getGPR(i->RS));
}

PRINT(STDX, XForm_8) { PrintStoreIndexed("stdx", i, result); }
EMU(STDX, XForm_8) { EmuStoreIndexed<8>(i, ppu); }

PRINT(STDU, DSForm_2) { 
    *result = format_br_nnn("stdu", i->RS, i->DS, i->RA);
}

EMU(STDU, DSForm_2) { 
    auto ea = ppu->getGPR(i->RA) + i->DS.native();
    ppu->store<8>(ea, ppu->getGPR(i->RS));
    ppu->setGPR(i->RA, ea);
}

PRINT(STDUX, XForm_8) { PrintStoreIndexed("stdux", i, result); }
EMU(STDUX, XForm_8) { EmuStoreUpdateIndexed<8>(i, ppu); }

// Fixed-Point Load and Store with Byte Reversal Instructions, p44

template <int Bytes>
void EmuLoadByteReverseIndexed(XForm_1* i, PPU* ppu) {
    auto b = getB(i->RA, ppu);
    auto ea = (b << 1) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, endian_reverse(ppu->load<Bytes>(ea)));
}

PRINT(LHBRX, XForm_1) { *result = format_nnn("lhbrx", i->RT, i->RA, i->RB); }
EMU(LHBRX, XForm_1) { EmuLoadByteReverseIndexed<2>(i, ppu); }
PRINT(LWBRX, XForm_1) { *result = format_nnn("lwbrx", i->RT, i->RA, i->RB); }
EMU(LWBRX, XForm_1) { EmuLoadByteReverseIndexed<4>(i, ppu); }

template <int Bytes>
void EmuStoreByteReverseIndexed(XForm_8* i, PPU* ppu) {
    auto b = getB(i->RA, ppu);
    auto ea = (b << 1) + ppu->getGPR(i->RB);
    ppu->store<Bytes>(ea, endian_reverse(i->RS.u()));
}

PRINT(STHBRX, XForm_8) { *result = format_nnn("sthbrx", i->RS, i->RA, i->RB); }
EMU(STHBRX, XForm_8) { EmuStoreByteReverseIndexed<2>(i, ppu); }
PRINT(STWBRX, XForm_8) { *result = format_nnn("stwbrx", i->RS, i->RA, i->RB); }
EMU(STWBRX, XForm_8) { EmuStoreByteReverseIndexed<4>(i, ppu); }

// Fixed-Point Arithmetic Instructions, p51

PRINT(ADDI, DForm_2) {
    *result = format_nnn("addi", i->RT, i->RA, i->SI);
}

EMU(ADDI, DForm_2) {
    auto b = getB(i->RA, ppu);
    ppu->setGPR(i->RT, i->SI + b);
}

PRINT(ADDIS, DForm_2) {
    *result = format_nnn("addis", i->RT, i->RA, i->SI);
}

EMU(ADDIS, DForm_2) {
    auto b = getB(i->RA, ppu);
    ppu->setGPR(i->RT, (i->SI << 16) + b);
}

inline void update_CR0(int64_t result, PPU* ppu) {
    auto s = result < 0 ? 4
           : result > 0 ? 2
           : 1;
    ppu->setCR0_sign(s);
}

template <int P1, int P2>
inline void update_CR0_OV(BitField<P1, P1 + 1> oe, 
                          BitField<P2, P2 + 1> rc, 
                          bool ov, int64_t result, PPU* ppu) {
    if (oe.u() && ov) {
        ppu->setOV();
    }
    if (rc.u()) {
        update_CR0(result, ppu);
    }
}

PRINT(ADD, XOForm_1) {
    const char* mnemonics[][2] = {
        { "add", "add." }, { "addo", "addo." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(ADD, XOForm_1) {
    auto ra = ppu->getGPR(i->RA);
    auto rb = ppu->getGPR(i->RB);
    int64_t res;
    bool ov = 0;
    if (i->OE.u())
        ov = __builtin_saddll_overflow(rb, ra, (long long int*)&res);
    else
        res = ra + rb;
    ppu->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, ppu);
}

PRINT(SUBF, XOForm_1) {
    const char* mnemonics[][2] = {
        { "subf", "subf." }, { "subfo", "subfo." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(SUBF, XOForm_1) {
    auto ra = ppu->getGPR(i->RA);
    auto rb = ppu->getGPR(i->RB);
    int64_t res;
    bool ov = 0;
    if (i->OE.u())
        ov = __builtin_ssubll_overflow(rb, ra, (long long int*)&res);
    else
        res = ra + rb;
    ppu->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, ppu);
}

// Fixed-Point Logical Instructions, p65

// AND

PRINT(ANDID, DForm_4) {
    *result = format_nnn("andi.", i->RA, i->RS, i->UI);
}

EMU(ANDID, DForm_4) {
    auto res = ppu->getGPR(i->RS) & i->UI.u();
    update_CR0(res, ppu);
    ppu->setGPR(i->RA, res);
}

PRINT(ANDISD, DForm_4) {
    *result = format_nnn("andis.", i->RA, i->RS, i->UI);
}

EMU(ANDISD, DForm_4) {
    auto res = ppu->getGPR(i->RS) & (i->UI << 16);
    update_CR0(res, ppu);
    ppu->setGPR(i->RA, res);
}

// OR

PRINT(ORI, DForm_4) {
    *result = format_nnn("ori", i->RA, i->RS, i->UI);
}

EMU(ORI, DForm_4) {
    auto res = ppu->getGPR(i->RS) | i->UI.u();
    ppu->setGPR(i->RA, res);
}

PRINT(ORIS, DForm_4) {
    *result = format_nnn("oris", i->RA, i->RS, i->UI);
}

EMU(ORIS, DForm_4) {
    auto res = ppu->getGPR(i->RS) | (i->UI << 16);
    ppu->setGPR(i->RA, res);
}

// XOR

PRINT(XORI, DForm_4) {
    *result = format_nnn("xori", i->RA, i->RS, i->UI);
}

EMU(XORI, DForm_4) {
    auto res = ppu->getGPR(i->RS) ^ i->UI.u();
    ppu->setGPR(i->RA, res);
}

PRINT(XORIS, DForm_4) {
    *result = format_nnn("xoris", i->RA, i->RS, i->UI);
}

EMU(XORIS, DForm_4) {
    auto res = ppu->getGPR(i->RS) ^ (i->UI << 16);
    ppu->setGPR(i->RA, res);
}

// X-forms

PRINT(AND, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "and." : "and", i->RA, i->RS, i->RB);
}

EMU(AND, XForm_6) {
    auto res = ppu->getGPR(i->RS) & ppu->getGPR(i->RB);
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(OR, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "or." : "or", i->RA, i->RS, i->RB);
}

EMU(OR, XForm_6) {
    auto res = ppu->getGPR(i->RS) | ppu->getGPR(i->RB);
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(XOR, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "xor." : "xor", i->RA, i->RS, i->RB);
}

EMU(XOR, XForm_6) {
    auto res = ppu->getGPR(i->RS) ^ ppu->getGPR(i->RB);
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(NAND, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "nand." : "nand", i->RA, i->RS, i->RB);
}

EMU(NAND, XForm_6) {
    auto res = ~(ppu->getGPR(i->RS) & ppu->getGPR(i->RB));
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(NOR, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "nor." : "nor", i->RA, i->RS, i->RB);
}

EMU(NOR, XForm_6) {
    auto res = ~(ppu->getGPR(i->RS) | ppu->getGPR(i->RB));
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(EQV, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "eqv." : "eqv", i->RA, i->RS, i->RB);
}

EMU(EQV, XForm_6) {
    auto res = ~(ppu->getGPR(i->RS) ^ ppu->getGPR(i->RB));
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(ANDC, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "andc." : "andc", i->RA, i->RS, i->RB);
}

EMU(ANDC, XForm_6) {
    auto res = ppu->getGPR(i->RS) & ~ppu->getGPR(i->RB);
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(ORC, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "orc." : "orc", i->RA, i->RS, i->RB);
}

EMU(ORC, XForm_6) {
    auto res = ppu->getGPR(i->RS) | ~ppu->getGPR(i->RB);
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

// Extend Sign

PRINT(EXTSB, XForm_11) {
    *result = format_nn(i->Rc.u() ? "extsb." : "extsb", i->RA, i->RS);
}

EMU(EXTSB, XForm_11) {
    ppu->setGPR(i->RA, (int64_t)static_cast<int8_t>(ppu->getGPR(i->RS)));
}

PRINT(EXTSH, XForm_11) {
    *result = format_nn(i->Rc.u() ? "extsh." : "extsh", i->RA, i->RS);
}

EMU(EXTSH, XForm_11) {
    ppu->setGPR(i->RA, (int64_t)static_cast<int16_t>(ppu->getGPR(i->RS)));
}

PRINT(EXTSW, XForm_11) {
    *result = format_nn(i->Rc.u() ? "extsw." : "extsw", i->RA, i->RS);
}

EMU(EXTSW, XForm_11) {
    ppu->setGPR(i->RA, (int64_t)static_cast<int32_t>(ppu->getGPR(i->RS)));
}

// Move To/From System Register Instructions, p81

inline bool isXER(BitField<11, 21> spr) {
    return spr.u() == 1 << 5;
}

inline bool isLR(BitField<11, 21> spr) {
    return spr.u() == 8 << 5;
}

inline bool isCTR(BitField<11, 21> spr) {
    return spr.u() == 9 << 5;
}

PRINT(MTSPR, XFXForm_7) {
    auto mnemonic = isXER(i->spr) ? "mtxer"
                  : isLR(i->spr) ? "mtlr"
                  : isCTR(i->spr) ? "mtctr"
                  : nullptr;
    if (mnemonic) {
        *result = format_n(mnemonic, i->RS);
    } else {
        throw std::runtime_error("illegal");
    }
}

EMU(MTSPR, XFXForm_7) {
    auto rs = ppu->getGPR(i->RS);
    if (isXER(i->spr)) {
        ppu->setXER(rs);
    } else if (isLR(i->spr)) {
        ppu->setLR(rs);
    } else if (isCTR(i->spr)) {
        ppu->setCTR(rs);
    } else {
        throw std::runtime_error("illegal");
    }
}

PRINT(MFSPR, XFXForm_7) {
    auto mnemonic = isXER(i->spr) ? "mfxer"
                  : isLR(i->spr) ? "mflr"
                  : isCTR(i->spr) ? "mfctr"
    : nullptr;
    if (mnemonic) {
        *result = format_n(mnemonic, i->RS);
    } else {
        throw std::runtime_error("illegal");
    }
}

EMU(MFSPR, XFXForm_7) {
    auto v = isXER(i->spr) ? ppu->getXER()
           : isLR(i->spr) ? ppu->getLR()
           : isCTR(i->spr) ? ppu->getCTR()
           : (throw std::runtime_error("illegal"), 0);
     ppu->setGPR(i->RS, v);
}

// PRINT(MTCRF, XFXForm_5) {
//     
// }

template <int Pos04, int Pos5>
inline uint8_t getNBE(BitField<Pos04, Pos04 + 5> _04, BitField<Pos5, Pos5 + 1> _05) {
    return (_05.u() << 5) | _04.u();
}

inline uint64_t ror(uint64_t n, uint8_t s) {
    asm("rol %1,%0" : "+r" (n) : "c" (s));
    return n;
}

inline uint64_t rol(uint64_t n, uint8_t s) {
    asm("rol %1,%0" : "+r" (n) : "c" (s));
    return n;
}

inline uint32_t rol32(uint32_t n, uint8_t s) {
    asm("roll %1,%0" : "+r" (n) : "c" (s));
    return n;
}

PRINT(RLDICL, MDForm_1) {
    auto n = getNBE(i->sh04, i->sh5);
    auto b = getNBE(i->mb04, i->mb5);
    if (b == 0 && n > 32) {
        *result = format_nnu(i->Rc.u() ? "rotrdi." : "rotrdi", i->RA, i->RS, 64 - n);
    } else if (b == 0 && n <= 32 ) {
        *result = format_nnu(i->Rc.u() ? "rotldi." : "rotldi", i->RA, i->RS, n);
    } else if (b == 64 - n) {
        *result = format_nnu(i->Rc.u() ? "srdi." : "srdi", i->RA, i->RS, b);
    } else if (n == 0) {
        *result = format_nnu(i->Rc.u() ? "clrldi." : "clrldi", i->RA, i->RS, b);
    } else if (64 - b > 0) {
        *result = format_nnuu(i->Rc.u() ? "extrdi." : "extrdi", i->RA, i->RS, 64 - b, n + b - 64);
    } else {
        *result = format_nnuu(i->Rc.u() ? "rldicl." : "rldicl", i->RA, i->RS, b + n, 64 - n);
    }
}

EMU(RLDICL, MDForm_1) {
    auto n = getNBE(i->sh04, i->sh5);
    auto b = getNBE(i->mb04, i->mb5);
    auto r = rol(ppu->getGPR(i->RS), n);
    auto m = mask<64>(b, 63);
    auto res = r & m;
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(RLDICR, MDForm_2) {
    auto n = getNBE(i->sh04, i->sh5);
    auto b = getNBE(i->me04, i->me5);
    if (63 - b == n) {
        *result = format_nnu(i->Rc.u() ? "sldi." : "sldi", i->RA, i->RS, n);
    } else if (n == 0) {
        *result = format_nnu(i->Rc.u() ? "clrrdi." : "clrrdi", i->RA, i->RS, 63 - b);
    } else if (b > 1) {
        *result = format_nnuu(i->Rc.u() ? "extldi." : "extldi", i->RA, i->RS, b + 1, n);
    } else {
        *result = format_nnuu(i->Rc.u() ? "rldicr." : "rldicr", i->RA, i->RS, b + n, 64 - n);
    }
}

EMU(RLDICR, MDForm_2) {
    auto n = getNBE(i->sh04, i->sh5);
    auto e = getNBE(i->me04, i->me5);
    auto r = rol(ppu->getGPR(i->RS), n);
    auto m = mask<64>(0, e);
    auto res = r & m;
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(RLDIMI, MDForm_1) {
    auto n = getNBE(i->sh04, i->sh5);
    auto b = getNBE(i->mb04, i->mb5);
    if (n + b < 64) {
        *result = format_nnuu(i->Rc.u() ? "insrdi." : "insrdi", i->RA, i->RS, 64 - (n + b), b);
    } else {
        *result = format_nnuu(i->Rc.u() ? "rldimi." : "rldimi", i->RA, i->RS, n, b);
    }
}

EMU(RLDIMI, MDForm_1) {
    auto n = getNBE(i->sh04, i->sh5);
    auto b = getNBE(i->mb04, i->mb5);
    auto r = rol(ppu->getGPR(i->RS), n);
    auto m = mask<64>(b, ~n);
    auto res = (r & m) | (ppu->getGPR(i->RA) & (~m));
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(RLDCL, MDSForm_1) {
    if (i->mb.u() == 0) {
        *result = format_nnn(i->Rc.u() ? "rotld." : "rotld", i->RA, i->RS, i->RB);
    } else {
        *result = format_nnnn(i->Rc.u() ? "rotld." : "rotld", i->RA, i->RS, i->RB, i->mb);
    }
}
    
EMU(RLDCL, MDSForm_1) {
    auto n = ppu->getGPR(i->RB) & 63;
    auto r = rol(ppu->getGPR(i->RS), n);
    auto b = i->mb.u();
    auto m = mask<64>(b, 63);
    auto res = r & m;
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(RLDCR, MDSForm_2) {
    *result = format_nnnn(i->Rc.u() ? "rldcr." : "rldcr", i->RA, i->RS, i->RB, i->me);
}

EMU(RLDCR, MDSForm_2) {
    auto n = ppu->getGPR(i->RB) & 63;
    auto r = rol(ppu->getGPR(i->RS), n);
    auto e = i->me.u();
    auto m = mask<64>(0, e);
    auto res = r & m;
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(RLDIC, MDForm_1) {
    auto n = getNBE(i->sh04, i->sh5);
    auto b = getNBE(i->mb04, i->mb5);
    if (n <= b + n && b + n < 64) {
        *result = format_nnuu(i->Rc.u() ? "clrlsldi." : "clrlsldi", i->RA, i->RS, b + n, n);
    } else {
        *result = format_nnuu(i->Rc.u() ? "rldic." : "rldic", i->RA, i->RS, n, b);   
    }
}

EMU(RLDIC, MDForm_1) {
    auto n = getNBE(i->sh04, i->sh5);
    auto b = getNBE(i->mb04, i->mb5);    
    auto r = rol(ppu->getGPR(i->RS), n);
    auto m = mask<64>(b, ~n);
    auto res = r & m;
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(RLWINM, MForm_2) {
    auto s = i->SH.u();
    auto b = i->mb.u();
    auto e = i->me.u();
    if (b == 0 && e == 31) {
        if (s < 16) {
            *result = format_nnn(i->Rc.u() ? "rotlwi." : "rotlwi", i->RA, i->RS, i->SH);
        } else {
            *result = format_nnu(i->Rc.u() ? "rotrwi." : "rotrwi", i->RA, i->RS, 32 - s);
        }
    } else if (b == 0 && e == 31 - s) {
        *result = format_nnu(i->Rc.u() ? "slwi." : "slwi", i->RA, i->RS, s);
    } else if (e == 31 && 32 - s == b) {
        *result = format_nnu(i->Rc.u() ? "srwi." : "srwi", i->RA, i->RS, b);
    } else if (s == 0 && e == 31) {
        *result = format_nnu(i->Rc.u() ? "clrlwi." : "clrlwi", i->RA, i->RS, b);
    } else if (s == 0 && b == 0) {
        *result = format_nnu(i->Rc.u() ? "clrrwi." : "clrrwi", i->RA, i->RS, 31 - e);
    } else if (e == 31 - s && s <= s + b && s + b < 32) {
        *result = format_nnuu(i->Rc.u() ? "clrlslwi." : "clrlslwi", i->RA, i->RS, s + b, s);
    } else if (b == 0 && e <= 30) {
        *result = format_nnuu(i->Rc.u() ? "extlwi." : "extlwi", i->RA, i->RS, e + 1, s);
    } else if (e == 31) {
        *result = format_nnuu(i->Rc.u() ? "extrwi." : "extrwi", i->RA, i->RS, 32 - b, s + b - 32);
    } else {
        *result = format_nnnnn(i->Rc.u() ? "rlwinm." : "rlwinm", i->RA, i->RS, i->SH, i->mb, i->me);
    }
}

EMU(RLWINM, MForm_2) {
    auto n = i->SH.u();
    auto r = rol32(ppu->getGPR(i->RS), n);
    auto m = mask<64>(i->mb.u() + 32, i->me.u() + 32);
    auto res = r & m;
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(RLWNM, MForm_1) {
    if (i->mb.u() == 0 && i->me.u() == 31) {
        *result = format_nnn(i->Rc.u() ? "rotlw." : "rotlw", i->RA, i->RS, i->RB);
    } else {
        *result = format_nnnnn(i->Rc.u() ? "rlwnm." : "rlwnm", i->RA, i->RS, i->RB, i->mb, i->me);
    }
}

EMU(RLWNM, MForm_1) {
    auto n = ppu->getGPR(i->RB) & 31;
    auto r = rol32(ppu->getGPR(i->RS), n);
    auto m = mask<64>(i->mb.u() + 32, i->me.u() + 32);
    auto res = r & m;
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(RLWIMI, MForm_2) {
    auto s = i->SH.u();
    auto b = i->mb.u();
    auto e = i->me.u();
    if (s == 32 - b && b <= e) {
        *result = format_nnuu(i->Rc.u() ? "inslwi." : "inslwi", i->RA, i->RS, e - b + 1, b);
    } else if (32 - s == e + 1 && e + 1 > b) {
        *result = format_nnuu(i->Rc.u() ? "insrwi." : "insrwi", i->RA, i->RS, e + 1 - b, b);
    } else {
        *result = format_nnnnn(i->Rc.u() ? "rlwimi." : "rlwimi", i->RA, i->RS, i->SH, i->mb, i->me);
    }
}

EMU(RLWIMI, MForm_2) {
    auto n = i->SH.u();
    auto r = rol32(ppu->getGPR(i->RS), n);
    auto m = mask<64>(i->mb.u() + 32, i->me.u() + 32);
    auto res = (r & m) | (ppu->getGPR(i->RA) & ~m);
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(SC, SCForm) {
    *result = "sc";
}

EMU(SC, SCForm) {
    ppu->scall();
}

PRINT(NCALL, NCallForm) {
    *result = format_u("ncall", i->idx.u());
}

EMU(NCALL, NCallForm) {
    ppu->ncall(i->idx.u());
}

template <int LPos>
inline int64_t get_cmp_ab(BitField<LPos, LPos + 1> l, uint64_t value) {
    return l.u() == 0 ? (int64_t)static_cast<int32_t>(value) : value;
}

PRINT(CMPI, DForm_5) {
    if (i->BF.u() == 0 && i->L.u() == 1) {
        *result = format_nn("cmpdi", i->RA, i->SI);
    } else if (i->L.u() == 0) {
        *result = format_nnn("cmpwi", i->BF, i->RA, i->SI);
    } else {
        *result = format_nnnn("cmpi", i->BF, i->L, i->RA, i->SI);
    }
}

EMU(CMPI, DForm_5) {
    auto a = get_cmp_ab(i->L, ppu->getGPR(i->RA));
    auto c = a < i->SI.s() ? 4
           : a > i->SI.s() ? 2
           : 1;
    ppu->setCRF_sign(i->BF.u(), c);
}

PRINT(CMP, XForm_16) {
    auto mnemonic = i->L.u() ? "cmpd" : "cmpw";
    if (i->BF.u() == 0) {
        *result = format_nn(mnemonic, i->RA, i->RB);
    } else {
        *result = format_nnn(mnemonic, i->BF, i->RA, i->RB);
    }
}

EMU(CMP, XForm_16) {
    auto a = get_cmp_ab(i->L, ppu->getGPR(i->RA));
    auto b = get_cmp_ab(i->L, ppu->getGPR(i->RB));
    auto c = a < b ? 4
           : a > b ? 2
           : 1;
    ppu->setCRF_sign(i->BF.u(), c);
}

template <int LPos>
inline uint64_t get_cmpl_ab(BitField<LPos, LPos + 1> l, uint64_t value) {
    return l.u() == 0 ? static_cast<uint32_t>(value) : value;
}

PRINT(CMPLI, DForm_6) {
    auto mnemonic = i->L.u() ? "cmpldi" : "cmplwi";
    if (i->BF.u() == 0) {
        *result = format_nn(mnemonic, i->RA, i->UI);
    } else {
        *result = format_nnn(mnemonic, i->BF, i->RA, i->UI);
    }
}

EMU(CMPLI, DForm_6) {
    auto a = get_cmpl_ab(i->L, ppu->getGPR(i->RA));
    auto c = a < i->UI.u() ? 4
           : a > i->UI.u() ? 2
           : 1;
    ppu->setCRF_sign(i->BF.u(), c);
}

PRINT(CMPL, XForm_16) {
    auto mnemonic = i->L.u() ? "cmpld" : "cmplw";
    if (i->BF.u() == 0) {
        *result = format_nn(mnemonic, i->RA, i->RB);
    } else {
        *result = format_nnn(mnemonic, i->BF, i->RA, i->RB);
    }
}

EMU(CMPL, XForm_16) {
    auto a = get_cmpl_ab(i->L, ppu->getGPR(i->RA));
    auto b = get_cmpl_ab(i->L, ppu->getGPR(i->RB));
    auto c = a < b ? 4
           : a > b ? 2
           : 1;
    ppu->setCRF_sign(i->BF.u(), c);
}

enum class DasmMode {
    Print, Emulate  
};

template <DasmMode M, typename P, typename E, typename S>
void invoke_impl(P* phandler, E* ehandler, void* instr, uint64_t cia, S* s) {
    typedef typename boost::function_traits<P>::arg1_type F;
    typedef typename boost::function_traits<P>::arg3_type PS;
    typedef typename boost::function_traits<E>::arg3_type ES;
    if (M == DasmMode::Print)
        phandler(reinterpret_cast<F>(instr), cia, reinterpret_cast<PS>(s));
    if (M == DasmMode::Emulate)
        ehandler(reinterpret_cast<F>(instr), cia, reinterpret_cast<ES>(s));
}

struct PPUDasmInstruction {
    const char* mnemonic;
    std::string operands;
};

#define invoke(name) invoke_impl<M>(print##name, emulate##name, &x, cia, state); break

template <DasmMode M, typename S>
void ppu_dasm(void* instr, uint64_t cia, S* state) {
    uint32_t x = big_to_native<uint32_t>(*reinterpret_cast<uint32_t*>(instr));
    auto iform = reinterpret_cast<IForm*>(&x);
    switch (iform->OPCD.u()) {
        case 1: invoke(NCALL);
        case 10: invoke(CMPLI);
        case 11: invoke(CMPI);
        case 14: invoke(ADDI);
        case 15: invoke(ADDIS);
        case 16: invoke(BC);
        case 17: invoke(SC);
        case 18: invoke(B);
        case 19: {
            auto xlform = reinterpret_cast<XLForm_1*>(&x);
            switch (xlform->XO.u()) {
                case 16: invoke(BCLR);
                case 528: invoke(BCCTR);
                case 257: invoke(CRAND);
                case 449: invoke(CROR);
                case 193: invoke(CRXOR);
                case 225: invoke(CRNAND);
                case 33: invoke(CRNOR);
                case 289: invoke(CREQV);
                case 129: invoke(CRANDC);
                case 417: invoke(CRORC);
                case 0: invoke(MCRF);
                default: throw std::runtime_error("unknown extented opcode");
            }
            break;
        }
        case 20: invoke(RLWIMI);
        case 21: invoke(RLWINM);
        case 23: invoke(RLWNM);
        case 24: invoke(ORI);
        case 25: invoke(ORIS);
        case 26: invoke(XORI);
        case 27: invoke(XORIS);
        case 28: invoke(ANDID);
        case 29: invoke(ANDISD);
        case 34: invoke(LBZ);
        case 30: {
            auto mdform = reinterpret_cast<MDForm_1*>(&x);
            auto mdsform = reinterpret_cast<MDSForm_1*>(&x);
            if (mdsform->XO.u() == 8) {
                invoke(RLDCL);
            } else if (mdsform->XO.u() == 9) {
                invoke(RLDCR);
            } else switch (mdform->XO.u()) {
                case 0: invoke(RLDICL);
                case 1: invoke(RLDICR);
                case 2: invoke(RLDIC);
                case 3: invoke(RLDIMI);
                default: throw std::runtime_error("unknown extented opcode");
            }
            break;
        }
        case 31: {
            auto xform = reinterpret_cast<XForm_1*>(&x);
            switch (xform->XO.u()) {
                case 87: invoke(LBZX);
                case 119: invoke(LBZUX);
                case 279: invoke(LHZX);
                case 311: invoke(LHZUX);
                case 343: invoke(LHAX);
                case 375: invoke(LHAUX);
                case 23: invoke(LWZX);
                case 55: invoke(LWZUX);
                case 341: invoke(LWAX);
                case 373: invoke(LWAUX);
                case 21: invoke(LDX);
                case 53: invoke(LDUX);
                case 215: invoke(STBX);
                case 247: invoke(STBUX);
                case 407: invoke(STHX);
                case 439: invoke(STHUX);
                case 151: invoke(STWX);
                case 183: invoke(STWUX);
                case 149: invoke(STDX);
                case 181: invoke(STDUX);
                case 790: invoke(LHBRX);
                case 534: invoke(LWBRX);
                case 918: invoke(STHBRX);
                case 662: invoke(STWBRX);
                case 266: invoke(ADD);
                case 40: invoke(SUBF);
                case 28: invoke(AND);
                case 444: invoke(OR);
                case 316: invoke(XOR);
                case 476: invoke(NAND);
                case 124: invoke(NOR);
                case 284: invoke(EQV);
                case 60: invoke(ANDC);
                case 412: invoke(ORC);
                case 954: invoke(EXTSB);
                case 922: invoke(EXTSH);
                case 986: invoke(EXTSW);
                case 467: invoke(MTSPR);
                case 339: invoke(MFSPR);
                case 0: invoke(CMP);
                case 32: invoke(CMPL);
                default: throw std::runtime_error("unknown extented opcode");
            }
            break;
        }
        case 35: invoke(LBZU);
        case 32: invoke(LWZ);
        case 33: invoke(LWZU);
        case 36: invoke(STW);
        case 37: invoke(STWU);
        case 38: invoke(STB);
        case 39: invoke(STBU);
        case 40: invoke(LHZ);
        case 41: invoke(LHZU);
        case 42: invoke(LHA);
        case 43: invoke(LHAU);
        case 44: invoke(STH);
        case 45: invoke(STHU);
        case 58: {
            auto dsform = reinterpret_cast<DSForm_1*>(&x);
            switch (dsform->XO.u()) {
                case 2: invoke(LWA);
                case 0: invoke(LD);
                case 1: invoke(LDU);
                default: throw std::runtime_error("unknown extented opcode");
            }
            break;
        }
        case 62: {
            auto dsform = reinterpret_cast<DSForm_2*>(&x);
            switch (dsform->XO.u()) {
                case 0: invoke(STD);
                case 1: invoke(STDU);
                default: throw std::runtime_error("unknown extented opcode");
            }
            break;
        }
        default: throw std::runtime_error("unknown opcode");
    }
}

bool isAbsoluteBranch(void* instr);
bool isTaken(void* branchInstr, uint64_t cia, PPU* ppu);
uint64_t getTargetAddress(void* branchInstr, uint64_t cia);

#undef PRINT
#undef EMU
#undef invoke