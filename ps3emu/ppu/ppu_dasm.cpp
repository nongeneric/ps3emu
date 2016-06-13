#include "ppu_dasm.h"

#include "../Process.h"
#include "../MainMemory.h"
#include "../dasm_utils.h"
#include "ppu_dasm_forms.h"
#include "../utils.h"

#include <boost/type_traits.hpp>
#include <boost/endian/conversion.hpp>
#include <string>
#include <stdint.h>
#include <cfenv>
#include <cmath>
#include <cfloat>
#include <stdio.h>
#include <cinttypes>
#include <limits>
#include <tuple>

using namespace boost::endian;

#define MM thread->mm()
#define TH thread
#define PRINT(name, form) inline void print##name(form* i, uint64_t cia, std::string* result)
#define EMU(name, form) inline void emulate##name(form* i, uint64_t cia, PPUThread* thread)

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
    TH->setNIP(getNIA(i, cia));
    if (i->LK.u())
        TH->setLR(cia + 4);
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
                    PPUThread* thread,
                    BI_t bi)
{
    auto ctr_ok = bo2.u() | ((TH->getCTR() != 0) ^ bo3.u());
    auto cond_ok = bo0.u() | (bit_test(TH->getCR(), 32, bi) == bo1.u());
    return ctr_ok && cond_ok;
}

EMU(BC, BForm) {
    if (!i->BO2.u())
        TH->setCTR(TH->getCTR() - 1);
    if (isTaken(i->BO0, i->BO1, i->BO2, i->BO3, TH, i->BI))
        TH->setNIP(getNIA(i, cia));
    if (i->LK.u())
        TH->setLR(cia + 4);
}

// Branch Conditional to Link Register XL-form, p25

inline int64_t getB(RA_t ra, PPUThread* thread) {
    return ra.u() == 0 ? 0 : TH->getGPR(ra);
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
        TH->setCTR(TH->getCTR() - 1);
    if (isTaken(i->BO0, i->BO1, i->BO2, i->BO3, TH, i->BI))
        TH->setNIP(TH->getLR() & ~3ul);
    if (i->LK.u())
        TH->setLR(cia + 4);
}

// Branch Conditional to Count Register, p25

PRINT(BCCTR, XLForm_2) {
    std::string extMnemonic;
    auto mtype = getExtBranchMnemonic(i->LK.u(), 0, false, true, i->BO, i->BI, extMnemonic);
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
    auto cond_ok = i->BO0.u() || bit_test(TH->getCR(), 32, i->BI) == i->BO1.u();
    if (cond_ok)
        TH->setNIP(TH->getCTR() & ~3);
    if (i->LK.u())
        TH->setLR(cia + 4);
}

// Condition Register AND, p28

PRINT(CRAND, XLForm_1) {
    *result = format_nnn("crand", i->BT, i->BA, i->BB);
}

EMU(CRAND, XLForm_1) {
    auto cr = TH->getCR();
    cr = bit_set(cr, i->BT.u(), bit_test(cr, i->BA) & bit_test(cr, i->BB));
    TH->setCR(cr);
}

// Condition Register OR, p28

PRINT(CROR, XLForm_1) {
    *result = format_nnn("cror", i->BT, i->BA, i->BB);
}

EMU(CROR, XLForm_1) {
    auto cr = TH->getCR();
    cr = bit_set(cr, i->BT.u(), bit_test(cr, i->BA) | bit_test(cr, i->BB));
    TH->setCR(cr);
}

// Condition Register XOR, p28

PRINT(CRXOR, XLForm_1) {
    *result = format_nnn("crxor", i->BT, i->BA, i->BB);
}

EMU(CRXOR, XLForm_1) {
    auto cr = TH->getCR();
    cr = bit_set(cr, i->BT.u(), bit_test(cr, i->BA) ^ bit_test(cr, i->BB));
    TH->setCR(cr);
}

// Condition Register NAND, p28

PRINT(CRNAND, XLForm_1) {
    *result = format_nnn("crnand", i->BT, i->BA, i->BB);
}

EMU(CRNAND, XLForm_1) {
    auto cr = TH->getCR();
    cr = bit_set(cr, i->BT.u(), !(bit_test(cr, i->BA) & bit_test(cr, i->BB)));
    TH->setCR(cr);
}

// Condition Register NOR, p29

PRINT(CRNOR, XLForm_1) {
    *result = format_nnn("crnor", i->BT, i->BA, i->BB);
}

EMU(CRNOR, XLForm_1) {
    auto cr = TH->getCR();
    cr = bit_set(cr, i->BT.u(), (!bit_test(cr, i->BA)) | bit_test(cr, i->BB));
    TH->setCR(cr);
}

// Condition Register Equivalent, p29

PRINT(CREQV, XLForm_1) {
    *result = format_nnn("creqv", i->BT, i->BA, i->BB);
}

EMU(CREQV, XLForm_1) {
    auto cr = TH->getCR();
    cr = bit_set(cr, i->BT.u(), bit_test(cr, i->BA) == bit_test(cr, i->BB));
    TH->setCR(cr);
}

// Condition Register AND with Complement, p29

PRINT(CRANDC, XLForm_1) {
    *result = format_nnn("crandc", i->BT, i->BA, i->BB);
}

EMU(CRANDC, XLForm_1) {
    auto cr = TH->getCR();
    cr = bit_set(cr, i->BT.u(), bit_test(cr, i->BA) & (!bit_test(cr, i->BB)));
    TH->setCR(cr);
}

// Condition Register OR with Complement, p29

PRINT(CRORC, XLForm_1) {
    *result = format_nnn("crorc", i->BT, i->BA, i->BB);
}

EMU(CRORC, XLForm_1) {
    auto cr = TH->getCR();
    cr = bit_set(cr, i->BT.u(), bit_test(cr, i->BA) | (!bit_test(cr, i->BB)));
    TH->setCR(cr);
}

// Condition Register Field Instruction, p30

PRINT(MCRF, XLForm_3) {
    *result = format_nn("mcrf", i->BF, i->BFA);
}

EMU(MCRF, XLForm_3) {
    TH->setCRF(i->BF.u(), TH->getCRF(i->BFA.u()));
}

// Load Byte and Zero, p34

PRINT(LBZ, DForm_1) {
    *result = format_br_nnn("lbz", i->RT, i->D, i->RA);
}

EMU(LBZ, DForm_1) {
    auto b = getB(i->RA, TH);
    auto ea = b + i->D.s();
    TH->setGPR(i->RT, MM->load<1>(ea));
}

// Load Byte and Zero, p34

PRINT(LBZX, XForm_1) {
    *result = format_nnn("lbzx", i->RT, i->RA, i->RB);
}

EMU(LBZX, XForm_1) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->load<1>(ea));
}

// Load Byte and Zero with Update, p34

PRINT(LBZU, DForm_1) {
    *result = format_br_nnn("lbzu", i->RT, i->D, i->RA);
}

EMU(LBZU, DForm_1) {
    auto ea = TH->getGPR(i->RA) + i->D.s();
    TH->setGPR(i->RT, MM->load<1>(ea));
    TH->setGPR(i->RA, ea);
}

// Load Byte and Zero with Update Indexed, p34

PRINT(LBZUX, XForm_1) {
    *result = format_nnn("lbzux", i->RT, i->RA, i->RB);
}

EMU(LBZUX, XForm_1) {
    auto ea = TH->getGPR(i->RA) + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->load<1>(ea));
    TH->setGPR(i->RA, ea);
}

// Load Halfword and Zero, p35

PRINT(LHZ, DForm_1) {
    *result = format_br_nnn("lhz", i->RT, i->D, i->RA);
}

EMU(LHZ, DForm_1) {
    auto b = getB(i->RA, TH);
    auto ea = b + i->D.s();
    TH->setGPR(i->RT, MM->load<2>(ea));
}

// Load Halfword and Zero Indexed, p35

PRINT(LHZX, XForm_1) {
    *result = format_nnn("lhzx", i->RT, i->RA, i->RB);
}

EMU(LHZX, XForm_1) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->load<2>(ea));
}

// Load Halfword and Zero with Update, p35

PRINT(LHZU, DForm_1) {
    *result = format_br_nnn("lhzu", i->RT, i->D, i->RA);
}

EMU(LHZU, DForm_1) {
    auto ea = TH->getGPR(i->RA) + i->D.s();
    TH->setGPR(i->RT, MM->load<2>(ea));
    TH->setGPR(i->RA, ea);
}

// Load Halfword and Zero with Update Indexed, p35

PRINT(LHZUX, XForm_1) {
    *result = format_nnn("lhzux", i->RT, i->RA, i->RB);
}

EMU(LHZUX, XForm_1) {
    auto ea = TH->getGPR(i->RA) + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->load<2>(ea));
    TH->setGPR(i->RA, ea);
}

// Load Halfword Algebraic, p36

PRINT(LHA, DForm_1) {
    *result = format_br_nnn("lha", i->RT, i->D, i->RA);
}

EMU(LHA, DForm_1) {
    auto b = getB(i->RA, TH);
    auto ea = b + i->D.s();
    TH->setGPR(i->RT, MM->loads<2>(ea));
}

// Load Halfword Algebraic Indexed, p36

PRINT(LHAX, XForm_1) {
    *result = format_nnn("LHAX", i->RT, i->RA, i->RB);
}

EMU(LHAX, XForm_1) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->loads<2>(ea));
}

// Load Halfword Algebraic with Update, p36

PRINT(LHAU, DForm_1) {
    *result = format_br_nnn("lhaux", i->RT, i->D, i->RA);
}

EMU(LHAU, DForm_1) {
    auto ea = TH->getGPR(i->RA) + i->D.s();
    TH->setGPR(i->RT, MM->loads<2>(ea));
    TH->setGPR(i->RA, ea);
}

// Load Halfword Algebraic with Update Indexed, p36

PRINT(LHAUX, XForm_1) {
    *result = format_nnn("lhaux", i->RT, i->RA, i->RB);
}

EMU(LHAUX, XForm_1) {
    auto ea = TH->getGPR(i->RA) + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->loads<2>(ea));
    TH->setGPR(i->RA, ea);
}

// Load Word and Zero, p37

PRINT(LWZ, DForm_1) {
    *result = format_br_nnn("lwz", i->RT, i->D, i->RA);
}

EMU(LWZ, DForm_1) {
    auto b = getB(i->RA, TH);
    auto ea = b + i->D.s();
    TH->setGPR(i->RT, MM->load<4>(ea));
}

// Load Word and Zero Indexed, p37

PRINT(LWZX, XForm_1) {
    *result = format_nnn("lwzx", i->RT, i->RA, i->RB);
}

EMU(LWZX, XForm_1) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->load<4>(ea));
}

// Load Word and Zero with Update, p37

PRINT(LWZU, DForm_1) {
    *result = format_br_nnn("lwzu", i->RT, i->D, i->RA);
}

EMU(LWZU, DForm_1) {
    auto ea = TH->getGPR(i->RA) + i->D.s();
    TH->setGPR(i->RT, MM->load<4>(ea));
    TH->setGPR(i->RA, ea);
}

// Load Word and Zero with Update Indexed, p37

PRINT(LWZUX, XForm_1) {
    *result = format_nnn("lwzux", i->RT, i->RA, i->RB);
}

EMU(LWZUX, XForm_1) {
    auto ea = TH->getGPR(i->RA) + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->load<4>(ea));
    TH->setGPR(i->RA, ea);
}

// Load Word Algebraic, p38

PRINT(LWA, DSForm_1) {
    *result = format_br_nnn("lwa", i->RT, i->DS, i->RA);
}

EMU(LWA, DSForm_1) {
    auto b = i->RA.u() == 0 ? 0 : TH->getGPR(i->RA);
    auto ea = b + i->DS.native();
    TH->setGPR(i->RT, MM->loads<4>(ea));
}

// Load Word Algebraic Indexed, p38

PRINT(LWAX, XForm_1) {
    *result = format_nnn("lwax", i->RT, i->RA, i->RB);
}

EMU(LWAX, XForm_1) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->loads<4>(ea));
}

// Load Word Algebraic with Update Indexed, p38

PRINT(LWAUX, XForm_1) {
    *result = format_nnn("lwaux", i->RT, i->RA, i->RB);
}

EMU(LWAUX, XForm_1) {
    auto ea = TH->getGPR(i->RA) + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->loads<4>(ea));
    TH->setGPR(i->RA, ea);
}

// Load Doubleword, p39

PRINT(LD, DSForm_1) {
    *result = format_br_nnn("ld", i->RT, i->DS, i->RA);
}

EMU(LD, DSForm_1) {
    auto b = i->RA.u() == 0 ? 0 : TH->getGPR(i->RA);
    auto ea = b + i->DS.native();
    TH->setGPR(i->RT, MM->load<8>(ea));
}

// Load Doubleword Indexed, p39

PRINT(LDX, XForm_1) {
    *result = format_nnn("ldx", i->RT, i->RA, i->RB);
}

EMU(LDX, XForm_1) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->load<8>(ea));
}

// Load Doubleword with Update, p39

PRINT(LDU, DSForm_1) {
    *result = format_br_nnn("ldu", i->RT, i->DS, i->RA);
}

EMU(LDU, DSForm_1) {
    auto ea = TH->getGPR(i->RA) + i->DS.native();
    TH->setGPR(i->RT, MM->load<8>(ea));
    TH->setGPR(i->RA, ea);
}

// Load Doubleword with Update Indexed, p39

PRINT(LDUX, XForm_1) {
    *result = format_nnn("ldux", i->RT, i->RA, i->RB);
}

EMU(LDUX, XForm_1) {
    auto ea = TH->getGPR(i->RA) + TH->getGPR(i->RB);
    TH->setGPR(i->RT, MM->load<8>(ea));
    TH->setGPR(i->RA, ea);
}

// STORES
//

template <int Bytes>
void EmuStore(DForm_3* i, PPUThread* thread) {
    auto b = getB(i->RA, TH);
    auto ea = b + i->D.s();
    MM->store<Bytes>(ea, TH->getGPR(i->RS));
}

template <int Bytes>
void EmuStoreIndexed(XForm_8* i, PPUThread* thread) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    MM->store<Bytes>(ea, TH->getGPR(i->RS));
}

template <int Bytes>
void EmuStoreUpdate(DForm_3* i, PPUThread* thread) {
    auto ea = TH->getGPR(i->RA) + i->D.s();
    MM->store<Bytes>(ea, TH->getGPR(i->RS));
    TH->setGPR(i->RA, ea);
}

template <int Bytes>
void EmuStoreUpdateIndexed(XForm_8* i, PPUThread* thread) {
    auto ea = TH->getGPR(i->RA) + TH->getGPR(i->RB);
    MM->store<Bytes>(ea, TH->getGPR(i->RS));
    TH->setGPR(i->RA, ea);
}

inline void PrintStore(const char* mnemonic, DForm_3* i, std::string* result) {
    *result = format_br_nnn(mnemonic, i->RS, i->D, i->RA);
}

inline void PrintStoreIndexed(const char* mnemonic, XForm_8* i, std::string* result) {
    *result = format_nnn(mnemonic, i->RS, i->RA, i->RB);
}

PRINT(STB, DForm_3) { PrintStore("stb", i, result); }
EMU(STB, DForm_3) { EmuStore<1>(i, TH); }
PRINT(STBX, XForm_8) { PrintStoreIndexed("stbx", i, result); }
EMU(STBX, XForm_8) { EmuStoreIndexed<1>(i, TH); }
PRINT(STBU, DForm_3) { PrintStore("stbu", i, result); }
EMU(STBU, DForm_3) { EmuStoreUpdate<1>(i, TH); }
PRINT(STBUX, XForm_8) { PrintStoreIndexed("stbux", i, result); }
EMU(STBUX, XForm_8) { EmuStoreUpdateIndexed<1>(i, TH); }

PRINT(STH, DForm_3) { PrintStore("sth", i, result); }
EMU(STH, DForm_3) { EmuStore<2>(i, TH); }
PRINT(STHX, XForm_8) { PrintStoreIndexed("sthx", i, result); }
EMU(STHX, XForm_8) { EmuStoreIndexed<2>(i, TH); }
PRINT(STHU, DForm_3) { PrintStore("sthu", i, result); }
EMU(STHU, DForm_3) { EmuStoreUpdate<2>(i, TH); }
PRINT(STHUX, XForm_8) { PrintStoreIndexed("sthux", i, result); }
EMU(STHUX, XForm_8) { EmuStoreUpdateIndexed<2>(i, TH); }

PRINT(STW, DForm_3) { PrintStore("stw", i, result); }
EMU(STW, DForm_3) { EmuStore<4>(i, TH); }
PRINT(STWX, XForm_8) { PrintStoreIndexed("stwx", i, result); }
EMU(STWX, XForm_8) { EmuStoreIndexed<4>(i, TH); }
PRINT(STWU, DForm_3) { PrintStore("stwu", i, result); }
EMU(STWU, DForm_3) { EmuStoreUpdate<4>(i, TH); }
PRINT(STWUX, XForm_8) { PrintStoreIndexed("stwux", i, result); }
EMU(STWUX, XForm_8) { EmuStoreUpdateIndexed<4>(i, TH); }

PRINT(STD, DSForm_2) { 
    *result = format_br_nnn("std", i->RS, i->DS, i->RA);
}

EMU(STD, DSForm_2) { 
    auto b = getB(i->RA, TH);
    auto ea = b + i->DS.native();
    MM->store<8>(ea, TH->getGPR(i->RS));
}

PRINT(STDX, XForm_8) { PrintStoreIndexed("stdx", i, result); }
EMU(STDX, XForm_8) { EmuStoreIndexed<8>(i, TH); }

PRINT(STDU, DSForm_2) { 
    *result = format_br_nnn("stdu", i->RS, i->DS, i->RA);
}

EMU(STDU, DSForm_2) { 
    auto ea = TH->getGPR(i->RA) + i->DS.native();
    MM->store<8>(ea, TH->getGPR(i->RS));
    TH->setGPR(i->RA, ea);
}

PRINT(STDUX, XForm_8) { PrintStoreIndexed("stdux", i, result); }
EMU(STDUX, XForm_8) { EmuStoreUpdateIndexed<8>(i, TH); }

// Fixed-Point Load and Store with Byte Reversal Instructions, p44

template <int Bytes>
void EmuLoadByteReverseIndexed(XForm_1* i, PPUThread* thread) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    TH->setGPR(i->RT, endian_reverse(MM->load<Bytes>(ea)));
}

PRINT(LHBRX, XForm_1) { *result = format_nnn("lhbrx", i->RT, i->RA, i->RB); }
EMU(LHBRX, XForm_1) { EmuLoadByteReverseIndexed<2>(i, TH); }
PRINT(LWBRX, XForm_1) { *result = format_nnn("lwbrx", i->RT, i->RA, i->RB); }
EMU(LWBRX, XForm_1) { EmuLoadByteReverseIndexed<4>(i, TH); }

template <int Bytes>
void EmuStoreByteReverseIndexed(XForm_8* i, PPUThread* thread) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    MM->store<Bytes>(ea, endian_reverse(TH->getGPR(i->RS)));
}

PRINT(STHBRX, XForm_8) { *result = format_nnn("sthbrx", i->RS, i->RA, i->RB); }
EMU(STHBRX, XForm_8) { EmuStoreByteReverseIndexed<2>(i, TH); }
PRINT(STWBRX, XForm_8) { *result = format_nnn("stwbrx", i->RS, i->RA, i->RB); }
EMU(STWBRX, XForm_8) { EmuStoreByteReverseIndexed<4>(i, TH); }

// Fixed-Point Arithmetic Instructions, p51

PRINT(ADDI, DForm_2) {
    *result = format_nnn("addi", i->RT, i->RA, i->SI);
}

EMU(ADDI, DForm_2) {
    auto b = getB(i->RA, TH);
    TH->setGPR(i->RT, i->SI.s() + b);
}

PRINT(ADDIS, DForm_2) {
    *result = format_nnn("addis", i->RT, i->RA, i->SI);
}

EMU(ADDIS, DForm_2) {
    auto b = getB(i->RA, TH);
    TH->setGPR(i->RT, (int32_t)(i->SI.u() << 16) + b);
}

template <int N, typename T>
inline void update_CRFSign(T result, PPUThread* thread) {
    auto s = result < 0 ? 4
           : result > 0 ? 2
           : 1;
    TH->setCRF_sign(N, s);
}

inline void update_CR0(int64_t result, PPUThread* thread) {
    update_CRFSign<0>(result, TH);
}

template <int P1, int P2>
inline void update_CR0_OV(BitField<P1, P1 + 1> oe, 
                          BitField<P2, P2 + 1> rc, 
                          bool ov, int64_t result, PPUThread* thread) {
    if (oe.u() && ov) {
        TH->setOV();
    }
    if (rc.u()) {
        update_CR0(result, TH);
    }
}

PRINT(ADD, XOForm_1) {
    const char* mnemonics[][2] = {
        { "add", "add." }, { "addo", "addo." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(ADD, XOForm_1) {
    auto ra = TH->getGPR(i->RA);
    auto rb = TH->getGPR(i->RB);
    unsigned __int128 res = (__int128)ra + rb;
    bool ov = res > 0xffffffffffffffff;
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, TH);
}

PRINT(ADDZE, XOForm_3) {
    const char* mnemonics[][2] = {
        { "addze", "addze." }, { "addzeo", "addzeo." }
    };
    *result = format_nn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA);
}

EMU(ADDZE, XOForm_3) {
    auto ra = TH->getGPR(i->RA);
    unsigned __int128 res = (__int128)ra + TH->getCA();
    bool ov = res > 0xffffffffffffffff;
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, TH);
}

PRINT(SUBF, XOForm_1) {
    const char* mnemonics[][2] = {
        { "subf", "subf." }, { "subfo", "subfo." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(SUBF, XOForm_1) {
    auto ra = TH->getGPR(i->RA);
    auto rb = TH->getGPR(i->RB);
    uint64_t res;
    bool ov = 0;
    if (i->OE.u())
        ov = __builtin_usubl_overflow(rb, ra, &res);
    else
        res = rb - ra;
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, TH);
}

PRINT(ADDIC, DForm_2) {
    *result = format_nnn("addic", i->RT, i->RA, i->SI);
}

EMU(ADDIC, DForm_2) {
    auto ra = TH->getGPR(i->RA);
    auto si = i->SI.s();
    uint64_t res;
    auto ov = __builtin_uaddl_overflow(ra, si, &res);
    TH->setGPR(i->RT, res);
    TH->setCA(ov);
}

PRINT(ADDICD, DForm_2) {
    *result = format_nnn("addic.", i->RT, i->RA, i->SI);
}

EMU(ADDICD, DForm_2) {
    auto ra = TH->getGPR(i->RA);
    auto si = i->SI.s();
    uint64_t res;
    auto ov = __builtin_uaddl_overflow(ra, si, &res);
    TH->setGPR(i->RT, res);
    TH->setCA(ov);
    update_CR0(res, TH);
}

PRINT(SUBFIC, DForm_2) {
    *result = format_nnn("subfic", i->RT, i->RA, i->SI);
}

EMU(SUBFIC, DForm_2) {
    auto ra = TH->getGPR(i->RA);
    auto si = i->SI.s();
    int64_t res;
    auto ov = __builtin_ssubl_overflow(si, ra, &res);
    TH->setGPR(i->RT, res);
    TH->setCA(ov);
}

PRINT(ADDC, XOForm_1) {
    const char* mnemonics[][2] = {
        { "addc", "addc." }, { "addco", "addco." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(ADDC, XOForm_1) {
    auto ra = TH->getGPR(i->RA);
    auto rb = TH->getGPR(i->RB);
    unsigned __int128 res = ra;
    res += rb;
    auto ca = res >> 64;
    TH->setCA(ca);
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ca, res, TH);
}

PRINT(SUBFC, XOForm_1) {
    const char* mnemonics[][2] = {
        { "subfc", "subfc." }, { "subfco", "subfco." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(SUBFC, XOForm_1) {
    auto ra = TH->getGPR(i->RA);
    auto rb = TH->getGPR(i->RB);
    unsigned __int128 res = ~ra;
    res += rb;
    res += 1;
    auto ca = res >> 64;
    TH->setCA(ca);
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ca, res, TH);
}

// Fixed-Point Logical Instructions, p65

// AND

PRINT(ANDID, DForm_4) {
    *result = format_nnn("andi.", i->RA, i->RS, i->UI);
}

EMU(ANDID, DForm_4) {
    auto res = TH->getGPR(i->RS) & i->UI.u();
    update_CR0(res, TH);
    TH->setGPR(i->RA, res);
}

PRINT(ANDISD, DForm_4) {
    *result = format_nnn("andis.", i->RA, i->RS, i->UI);
}

EMU(ANDISD, DForm_4) {
    auto res = TH->getGPR(i->RS) & (i->UI.u() << 16);
    update_CR0(res, TH);
    TH->setGPR(i->RA, res);
}

// OR

PRINT(ORI, DForm_4) {
    if (i->RS.u() == 0 && i->RA.u() == 0 && i->UI.u() == 0) {
        *result = "nop";
    } else {
        *result = format_nnn("ori", i->RA, i->RS, i->UI);
    }
}

EMU(ORI, DForm_4) {
    auto res = TH->getGPR(i->RS) | i->UI.u();
    TH->setGPR(i->RA, res);
}

PRINT(ORIS, DForm_4) {
    *result = format_nnn("oris", i->RA, i->RS, i->UI);
}

EMU(ORIS, DForm_4) {
    auto res = TH->getGPR(i->RS) | (i->UI.u() << 16);
    TH->setGPR(i->RA, res);
}

// XOR

PRINT(XORI, DForm_4) {
    *result = format_nnn("xori", i->RA, i->RS, i->UI);
}

EMU(XORI, DForm_4) {
    auto res = TH->getGPR(i->RS) ^ i->UI.u();
    TH->setGPR(i->RA, res);
}

PRINT(XORIS, DForm_4) {
    *result = format_nnn("xoris", i->RA, i->RS, i->UI);
}

EMU(XORIS, DForm_4) {
    auto res = TH->getGPR(i->RS) ^ (i->UI.u() << 16);
    TH->setGPR(i->RA, res);
}

// X-forms

PRINT(AND, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "and." : "and", i->RA, i->RS, i->RB);
}

EMU(AND, XForm_6) {
    auto res = TH->getGPR(i->RS) & TH->getGPR(i->RB);
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(OR, XForm_6) {
    if (i->RS.u() == i->RB.u()) {
        *result = format_nn(i->Rc.u() ? "mr." : "mr", i->RA, i->RS);
    } else {
        *result = format_nnn(i->Rc.u() ? "or." : "or", i->RA, i->RS, i->RB);
    }
}

EMU(OR, XForm_6) {
    auto res = TH->getGPR(i->RS) | TH->getGPR(i->RB);
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(XOR, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "xor." : "xor", i->RA, i->RS, i->RB);
}

EMU(XOR, XForm_6) {
    auto res = TH->getGPR(i->RS) ^ TH->getGPR(i->RB);
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(NAND, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "nand." : "nand", i->RA, i->RS, i->RB);
}

EMU(NAND, XForm_6) {
    auto res = ~(TH->getGPR(i->RS) & TH->getGPR(i->RB));
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(NOR, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "nor." : "nor", i->RA, i->RS, i->RB);
}

EMU(NOR, XForm_6) {
    auto res = ~(TH->getGPR(i->RS) | TH->getGPR(i->RB));
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(EQV, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "eqv." : "eqv", i->RA, i->RS, i->RB);
}

EMU(EQV, XForm_6) {
    auto res = ~(TH->getGPR(i->RS) ^ TH->getGPR(i->RB));
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(ANDC, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "andc." : "andc", i->RA, i->RS, i->RB);
}

EMU(ANDC, XForm_6) {
    auto res = TH->getGPR(i->RS) & ~TH->getGPR(i->RB);
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(ORC, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "orc." : "orc", i->RA, i->RS, i->RB);
}

EMU(ORC, XForm_6) {
    auto res = TH->getGPR(i->RS) | ~TH->getGPR(i->RB);
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

// Extend Sign

PRINT(EXTSB, XForm_11) {
    *result = format_nn(i->Rc.u() ? "extsb." : "extsb", i->RA, i->RS);
}

EMU(EXTSB, XForm_11) {
    TH->setGPR(i->RA, (int64_t)static_cast<int8_t>(TH->getGPR(i->RS)));
}

PRINT(EXTSH, XForm_11) {
    *result = format_nn(i->Rc.u() ? "extsh." : "extsh", i->RA, i->RS);
}

EMU(EXTSH, XForm_11) {
    TH->setGPR(i->RA, (int64_t)static_cast<int16_t>(TH->getGPR(i->RS)));
}

PRINT(EXTSW, XForm_11) {
    *result = format_nn(i->Rc.u() ? "extsw." : "extsw", i->RA, i->RS);
}

EMU(EXTSW, XForm_11) {
    TH->setGPR(i->RA, (int64_t)static_cast<int32_t>(TH->getGPR(i->RS)));
}

// Move To/From System Register Instructions, p81

inline bool isXER(BitField<11, 21> spr) {
    return spr.u() == 1u << 5;
}

inline bool isLR(BitField<11, 21> spr) {
    return spr.u() == 8u << 5;
}

inline bool isCTR(BitField<11, 21> spr) {
    return spr.u() == 9u << 5;
}

PRINT(MTSPR, XFXForm_7) {
    auto mnemonic = isXER(i->spr) ? "mtxer"
                  : isLR(i->spr) ? "mtlr"
                  : isCTR(i->spr) ? "mtctr"
                  : nullptr;
    if (mnemonic) {
        *result = format_n(mnemonic, i->RS);
    } else {
        throw IllegalInstructionException();
    }
}

EMU(MTSPR, XFXForm_7) {
    auto rs = TH->getGPR(i->RS);
    if (isXER(i->spr)) {
        TH->setXER(rs);
    } else if (isLR(i->spr)) {
        TH->setLR(rs);
    } else if (isCTR(i->spr)) {
        TH->setCTR(rs);
    } else {
        throw IllegalInstructionException();
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
        throw IllegalInstructionException();
    }
}

EMU(MFSPR, XFXForm_7) {
    auto v = isXER(i->spr) ? TH->getXER()
           : isLR(i->spr) ? TH->getLR()
           : isCTR(i->spr) ? TH->getCTR()
           : (throw std::runtime_error("illegal"), 0);
     TH->setGPR(i->RS, v);
}

template <int Pos04, int Pos5>
inline uint8_t getNBE(BitField<Pos04, Pos04 + 5> _04, BitField<Pos5, Pos5 + 1> _05) {
    return (_05.u() << 5) | _04.u();
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
    auto r = rol<64>(TH->getGPR(i->RS), n);
    auto m = mask<64>(b, 63);
    auto res = r & m;
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
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
    auto r = rol<64>(TH->getGPR(i->RS), n);
    auto m = mask<64>(0, e);
    auto res = r & m;
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
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
    auto r = rol<64>(TH->getGPR(i->RS), n);
    auto m = mask<64>(b, ~n & 63);
    auto res = (r & m) | (TH->getGPR(i->RA) & (~m));
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(RLDCL, MDSForm_1) {
    if (i->mb.u() == 0) {
        *result = format_nnn(i->Rc.u() ? "rotld." : "rotld", i->RA, i->RS, i->RB);
    } else {
        *result = format_nnnn(i->Rc.u() ? "rotld." : "rotld", i->RA, i->RS, i->RB, i->mb);
    }
}
    
EMU(RLDCL, MDSForm_1) {
    auto n = TH->getGPR(i->RB) & 127;
    auto r = rol<64>(TH->getGPR(i->RS), n);
    auto b = i->mb.u();
    auto m = mask<64>(b, 63);
    auto res = r & m;
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(RLDCR, MDSForm_2) {
    *result = format_nnnn(i->Rc.u() ? "rldcr." : "rldcr", i->RA, i->RS, i->RB, i->me);
}

EMU(RLDCR, MDSForm_2) {
    auto n = TH->getGPR(i->RB) & 127;
    auto r = rol<64>(TH->getGPR(i->RS), n);
    auto e = i->me.u();
    auto m = mask<64>(0, e);
    auto res = r & m;
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
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
    auto r = rol<64>(TH->getGPR(i->RS), n);
    auto m = mask<64>(b, ~n & 63);
    auto res = r & m;
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
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
    auto r = rol<32>(TH->getGPR(i->RS), n);
    auto m = mask<64>(i->mb.u() + 32, i->me.u() + 32);
    auto res = r & m;
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(RLWNM, MForm_1) {
    if (i->mb.u() == 0 && i->me.u() == 31) {
        *result = format_nnn(i->Rc.u() ? "rotlw." : "rotlw", i->RA, i->RS, i->RB);
    } else {
        *result = format_nnnnn(i->Rc.u() ? "rlwnm." : "rlwnm", i->RA, i->RS, i->RB, i->mb, i->me);
    }
}

EMU(RLWNM, MForm_1) {
    auto n = TH->getGPR(i->RB) & 31;
    auto r = rol<32>(TH->getGPR(i->RS), n);
    auto m = mask<64>(i->mb.u() + 32, i->me.u() + 32);
    auto res = r & m;
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
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
    auto r = rol<32>(TH->getGPR(i->RS), n);
    auto m = mask<64>(i->mb.u() + 32, i->me.u() + 32);
    auto res = (r & m) | (TH->getGPR(i->RA) & ~m);
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(SC, SCForm) {
    *result = "sc";
}

EMU(SC, SCForm) {
    TH->scall();
}

PRINT(NCALL, NCallForm) {
    *result = format_u("ncall", i->idx.u());
}

EMU(NCALL, NCallForm) {
    TH->ncall(i->idx.u());
}

template <int LPos>
inline int64_t get_cmp_ab(BitField<LPos, LPos + 1> l, uint64_t value) {
    return l.u() == 0 ? (int64_t)static_cast<int32_t>(value) : value;
}

PRINT(CMPI, DForm_5) {
    if (i->L.u() == 1) {
        if (i->BF.u() == 0) {
            *result = format_nn("cmpdi", i->RA, i->SI);
        } else {
            *result = format_nnn("cmpdi", i->BF, i->RA, i->SI);
        }
    } else if (i->L.u() == 0) {
        *result = format_nnn("cmpwi", i->BF, i->RA, i->SI);
    } else {
        *result = format_nnnn("cmpi", i->BF, i->L, i->RA, i->SI);
    }
}

EMU(CMPI, DForm_5) {
    auto a = get_cmp_ab(i->L, TH->getGPR(i->RA));
    auto c = a < i->SI.s() ? 4
           : a > i->SI.s() ? 2
           : 1;
    TH->setCRF_sign(i->BF.u(), c);
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
    auto a = get_cmp_ab(i->L, TH->getGPR(i->RA));
    auto b = get_cmp_ab(i->L, TH->getGPR(i->RB));
    auto c = a < b ? 4
           : a > b ? 2
           : 1;
    TH->setCRF_sign(i->BF.u(), c);
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
    auto a = get_cmpl_ab(i->L, TH->getGPR(i->RA));
    auto c = a < i->UI.u() ? 4
           : a > i->UI.u() ? 2
           : 1;
    TH->setCRF_sign(i->BF.u(), c);
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
    auto a = get_cmpl_ab(i->L, TH->getGPR(i->RA));
    auto b = get_cmpl_ab(i->L, TH->getGPR(i->RB));
    auto c = a < b ? 4
           : a > b ? 2
           : 1;
    TH->setCRF_sign(i->BF.u(), c);
}

PRINT(MFCR, XFXForm_3) {
    *result = format_n("mfcr", i->RT);
}

EMU(MFCR, XFXForm_3) {
    TH->setGPR(i->RT, TH->getCR());
}

PRINT(SLD, XForm_6) {
   *result = format_nnn(i->Rc.u() ? "sld." : "sld", i->RA, i->RS, i->RB); 
}

EMU(SLD, XForm_6) {
    auto b = TH->getGPR(i->RB) & 127;
    auto res = b > 63 ? 0 : TH->getGPR(i->RS) << b;
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(SLW, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "slw." : "slw", i->RA, i->RS, i->RB); 
}

EMU(SLW, XForm_6) {
    auto b = TH->getGPR(i->RB) & 63;
    auto res = b > 31 ? 0 : (uint32_t)TH->getGPR(i->RS) << b;
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(SRD, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "srd." : "srd", i->RA, i->RS, i->RB); 
}

EMU(SRD, XForm_6) {
    auto b = TH->getGPR(i->RB) & 127;
    auto res = b > 63 ? 0 : TH->getGPR(i->RS) >> b;
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(SRW, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "srw." : "srw", i->RA, i->RS, i->RB); 
}

EMU(SRW, XForm_6) {
    auto b = TH->getGPR(i->RB) & 63;
    auto res = b > 31 ? 0 : (uint32_t)TH->getGPR(i->RS) >> b;
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(SRADI, XSForm) {
    auto n =  getNBE(i->sh04, i->sh5);
    *result = format_nnu(i->Rc.u() ? "sradi." : "sradi", i->RA, i->RS, n);
}

EMU(SRADI, XSForm) {
    auto n =  getNBE(i->sh04, i->sh5);
    auto rs = TH->getGPR(i->RS);
    auto r = rol<64>(rs, 64 - n);
    auto m = mask<64>(n, 63);
    auto s = bit_test(rs, 64, 0);
    auto ra = (r & m) | ((s ? -1ull : 0ull) & ~m);
    auto ca = s & ((r & ~m) != 0);
    TH->setGPR(i->RA, ra);
    TH->setCA(ca);
    if (i->Rc.u())
        update_CR0(ra, TH);
}

PRINT(SRAWI, XForm_10) {
    *result = format_nnn(i->Rc.u() ? "srawi." : "srawi", i->RA, i->RS, i->SH);
}

EMU(SRAWI, XForm_10) {
    auto n =  i->SH.u();
    auto rs = TH->getGPR(i->RS) & 0xffffffff;
    auto r = rol<32>(rs, 64 - n);
    auto m = mask<64>(n + 32, 63);
    auto s = bit_test(rs, 64, 32);
    auto ra = (r & m) | ((s ? -1ull : 0ull) & ~m);
    auto ca = s & ((r & ~m) != 0);
    TH->setGPR(i->RA, ra);
    TH->setCA(ca);
    if (i->Rc.u())
        update_CR0(ra, TH);
}

PRINT(SRAW, XForm_6) {
    *result = format_nnn(i->Rc.u() ? "sraw." : "sraw", i->RA, i->RS, i->RB);
}

EMU(SRAW, XForm_6) {
    auto rb = TH->getGPR(i->RB);
    auto n = rb & 0b11111;
    auto rs = TH->getGPR(i->RS) & 0xffffffff;
    auto r = rol<32>(rs, 64 - n);
    uint64_t m = (rb & 0b100000) == 0 ? mask<64>(n + 32, 63) : 0;
    auto s = bit_test(rs, 64, 32);
    auto ra = (r & m) | ((s ? -1ull : 0ull) & ~m);
    auto ca = s & (((r & ~m) & 0xffffffff) != 0);
    TH->setGPR(i->RA, ra);
    TH->setCA(ca);
    if (i->Rc.u())
        update_CR0(ra, TH);
}

PRINT(NEG, XOForm_3) {
    const char* mnemonics[][2] = {
        { "neg", "neg." }, { "nego", "nego." }
    };
    *result = format_nn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA);
}

EMU(NEG, XOForm_3) {
    auto ra = TH->getGPR(i->RA);
    auto ov = ra == (1ull << 63);
    auto res = ov ? ra : (~ra + 1);
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, TH);    
}

PRINT(DIVD, XOForm_1) {
    const char* mnemonics[][2] = {
        { "divd", "divd." }, { "divdo", "divdo." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(DIVD, XOForm_1) {
    int64_t dividend = TH->getGPR(i->RA);
    int64_t divisor = TH->getGPR(i->RB);
    auto ov = divisor == 0 || ((uint64_t)dividend == 0x8000000000000000 && divisor == -1ll);
    int64_t res = ov ? 0 : dividend / divisor;
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, TH);
}

PRINT(DIVW, XOForm_1) {
    const char* mnemonics[][2] = {
        { "divw", "divw." }, { "divwo", "divwo." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(DIVW, XOForm_1) {
    int64_t dividend = (int32_t)(TH->getGPR(i->RA) & 0xffffffff);
    int64_t divisor = (int32_t)(TH->getGPR(i->RB) & 0xffffffff);
    auto ov = divisor == 0 || ((uint64_t)dividend == 0x80000000 && divisor == -1l);
    int64_t res = ov ? 0 : dividend / divisor;
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, TH);
}

PRINT(DIVDU, XOForm_1) {
    const char* mnemonics[][2] = {
        { "divdu", "divdu." }, { "divduo", "divduo." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(DIVDU, XOForm_1) {
    auto dividend = TH->getGPR(i->RA);
    auto divisor = TH->getGPR(i->RB);
    auto ov = divisor == 0;
    auto res = ov ? 0 : dividend / divisor;
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, TH);
}

PRINT(DIVWU, XOForm_1) {
    const char* mnemonics[][2] = {
        { "divwu", "divwu." }, { "divwuo", "divwuo." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(DIVWU, XOForm_1) {
    auto dividend = static_cast<uint32_t>(TH->getGPR(i->RA));
    auto divisor = static_cast<uint32_t>(TH->getGPR(i->RB));
    auto ov = divisor == 0;
    auto res = ov ? 0 : dividend / divisor;
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, TH);
}

PRINT(MULLD, XOForm_1) {
    const char* mnemonics[][2] = {
        { "mulld", "mulld." }, { "mulldo", "mulldo." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(MULLD, XOForm_1) {
    auto a = TH->getGPR(i->RA);
    auto b = TH->getGPR(i->RB);
    unsigned __int128 res = a * b;
    auto ov = res > 0xffffffffffffffff;
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, TH);
}


PRINT(MULLW, XOForm_1) {
    const char* mnemonics[][2] = {
        { "mullw", "mullw." }, { "mullwo", "mullwo." }
    };
    *result = format_nnn(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
}

EMU(MULLW, XOForm_1) {
    auto res = (TH->getGPR(i->RA) & 0xffffffff)
             * (TH->getGPR(i->RB) & 0xffffffff);
    auto ov = res > 0xffffffff;
    TH->setGPR(i->RT, res);
    update_CR0_OV(i->OE, i->Rc, ov, res, TH);
}

PRINT(MULHD, XOForm_1) {
    *result = format_nnn(i->Rc.u() ? "mulhd." : "mulhd", i->RT, i->RA, i->RB);
}

EMU(MULHD, XOForm_1) {
    unsigned __int128 prod = (__int128)TH->getGPR(i->RA) * TH->getGPR(i->RB);
    auto res = prod >> 64;
    TH->setGPR(i->RT, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(MULHW, XOForm_1) {
    *result = format_nnn(i->Rc.u() ? "mulhw." : "mulhw", i->RT, i->RA, i->RB);
}

EMU(MULHW, XOForm_1) {
    int64_t prod = (int64_t)(int32_t)TH->getGPR(i->RA)
                 * (int64_t)(int32_t)TH->getGPR(i->RB);
    auto res = (int64_t)((uint64_t)prod >> 32);
    TH->setGPR(i->RT, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(MULHWU, XOForm_1) {
    *result = format_nnn(i->Rc.u() ? "mulhwu." : "mulhwu", i->RT, i->RA, i->RB);
}

EMU(MULHWU, XOForm_1) {
    uint64_t prod = (uint64_t)(uint32_t)TH->getGPR(i->RA)
                  * (uint64_t)(uint32_t)TH->getGPR(i->RB);
    auto res = (uint64_t)((uint64_t)prod >> 32);
    TH->setGPR(i->RT, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(MULHDU, XOForm_1) {
    *result = format_nnn(i->Rc.u() ? "mulhdu." : "mulhdu", i->RT, i->RA, i->RB);
}

EMU(MULHDU, XOForm_1) {
    unsigned __int128 prod = (unsigned __int128)TH->getGPR(i->RA) * 
                             (unsigned __int128)TH->getGPR(i->RB);
    auto res = prod >> 64;
    TH->setGPR(i->RT, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(MULLI, DForm_2) {
    *result = format_nnn("mulli", i->RT, i->RA, i->SI);
}

EMU(MULLI, DForm_2) {
    __int128 a = (int64_t)TH->getGPR(i->RA);
    auto prod = a * i->SI.s();
    TH->setGPR(i->RT, prod);
}

PRINT(MTOCRF, XFXForm_6) {
    *result = format_nn("mtocrf", i->FXM, i->RS);
}

EMU(MTOCRF, XFXForm_6) {
    auto fxm = i->FXM.u();
    auto n = fxm & 128 ? 0
           : fxm & 64 ? 1
           : fxm & 32 ? 2
           : fxm & 16 ? 3
           : fxm & 8 ? 4
           : fxm & 4 ? 5
           : fxm & 2 ? 6
           : 7;
    auto cr = TH->getCR() & ~mask<32>(4*n, 4*n + 3);
    auto rs = TH->getGPR(i->RS) & mask<32>(4*n, 4*n + 3);
    TH->setCR(cr | rs);
}

PRINT(DCBT, XForm_31) {
    *result = format_nn("dcbt", i->RA, i->RB);
}

EMU(DCBT, XForm_31) { }

PRINT(CNTLZD, XForm_11) {
    *result = format_nn(i->Rc.u() ? "cntlzd." : "cntlzd", i->RA, i->RS);
}

EMU(CNTLZD, XForm_11) {
    static_assert(sizeof(unsigned long long int) == 8, "");
    auto rs = TH->getGPR(i->RS);
    auto res = !rs ? 64 : __builtin_clzll(rs);
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(CNTLZW, XForm_11) {
    *result = format_nn(i->Rc.u() ? "cntlzw." : "cntlzw", i->RA, i->RS);
}

EMU(CNTLZW, XForm_11) {
    static_assert(sizeof(unsigned int) == 4, "");
    auto rs = TH->getGPR(i->RS);
    auto res = !rs ? 32 : __builtin_clz(rs);
    TH->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, TH);
}

PRINT(LFS, DForm_8) {
    *result = format_br_nnn("lfs", i->FRT, i->D, i->RA);
}

EMU(LFS, DForm_8) {
    auto b = getB(i->RA, TH);
    auto ea = b + i->D.s();
    TH->setFPRd(i->FRT, MM->loadf(ea));
}

PRINT(LFSX, XForm_26) {
    *result = format_nnn("lfsx", i->FRT, i->RA, i->RB);
}

EMU(LFSX, XForm_26) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    TH->setFPRd(i->FRT, MM->loadf(ea));
}

PRINT(LFSU, DForm_8) {
    *result = format_br_nnn("lfsu", i->FRT, i->D, i->RA);
}

EMU(LFSU, DForm_8) {
    auto ea = TH->getGPR(i->RA) + i->D.s();
    TH->setFPRd(i->FRT, MM->loadf(ea));
    TH->setGPR(i->RA, ea);
}

PRINT(LFSUX, XForm_26) {
    *result = format_nnn("lfsux", i->FRT, i->RA, i->RB);
}

EMU(LFSUX, XForm_26) {
    auto ea = TH->getGPR(i->RA) + TH->getGPR(i->RB);
    TH->setFPRd(i->FRT, MM->loadf(ea));
    TH->setGPR(i->RA, ea);
}

PRINT(LFD, DForm_8) {
    *result = format_br_nnn("lfd", i->FRT, i->D, i->RA);
}

EMU(LFD, DForm_8) {
    auto b = getB(i->RA, TH);
    auto ea = b + i->D.s();
    TH->setFPRd(i->FRT, MM->loadd(ea));
}

PRINT(LFDX, XForm_26) {
    *result = format_nnn("lfdx", i->FRT, i->RA, i->RB);
}

EMU(LFDX, XForm_26) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    TH->setFPRd(i->FRT, MM->loadd(ea));
}

PRINT(LFDU, DForm_8) {
    *result = format_br_nnn("lfdu", i->FRT, i->D, i->RA);
}

EMU(LFDU, DForm_8) {
    auto ea = TH->getGPR(i->RA) + i->D.s();
    TH->setFPRd(i->FRT, MM->loadd(ea));
    TH->setGPR(i->RA, ea);
}

PRINT(LFDUX, XForm_26) {
    *result = format_nnn("lfdux", i->FRT, i->RA, i->RB);
}

EMU(LFDUX, XForm_26) {
    auto ea = TH->getGPR(i->RA) + TH->getGPR(i->RB);
    TH->setFPRd(i->FRT, MM->loadd(ea));
    TH->setGPR(i->RA, ea);
}

PRINT(STFS, DForm_9) {
    *result = format_br_nnn("stfs", i->FRS, i->D, i->RA);
}

EMU(STFS, DForm_9) {
    auto b = getB(i->RA, TH);
    auto ea = b + i->D.s();
    auto frs = TH->getFPRd(i->FRS);
    MM->storef(ea, frs);
}

PRINT(STFSX, XForm_29) {
    *result = format_br_nnn("stfsx", i->FRS, i->RA, i->RB);
}

EMU(STFSX, XForm_29) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    auto frs = TH->getFPRd(i->FRS);
    MM->storef(ea, frs);
}

PRINT(STFSU, DForm_9) {
    *result = format_br_nnn("stfsu", i->FRS, i->D, i->RA);
}

EMU(STFSU, DForm_9) {
    auto ea = TH->getGPR(i->RA) + i->D.s();
    auto frs = TH->getFPRd(i->FRS);
    MM->storef(ea, frs);
}

PRINT(STFSUX, XForm_29) {
    *result = format_br_nnn("sftsux", i->FRS, i->RA, i->RB);
}

EMU(STFSUX, XForm_29) {
    auto ea = TH->getGPR(i->RA) + TH->getGPR(i->RB);
    auto frs = TH->getFPRd(i->FRS);
    MM->storef(ea, frs);
}

PRINT(STFD, DForm_9) {
    *result = format_br_nnn("stfd", i->FRS, i->D, i->RA);
}

EMU(STFD, DForm_9) {
    auto b = getB(i->RA, TH);
    auto ea = b + i->D.s();
    auto frs = TH->getFPRd(i->FRS);
    MM->stored(ea, frs);
}

PRINT(STFDX, XForm_29) {
    *result = format_br_nnn("stfdx", i->FRS, i->RA, i->RB);
}

EMU(STFDX, XForm_29) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    auto frs = TH->getFPRd(i->FRS);
    MM->stored(ea, frs);
}

PRINT(STFDU, DForm_9) {
    *result = format_br_nnn("stfdu", i->FRS, i->D, i->RA);
}

EMU(STFDU, DForm_9) {
    auto ea = TH->getGPR(i->RA) + i->D.s();
    auto frs = TH->getFPRd(i->FRS);
    MM->stored(ea, frs);
}

PRINT(STFDUX, XForm_29) {
    *result = format_br_nnn("sftdux", i->FRS, i->RA, i->RB);
}

EMU(STFDUX, XForm_29) {
    auto ea = TH->getGPR(i->RA) + TH->getGPR(i->RB);
    auto frs = TH->getFPRd(i->FRS);
    MM->stored(ea, frs);
}

PRINT(STFIWX, XForm_29) {
    *result = format_br_nnn("stfiwx", i->FRS, i->RA, i->RB);
}

EMU(STFIWX, XForm_29) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    auto frs = (uint32_t)TH->getFPR(i->FRS);
    MM->store<4>(ea, frs);
}

PRINT(FMR, XForm_27) {
    *result = format_nn(i->Rc.u() ? "fmr." : "fmr", i->FRT, i->FRB);
}

EMU(FMR, XForm_27) {
    auto res = TH->getFPRd(i->FRB);
    TH->setFPRd(i->FRT, res);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

PRINT(FNEG, XForm_27) {
    *result = format_nn(i->Rc.u() ? "fneg." : "fneg", i->FRT, i->FRB);
}

EMU(FNEG, XForm_27) {
    auto res = -TH->getFPRd(i->FRB);
    TH->setFPRd(i->FRT, res);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

PRINT(FABS, XForm_27) {
    *result = format_nn(i->Rc.u() ? "fabs." : "fabs", i->FRT, i->FRB);
}

EMU(FABS, XForm_27) {
    auto res = std::abs(TH->getFPRd(i->FRB));
    TH->setFPRd(i->FRT, res);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

PRINT(FNABS, XForm_27) {
    *result = format_nn(i->Rc.u() ? "fnabs." : "fnabs", i->FRT, i->FRB);
}

EMU(FNABS, XForm_27) {
    auto res = -std::abs(TH->getFPRd(i->FRB));
    TH->setFPRd(i->FRT, res);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

PRINT(FSQRT, AForm_4) {
    *result = format_nn(i->Rc.u() ? "fsqrt." : "fsqrt", i->FRT, i->FRB);
}

EMU(FSQRT, AForm_4) {
    auto rb = TH->getFPRd(i->FRB);
    auto res = sqrt(rb);
    TH->setFPRd(i->FRT, res);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

PRINT(FSQRTS, AForm_4) {
    *result = format_nn(i->Rc.u() ? "fsqrts." : "fsqrts", i->FRT, i->FRB);
}

EMU(FSQRTS, AForm_4) {
    float rb = TH->getFPRd(i->FRB);
    auto res = sqrt(rb);
    TH->setFPRd(i->FRT, res);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

PRINT(FRE, AForm_4) {
    *result = format_nn(i->Rc.u() ? "fre." : "fre", i->FRT, i->FRB);
}

EMU(FRE, AForm_4) {
    auto rb = TH->getFPRd(i->FRB);
    auto res = 1. / rb;
    TH->setFPRd(i->FRT, res);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

PRINT(FRES, AForm_4) {
    *result = format_nn(i->Rc.u() ? "fres." : "fres", i->FRT, i->FRB);
}

EMU(FRES, AForm_4) {
    float rb = TH->getFPRd(i->FRB);
    auto res = 1.f / rb;
    TH->setFPRd(i->FRT, res);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

PRINT(FRSQRTE, AForm_4) {
    *result = format_nn(i->Rc.u() ? "frsqrte." : "frsqrte", i->FRT, i->FRB);
}

EMU(FRSQRTE, AForm_4) {
    auto rb = TH->getFPRd(i->FRB);
    auto res = 1. / sqrt(rb);
    TH->setFPRd(i->FRT, res);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

PRINT(FRSQRTES, AForm_4) {
    *result = format_nn(i->Rc.u() ? "frsqrtes." : "frsqrtes", i->FRT, i->FRB);
}

EMU(FRSQRTES, AForm_4) {
    float rb = TH->getFPRd(i->FRB);
    auto res = 1.f / sqrt(rb);
    TH->setFPRd(i->FRT, res);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

namespace FPRF {
    enum t {
        QNAN = 17,
        NINF = 9,
        NNORM = 8,
        NDENORM = 24,
        _NZERO = 18,
        PZERO = 2,
        PDENORM = 20,
        PNORM = 4,
        PINF = 5
    };
}

inline uint32_t getFPRF(double r) {
    auto c = std::fpclassify(r);
    uint32_t fprf = 0;
    if (c == FP_INFINITE) {
        fprf = r > 0 ? FPRF::PINF : FPRF::NINF;
    } else if (c == FP_ZERO) {
        fprf = r > 0 ? FPRF::PZERO : FPRF::_NZERO;
    } else if (c == FP_NAN) {
        fprf = FPRF::QNAN;
    } else if (c == FP_NORMAL) {
        fprf = r > 0 ? FPRF::PNORM : FPRF::NNORM;
    } else if (c == FP_SUBNORMAL) {
        fprf = r > 0 ? FPRF::PDENORM : FPRF::NDENORM;
    }
    return fprf;
}

template <typename T>
inline void completeFPInstr(T a, T b, T c, T r, Rc_t rc, PPUThread* thread) {
    // TODO: set FI
    // TODO: set FR
    auto fpscr = TH->getFPSCR();
    fpscr.fprf.v = getFPRF(r);
    uint32_t fx = fpscr.f.FX;
    if (std::fetestexcept(FE_OVERFLOW)) {
        fx |= fpscr.f.OX == 0;
        fpscr.f.OX = 1;
    }
    if (std::fetestexcept(FE_UNDERFLOW)) {
        fx |= fpscr.f.UX == 0;
        fpscr.f.UX = 1;
    }
    if (issignaling(a) || issignaling(b)) {
        fx |= fpscr.f.VXSNAN == 0;
        fpscr.f.VXSNAN = 1;
    }
    if (std::isinf(a) && std::isinf(b)) {
        if ((a > 0 && b < 0) || (a < 0 && b > 0)) {
            fx |= fpscr.f.VXISI == 0;
            fpscr.f.VXISI = 1;
        }
    }
    fpscr.f.FX = fx;
    TH->setFPSCR(fpscr.v);
    if (rc.u())
        update_CRFSign<1>(r, TH);
}

PRINT(FADD, AForm_2) {
    *result = format_nnn(i->Rc.u() ? "fadd." : "fadd", i->FRT, i->FRA, i->FRB);
}

EMU(FADD, AForm_2) {
    auto a = TH->getFPRd(i->FRA);
    auto b = TH->getFPRd(i->FRB);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a + b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, .0, r, i->Rc, TH);
}

PRINT(FSUB, AForm_2) {
    *result = format_nnn(i->Rc.u() ? "fsub." : "fsub", i->FRT, i->FRA, i->FRB);
}

EMU(FSUB, AForm_2) {
    auto a = TH->getFPRd(i->FRA);
    auto b = TH->getFPRd(i->FRB);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a - b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, .0, r, i->Rc, TH);
}

PRINT(FSUBS, AForm_2) {
    *result = format_nnn(i->Rc.u() ? "fsubs." : "fsubs", i->FRT, i->FRA, i->FRB);
}

EMU(FSUBS, AForm_2) {
    auto a = TH->getFPRd(i->FRA);
    auto b = TH->getFPRd(i->FRB);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a - b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, .0, r, i->Rc, TH);
}

PRINT(FADDS, AForm_2) {
    *result = format_nnn(i->Rc.u() ? "fadds." : "fadds", i->FRT, i->FRA, i->FRB);
}

EMU(FADDS, AForm_2) {
    float a = TH->getFPRd(i->FRA);
    float b = TH->getFPRd(i->FRB);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a + b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, .0f, r, i->Rc, TH);
}

PRINT(FMUL, AForm_3) {
    *result = format_nnn(i->Rc.u() ? "fmul." : "fmul", i->FRT, i->FRA, i->FRC);
}

EMU(FMUL, AForm_3) {
    auto a = TH->getFPRd(i->FRA);
    auto b = TH->getFPRd(i->FRC);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a * b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, .0, r, i->Rc, TH);
}

PRINT(FMULS, AForm_3) {
    *result = format_nnn(i->Rc.u() ? "fmuls." : "fmuls", i->FRT, i->FRA, i->FRC);
}

EMU(FMULS, AForm_3) {
    float a = TH->getFPRd(i->FRA);
    float b = TH->getFPRd(i->FRC);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a * b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, .0f, r, i->Rc, TH);
}

PRINT(FDIV, AForm_2) {
    *result = format_nnn(i->Rc.u() ? "fdiv." : "fdiv", i->FRT, i->FRA, i->FRB);
}

EMU(FDIV, AForm_2) {
    auto a = TH->getFPRd(i->FRA);
    auto b = TH->getFPRd(i->FRB);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a / b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, .0, r, i->Rc, TH);
}

PRINT(FDIVS, AForm_2) {
    *result = format_nnn(i->Rc.u() ? "fdivs." : "fdivs", i->FRT, i->FRA, i->FRB);
}

EMU(FDIVS, AForm_2) {
    float a = TH->getFPRd(i->FRA);
    float b = TH->getFPRd(i->FRB);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a / b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, .0f, r, i->Rc, TH);
}

PRINT(FMADD, AForm_1) {
    *result = format_nnnn(i->Rc.u() ? "fmadd." : "fmadd", i->FRT, i->FRA, i->FRC, i->FRB);
}

EMU(FMADD, AForm_1) {
    auto a = TH->getFPRd(i->FRA);
    auto b = TH->getFPRd(i->FRB);
    auto c = TH->getFPRd(i->FRC);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a * c + b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, c, r, i->Rc, TH);
}

PRINT(FMADDS, AForm_1) {
    *result = format_nnnn(i->Rc.u() ? "fmadds." : "fmadds", i->FRT, i->FRA, i->FRC, i->FRB);
}

EMU(FMADDS, AForm_1) {
    float a = TH->getFPRd(i->FRA);
    float b = TH->getFPRd(i->FRB);
    float c = TH->getFPRd(i->FRC);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a * c + b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, c, r, i->Rc, TH);
}

PRINT(FMSUB, AForm_1) {
    *result = format_nnnn(i->Rc.u() ? "fsub." : "fmsub", i->FRT, i->FRA, i->FRC, i->FRB);
}

EMU(FMSUB, AForm_1) {
    double a = TH->getFPRd(i->FRA);
    double b = TH->getFPRd(i->FRB);
    double c = TH->getFPRd(i->FRC);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a * c - b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, c, r, i->Rc, TH);
}


PRINT(FMSUBS, AForm_1) {
    *result = format_nnnn(i->Rc.u() ? "fsubs." : "fmsubs", i->FRT, i->FRA, i->FRC, i->FRB);
}

EMU(FMSUBS, AForm_1) {
    float a = TH->getFPRd(i->FRA);
    float b = TH->getFPRd(i->FRB);
    float c = TH->getFPRd(i->FRC);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = a * c - b;
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, c, r, i->Rc, TH);
}

PRINT(FNMADD, AForm_1) {
    *result = format_nnnn(i->Rc.u() ? "fnmadd." : "fnmadd", i->FRT, i->FRA, i->FRC, i->FRB);
}

EMU(FNMADD, AForm_1) {
    auto a = TH->getFPRd(i->FRA);
    auto b = TH->getFPRd(i->FRB);
    auto c = TH->getFPRd(i->FRC);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = -(a * c + b);
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, c, r, i->Rc, TH);
}

PRINT(FNMADDS, AForm_1) {
    *result = format_nnnn(i->Rc.u() ? "fnmadds." : "fnmadds", i->FRT, i->FRA, i->FRC, i->FRB);
}

EMU(FNMADDS, AForm_1) {
    float a = TH->getFPRd(i->FRA);
    float b = TH->getFPRd(i->FRB);
    float c = TH->getFPRd(i->FRC);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = -(a * c + b);
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, c, r, i->Rc, TH);
}

PRINT(FNMSUB, AForm_1) {
    *result = format_nnnn(i->Rc.u() ? "fnsub." : "fnmsub", i->FRT, i->FRA, i->FRC, i->FRB);
}

EMU(FNMSUB, AForm_1) {
    double a = TH->getFPRd(i->FRA);
    double b = TH->getFPRd(i->FRB);
    double c = TH->getFPRd(i->FRC);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = -(a * c - b);
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, c, r, i->Rc, TH);
}


PRINT(FNMSUBS, AForm_1) {
    *result = format_nnnn(i->Rc.u() ? "fnsubs." : "fnmsubs", i->FRT, i->FRA, i->FRC, i->FRB);
}

EMU(FNMSUBS, AForm_1) {
    float a = TH->getFPRd(i->FRA);
    float b = TH->getFPRd(i->FRB);
    float c = TH->getFPRd(i->FRC);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto r = -(a * c - b);
    TH->setFPRd(i->FRT, r);
    completeFPInstr(a, b, c, r, i->Rc, TH);
}

PRINT(FCFID, XForm_27) {
    *result = format_nn(i->Rc.u() ? "fcfid." : "fcfid", i->FRT, i->FRB);
}

EMU(FCFID, XForm_27) {
    double b = (int64_t)TH->getFPR(i->FRB);
    TH->setFPRd(i->FRT, b);
    auto fpscr = TH->getFPSCR();
    fpscr.fprf.v = getFPRF(b);
    TH->setFPSCR(fpscr.v);
    // TODO: FX XX
    if (i->Rc.u())
        update_CRFSign<1>(b, TH);
}

PRINT(FCMPU, XForm_17) {
    *result = format_nnn("fcmpu", i->BF, i->FRA, i->FRB);
}

EMU(FCMPU, XForm_17) {
    auto a = TH->getFPRd(i->FRA);
    auto b = TH->getFPRd(i->FRB);
    uint32_t c = std::isnan(a) || std::isnan(b) ? 1
               : a < b ? 8
               : a > b ? 4
               : 2;
    auto fpscr = TH->getFPSCR();
    fpscr.fpcc.v = c;
    TH->setCRF(i->BF.u(), c);
    if (issignaling(a) || issignaling(b)) {
        fpscr.f.FX |= 1;
        fpscr.f.VXSNAN = 1;
    }
    TH->setFPSCR(fpscr.v);
}

PRINT(FCTIWZ, XForm_27) {
    *result = format_nn(i->Rc.u() ? "fctiwz." : "fctiwz", i->FRT, i->FRB);
}

EMU(FCTIWZ, XForm_27) {
    auto b = TH->getFPRd(i->FRB);
    int32_t res = b > 2147483647. ? 0x7fffffff
                : b < -2147483648. ? 0x80000000
                : (int32_t)b;
    // TODO invalid operation, XX
    auto fpscr = TH->getFPSCR();
    if (issignaling(b)) {
        fpscr.f.FX = 1;
        fpscr.f.VXSNAN = 1;
    }
    TH->setFPR(i->FRT, res);
    TH->setFPSCR(fpscr.v);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

PRINT(FCTIDZ, XForm_27) { // test, change rounding mode
    *result = format_nn(i->Rc.u() ? "fctidz." : "fctidz", i->FRT, i->FRB);
}

EMU(FCTIDZ, XForm_27) {
    auto b = TH->getFPRd(i->FRB);
    int32_t res = b > 2147483647. ? 0x7fffffff
    : b < -2147483648. ? 0x80000000
    : (int32_t)b;
    // TODO invalid operation, XX
    auto fpscr = TH->getFPSCR();
    if (issignaling(b)) {
        fpscr.f.FX = 1;
        fpscr.f.VXSNAN = 1;
    }
    TH->setFPR(i->FRT, res);
    TH->setFPSCR(fpscr.v);
    if (i->Rc.u())
        update_CRFSign<1>(res, TH);
}

PRINT(FRSP, XForm_27) {
    *result = format_nn(i->Rc.u() ? "frsp." : "frsp", i->FRT, i->FRB);
}

EMU(FRSP, XForm_27) {
    // TODO: set class etc
    float b = TH->getFPRd(i->FRB);
    TH->setFPRd(i->FRT, b);
}

PRINT(MFFS, XForm_28) {
    *result = format_n(i->Rc.u() ? "mffs." : "mffs", i->FRT);
}

EMU(MFFS, XForm_28) {
    uint32_t fpscr = TH->getFPSCR().v;
    TH->setFPR(i->FRT, fpscr);
    if (i->Rc.u())
        update_CRFSign<1>(fpscr, TH);
}

PRINT(MTFSF, XFLForm) {
    *result = format_nn(i->Rc.u() ? "mtfsf." : "mtfsf", i->FLM, i->FRB);
}

EMU(MTFSF, XFLForm) {
    auto n = __builtin_clzll(i->FLM.u()) - 64 + FLM_t::W;
    auto r = TH->getFPSCRF(n);
    TH->setFPSCRF(n, r);
    if (i->Rc.u())
        update_CRFSign<1>(r, TH);
}

PRINT(LWARX, XForm_1) {
    *result = format_nnn("lwarx", i->RT, i->RA, i->RB);
}

EMU(LWARX, XForm_1) {
    auto ra = getB(i->RA, TH);
    auto ea = ra + TH->getGPR(i->RB);
    big_uint32_t val;
    MM->readReserve(ea, &val, 4);
    TH->setGPR(i->RT, val);
}

PRINT(STWCX, XForm_8) {
    *result = format_nnn("stwcx.", i->RS, i->RA, i->RB);
}

EMU(STWCX, XForm_8) {
    auto ra = getB(i->RA, TH);
    auto ea = ra + TH->getGPR(i->RB);
    big_uint32_t val = TH->getGPR(i->RS);
    auto stored = MM->writeCond(ea, &val, 4);
    TH->setCRF_sign(0, stored);
}

PRINT(LDARX, XForm_1) {
    *result = format_nnn("ldarx", i->RT, i->RA, i->RB);
}

EMU(LDARX, XForm_1) {
    auto ra = getB(i->RA, TH);
    auto ea = ra + TH->getGPR(i->RB);
    big_uint64_t val;
    MM->readReserve(ea, &val, 8);
    TH->setGPR(i->RT, val);
}

PRINT(STDCX, XForm_8) {
    *result = format_nnn("stdcx.", i->RS, i->RA, i->RB);
}

EMU(STDCX, XForm_8) {
    auto ra = getB(i->RA, TH);
    auto ea = ra + TH->getGPR(i->RB);
    big_uint64_t val = TH->getGPR(i->RS);
    auto stored = MM->writeCond(ea, &val, 8);
    TH->setCRF_sign(0, stored);
}

PRINT(SYNC, XForm_24) {
    *result = format_n("sync", i->L);
}

EMU(SYNC, XForm_24) {
    __sync_synchronize();
}

PRINT(ISYNC, XLForm_1) {
    *result = "sync";
}

EMU(ISYNC, XLForm_1) {
    __sync_synchronize();
}

PRINT(EIEIO, XForm_24) {
    *result = "eieio";
}

EMU(EIEIO, XForm_24) {
    __sync_synchronize();
}

PRINT(TD, XForm_25) {
    *result = format_nnn("td", i->TO, i->RA, i->RB);
}

EMU(TD, XForm_25) {
    if (i->TO.u() == 31 && i->RA.u() == i->RB.u())
        throw BreakpointException();
    throw IllegalInstructionException();
}

PRINT(TW, XForm_25) {
    *result = format_nnn("tw", i->TO, i->RA, i->RB);
}

EMU(TW, XForm_25) {
    if (i->TO.u() == 31 && i->RA.u() == i->RB.u())
        throw BreakpointException();
    throw IllegalInstructionException();
}

PRINT(MFTB, XFXForm_2) {
    *result = format_nn("mftb", i->RT, i->tbr);
}

EMU(MFTB, XFXForm_2) {
    auto tbr = i->tbr.u();
    auto tb = TH->proc()->getTimeBase();
    assert(tbr == 392 || tbr == 424);
    if (tbr == 424)
        tb &= 0xffffffff;
    TH->setGPR(i->RT, tb);
}

PRINT(STVX, SIMDForm) {
    *result = format_nnn("stvx", i->vS, i->rA, i->rB);
}

EMU(STVX, SIMDForm) {
    auto b = getB(i->rA, TH);
    auto ea = b + TH->getGPR(i->rB);
    MM->store16(ea, TH->getV(i->vS));
}

PRINT(STVXL, SIMDForm) {
    *result = format_nnn("stvxl", i->vS, i->rA, i->rB);
}

EMU(STVXL, SIMDForm) {
    emulateSTVX(i, cia, thread);
}

PRINT(LVX, SIMDForm) {
    *result = format_nnn("lvx", i->vD, i->rA, i->rB);
}

EMU(LVX, SIMDForm) {
    auto b = getB(i->rA, TH);
    auto ea = b + TH->getGPR(i->rB);
    TH->setV(i->vD, MM->load16(ea));
}

PRINT(LVXL, SIMDForm) {
    *result = format_nnn("lvxl", i->vD, i->rA, i->rB);
}

EMU(LVXL, SIMDForm) {
    emulateLVX(i, cia, thread);
}

PRINT(LVLX, SIMDForm) {
    *result = format_nnn("lvlx", i->vD, i->rA, i->rB);
}

EMU(LVLX, SIMDForm) {
    auto b = getB(i->rA, TH);
    auto ea = b + TH->getGPR(i->rB);
    uint8_t be[16];
    MM->readMemory(ea, be, 16);
    TH->setV(i->vD, be);
}

PRINT(VSLDOI, SIMDForm) {
    *result = format_nnnn("vsldoi", i->vD, i->vA, i->vB, i->SHB);
}

EMU(VSLDOI, SIMDForm) {
    assert(i->SHB.u() <= 16);
    uint8_t be[32];
    TH->getV(i->vA, be);
    TH->getV(i->vB, be + 16);
    TH->setV(i->vD, be + i->SHB.u());
}

PRINT(LVEWX, SIMDForm) { // TODO: test
    *result = format_nnn("lvewx", i->vD, i->rA, i->rB);
}

EMU(LVEWX, SIMDForm) {
    auto b = getB(i->rA, TH);
    auto ea = b + TH->getGPR(i->rB);
    uint8_t be[16];
    MM->readMemory(ea, be, 16);
    TH->setV(i->vD, be);
}

PRINT(VSPLTW, SIMDForm) { // TODO: test
    *result = format_nnn("vspltw", i->vD, i->vB, i->UIMM);
}

EMU(VSPLTW, SIMDForm) {
    uint32_t be[4];
    TH->getV(i->vB, (uint8_t*)be);
    auto b = i->UIMM.u();
    be[0] = be[b];
    be[1] = be[b];
    be[2] = be[b];
    be[3] = be[b];
    TH->setV(i->vD, (uint8_t*)be);
}

PRINT(VSPLTISW, SIMDForm) { // TODO: test
    *result = format_nn("vspltisw", i->vD, i->SIMM);
}

EMU(VSPLTISW, SIMDForm) {
    int32_t v = i->SIMM.s();
    big_uint32_t be[4] = { v, v, v, v };
    TH->setV(i->vD, (uint8_t*)be);
}

PRINT(VMADDFP, SIMDForm) {
    *result = format_nnnn("vmaddfp", i->vD, i->vA, i->vC, i->vB);
}

EMU(VMADDFP, SIMDForm) {
    auto a = TH->getVf(i->vA);
    auto b = TH->getVf(i->vB);
    auto c = TH->getVf(i->vC);
    std::array<float, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = a[i] * c[i] + b[i];
    }
    TH->setVf(i->vD, d);
}

PRINT(VNMSUBFP, SIMDForm) {
    *result = format_nnnn("vnmsubfp", i->vD, i->vA, i->vC, i->vB);
}

EMU(VNMSUBFP, SIMDForm) {
    auto a = TH->getVf(i->vA);
    auto b = TH->getVf(i->vB);
    auto c = TH->getVf(i->vC);
    std::array<float, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = -(a[i] * c[i] - b[i]);
    }
    TH->setVf(i->vD, d);
}

PRINT(VXOR, SIMDForm) {
    *result = format_nnn("vxor", i->vD, i->vA, i->vB);
}

EMU(VXOR, SIMDForm) {
    auto d = TH->getV(i->vA) ^ TH->getV(i->vB);
    TH->setV(i->vD, d);
}

PRINT(VNOR, SIMDForm) {
    *result = format_nnn("vnor", i->vD, i->vA, i->vB);
}

EMU(VNOR, SIMDForm) {
    auto d = ~(TH->getV(i->vA) | TH->getV(i->vB));
    TH->setV(i->vD, d);
}

PRINT(VOR, SIMDForm) {
    *result = format_nnn("vor", i->vD, i->vA, i->vB);
}

EMU(VOR, SIMDForm) {
    auto d = TH->getV(i->vA) | TH->getV(i->vB);
    TH->setV(i->vD, d);
}

PRINT(VAND, SIMDForm) {
    *result = format_nnn("vand", i->vD, i->vA, i->vB);
}

EMU(VAND, SIMDForm) {
    auto d = TH->getV(i->vA) & TH->getV(i->vB);
    TH->setV(i->vD, d);
}

PRINT(VSEL, SIMDForm) {
    *result = format_nnnn("vsel", i->vD, i->vA, i->vB, i->vC);
}

EMU(VSEL, SIMDForm) {
    auto m = TH->getV(i->vC);
    auto a = TH->getV(i->vA);
    auto b = TH->getV(i->vB);
    auto d = (m & b) | (~m & a);
    TH->setV(i->vD, d);
}

PRINT(VADDFP, SIMDForm) {
    *result = format_nnn("vaddfp", i->vD, i->vA, i->vB);
}

EMU(VADDFP, SIMDForm) {
    auto a = TH->getVf(i->vA);
    auto b = TH->getVf(i->vB);
    std::array<float, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = a[i] + b[i];
    }
    TH->setVf(i->vD, d);
}

PRINT(VSUBFP, SIMDForm) {
    *result = format_nnn("vsubfp", i->vD, i->vA, i->vB);
}

EMU(VSUBFP, SIMDForm) {
    auto a = TH->getVf(i->vA);
    auto b = TH->getVf(i->vB);
    std::array<float, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = a[i] - b[i];
    }
    TH->setVf(i->vD, d);
}

PRINT(VRSQRTEFP, SIMDForm) {
    *result = format_nn("vrsqrtefp", i->vD, i->vB);
}

EMU(VRSQRTEFP, SIMDForm) {
    auto b = TH->getVf(i->vB);
    std::array<float, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = 1.f / sqrt(b[i]);
    }
    TH->setVf(i->vD, d);
}

PRINT(VCTSXS, SIMDForm) {
    *result = format_nnn("vctsxs", i->vD, i->vB, i->UIMM);
}

EMU(VCTSXS, SIMDForm) {
    auto b = TH->getVf(i->vB);
    std::array<uint32_t, 4> d;
    auto m = 1u << i->UIMM.u();
    for (int i = 0; i < 4; ++i) {
        d[i] = b[i] * m;
    }
    TH->setVuw(i->vD, d);
}

PRINT(VCFSX, SIMDForm) {
    *result = format_nnn("vcfsx", i->vD, i->vB, i->UIMM);
}

EMU(VCFSX, SIMDForm) {
    auto b = TH->getVw(i->vB);
    auto div = 1u << i->UIMM.u();
    std::array<float, 4> d;
    for (int n = 0; n < 4; ++n) {
        d[n] = b[n];
        d[n] /= div;
    }
    TH->setVf(i->vD, d);
}

PRINT(VADDUWM, SIMDForm) {
    *result = format_nnn("vadduwm", i->vD, i->vA, i->vB);
}

EMU(VADDUWM, SIMDForm) {
    auto a = TH->getVuw(i->vA);
    auto b = TH->getVuw(i->vB);
    std::array<uint32_t, 4> d;
    for (int n = 0; n < 4; ++n) {
        d[n] = a[n] + b[n];
    }
    TH->setVuw(i->vD, d);
}

PRINT(VSUBUWM, SIMDForm) {
    *result = format_nnn("vsubuwm", i->vD, i->vA, i->vB);
}

EMU(VSUBUWM, SIMDForm) {
    auto a = TH->getVuw(i->vA);
    auto b = TH->getVuw(i->vB);
    std::array<uint32_t, 4> d;
    for (int n = 0; n < 4; ++n) {
        d[n] = a[n] - b[n];
    }
    TH->setVuw(i->vD, d);
}

PRINT(VCMPEQUW, SIMDForm) {
    *result = format_nnn(i->Rc.u() ? "vcmpequw." : "vcmpequw", i->vD, i->vA, i->vB);
}

EMU(VCMPEQUW, SIMDForm) {
    auto a = TH->getVuw(i->vA);
    auto b = TH->getVuw(i->vB);
    std::array<uint32_t, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = a[i] == b[i] ? 0xffffffff : 0;
    }
    TH->setVuw(i->vD, d);
    if (i->Rc.u()) {
        auto d128 = TH->getV(i->vD);
        auto t = ~d128 == 0;
        auto f = d128 == 0;
        auto c = t << 3 | f << 1;
        TH->setCRF(6, c);  
    }
}

PRINT(VCMPGTFP, SIMDForm) {
    *result = format_nnn(i->Rc.u() ? "vcmpgtfp." : "vcmpgtfp", i->vD, i->vA, i->vB);
}

EMU(VCMPGTFP, SIMDForm) {
    auto a = TH->getVf(i->vA);
    auto b = TH->getVf(i->vB);
    std::array<uint32_t, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = a[i] > b[i] ? 0xffffffff : 0;
    }
    TH->setVuw(i->vD, d);
    if (i->Rc.u()) {
        auto d128 = TH->getV(i->vD);
        auto t = ~d128 == 0;
        auto f = d128 == 0;
        auto c = t << 3 | f << 1;
        TH->setCRF(6, c);
    }
}

PRINT(VCMPGTUW, SIMDForm) {
    *result = format_nnn(i->Rc.u() ? "vcmpgtuw." : "vcmpgtuw", i->vD, i->vA, i->vB);
}

EMU(VCMPGTUW, SIMDForm) {
    auto a = TH->getVuw(i->vA);
    auto b = TH->getVuw(i->vB);
    std::array<uint32_t, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = a[i] > b[i] ? 0xffffffff : 0;
    }
    TH->setVuw(i->vD, d);
    if (i->Rc.u()) {
        auto d128 = TH->getV(i->vD);
        auto t = ~d128 == 0;
        auto f = d128 == 0;
        auto c = t << 3 | f << 1;
        TH->setCRF(6, c);
    }
}

PRINT(VCMPGTSW, SIMDForm) {
    *result = format_nnn(i->Rc.u() ? "vcmpgtsw." : "vcmpgtsw", i->vD, i->vA, i->vB);
}

EMU(VCMPGTSW, SIMDForm) {
    auto a = TH->getVw(i->vA);
    auto b = TH->getVw(i->vB);
    std::array<uint32_t, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = a[i] > b[i] ? 0xffffffff : 0;
    }
    TH->setVuw(i->vD, d);
    if (i->Rc.u()) {
        auto d128 = TH->getV(i->vD);
        auto t = ~d128 == 0;
        auto f = d128 == 0;
        auto c = t << 3 | f << 1;
        TH->setCRF(6, c);
    }
}

PRINT(VCMPEQFP, SIMDForm) { // TODO: test
    *result = format_nnn(i->Rc.u() ? "vcmpeqfp." : "vcmpeqfp", i->vD, i->vA, i->vB);
}

EMU(VCMPEQFP, SIMDForm) {
    auto a = TH->getVf(i->vA);
    auto b = TH->getVf(i->vB);
    std::array<uint32_t, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = a[i] == b[i] ? 0xffffffff : 0;
    }
    TH->setVuw(i->vD, d);
    if (i->Rc.u()) {
        auto d128 = TH->getV(i->vD);
        auto t = ~d128 == 0;
        auto f = d128 == 0;
        auto c = t << 3 | f << 1;
        TH->setCRF(6, c);
    }
}

PRINT(VSRW, SIMDForm) {
    *result = format_nnn("vsrw", i->vD, i->vA, i->vB);
}

EMU(VSRW, SIMDForm) {
    auto a = TH->getVuw(i->vA);
    auto b = TH->getVuw(i->vB);
    std::array<uint32_t, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = a[i] >> (b[i] & 31);
    }
    TH->setVuw(i->vD, d);
}

PRINT(VSLW, SIMDForm) {
    *result = format_nnn("vslw", i->vD, i->vA, i->vB);
}

EMU(VSLW, SIMDForm) {
    auto a = TH->getVuw(i->vA);
    auto b = TH->getVuw(i->vB);
    std::array<uint32_t, 4> d;
    for (int i = 0; i < 4; ++i) {
        d[i] = a[i] << (b[i] & 31);
    }
    TH->setVuw(i->vD, d);
}

PRINT(VMRGHW, SIMDForm) {
    *result = format_nnn("vmrghw", i->vD, i->vA, i->vB);
}

EMU(VMRGHW, SIMDForm) {
    uint32_t abe[4];
    uint32_t bbe[4];
    TH->getV(i->vA, (uint8_t*)abe);
    TH->getV(i->vB, (uint8_t*)bbe);
    uint32_t dbe[4];
    dbe[0] = abe[0];
    dbe[1] = bbe[0];
    dbe[2] = abe[1];
    dbe[3] = bbe[1];
    TH->setV(i->vD, (uint8_t*)dbe);
}

PRINT(VMRGLW, SIMDForm) { // TODO: test
    *result = format_nnn("vmrglw", i->vD, i->vA, i->vB);
}

EMU(VMRGLW, SIMDForm) {
    uint32_t abe[4];
    uint32_t bbe[4];
    TH->getV(i->vA, (uint8_t*)abe);
    TH->getV(i->vB, (uint8_t*)bbe);
    uint32_t dbe[4];
    dbe[0] = abe[2];
    dbe[1] = bbe[2];
    dbe[2] = abe[3];
    dbe[3] = bbe[3];
    TH->setV(i->vD, (uint8_t*)dbe);
}

PRINT(VPERM, SIMDForm) { // TODO: test
    *result = format_nnnn("vperm", i->vD, i->vA, i->vB, i->vC);
}

EMU(VPERM, SIMDForm) {
    uint8_t be[32];
    TH->getV(i->vA, be);
    TH->getV(i->vB, be + 16);
    uint8_t cbe[16];
    TH->getV(i->vC, cbe);
    uint8_t dbe[16] = { 0 };
    for (int i = 0; i < 16; ++i) {
        auto b = cbe[i] & 31;
        dbe[i] = be[b];
    }
    TH->setV(i->vD, dbe);
}

PRINT(LVSR, SIMDForm) { // TODO: test
    *result = format_nnn("lvsr", i->vD, i->rA, i->rB);
}

EMU(LVSR, SIMDForm) {
    auto b = getB(i->rA, TH);
    auto ea = b + TH->getGPR(i->rB);
    auto sh = ea & 15;
    unsigned __int128 vd = 0;
    switch (sh) {
        case 0: vd = make128(0x1011121314151617ull, 0x18191A1B1C1D1E1Full); break;
        case 1: vd = make128(0x0F10111213141516ull, 0x1718191A1B1C1D1Eull); break;
        case 2: vd = make128(0x0E0F101112131415ull, 0x161718191A1B1C1Dull); break;
        case 3: vd = make128(0x0D0E0F1011121314ull, 0x15161718191A1B1Cull); break;
        case 4: vd = make128(0x0C0D0E0F10111213ull, 0x1415161718191A1Bull); break;
        case 5: vd = make128(0x0B0C0D0E0F101112ull, 0x131415161718191Aull); break;
        case 6: vd = make128(0x0A0B0C0D0E0F1011ull, 0x1213141516171819ull); break;
        case 7: vd = make128(0x090A0B0C0D0E0F10ull, 0x1112131415161718ull); break;
        case 8: vd = make128(0x08090A0B0C0D0E0Full, 0x1011121314151617ull); break;
        case 9: vd = make128(0x0708090A0B0C0D0Eull, 0x0F10111213141516ull); break;
        case 10: vd = make128(0x060708090A0B0C0Dull, 0x0E0F101112131415ull); break;
        case 11: vd = make128(0x05060708090A0B0Cull, 0x0D0E0F1011121314ull); break;
        case 12: vd = make128(0x0405060708090A0Bull, 0x0C0D0E0F10111213ull); break;
        case 13: vd = make128(0x030405060708090Aull, 0x0B0C0D0E0F101112ull); break;
        case 14: vd = make128(0x0203040506070809ull, 0x0A0B0C0D0E0F1011ull); break;
        case 15: vd = make128(0x0102030405060708ull, 0x090A0B0C0D0E0F10ull); break;
    }
    TH->setV(i->vD, vd);
}

PRINT(LVSL, SIMDForm) {
    *result = format_nnn("lvsl", i->vD, i->rA, i->rB);
}

EMU(LVSL, SIMDForm) {
    auto b = getB(i->rA, TH);
    auto ea = b + TH->getGPR(i->rB);
    auto sh = ea & 15;
    unsigned __int128 vd = 0;
    switch (sh) {
        case 0: vd = make128(0x0001020304050607ull, 0x08090A0B0C0D0E0Full); break;
        case 1: vd = make128(0x0102030405060708ull, 0x090A0B0C0D0E0F10ull); break;
        case 2: vd = make128(0x0203040506070809ull, 0x0A0B0C0D0E0F1011ull); break;
        case 3: vd = make128(0x030405060708090Aull, 0x0B0C0D0E0F101112ull); break;
        case 4: vd = make128(0x0405060708090A0Bull, 0x0C0D0E0F10111213ull); break;
        case 5: vd = make128(0x05060708090A0B0Cull, 0x0D0E0F1011121314ull); break;
        case 6: vd = make128(0x060708090A0B0C0Dull, 0x0E0F101112131415ull); break;
        case 7: vd = make128(0x0708090A0B0C0D0Eull, 0x0F10111213141516ull); break;
        case 8: vd = make128(0x08090A0B0C0D0E0Full, 0x1011121314151617ull); break;
        case 9: vd = make128(0x090A0B0C0D0E0F10ull, 0x1112131415161718ull); break;
        case 10: vd = make128(0x0A0B0C0D0E0F1011ull, 0x1213141516171819ull); break;
        case 11: vd = make128(0x0B0C0D0E0F101112ull, 0x131415161718191Aull); break;
        case 12: vd = make128(0x0C0D0E0F10111213ull, 0x1415161718191A1Bull); break;
        case 13: vd = make128(0x0D0E0F1011121314ull, 0x15161718191A1B1Cull); break;
        case 14: vd = make128(0x0E0F101112131415ull, 0x161718191A1B1C1Dull); break;
        case 15: vd = make128(0x0F10111213141516ull, 0x1718191A1B1C1D1Eull); break;
    }
    TH->setV(i->vD, vd);
}

PRINT(VANDC, SIMDForm) { // TODO: test
    *result = format_nnn("vandc", i->vD, i->vA, i->vB);
}

EMU(VANDC, SIMDForm) {
    auto a = TH->getV(i->vA);
    auto b = TH->getV(i->vB);
    auto d = a & ~b;
    TH->setV(i->vD, d);
}

PRINT(VREFP, SIMDForm) {
    *result = format_nn("vrefp", i->vD, i->vB);
}

EMU(VREFP, SIMDForm) {
    std::array<float, 4> d;
    auto b = TH->getVf(i->vB);
    for (int i = 0; i < 4; ++i) {
        d[i] = 1.f / b[i];
    }
    TH->setVf(i->vD, d);
}

PRINT(VSRAW, SIMDForm) {
    *result = format_nnn("vsraw", i->vD, i->vA, i->vB);
}

EMU(VSRAW, SIMDForm) {
    std::array<int32_t, 4> d;
    auto a = TH->getVw(i->vA);
    auto b = TH->getVuw(i->vB);
    for (int i = 0; i < 4; ++i) {
        auto sh = b[i] & 31;
        d[i] = a[i] >> sh;
    }
    TH->setVw(i->vD, d);
}

PRINT(VRFIP, SIMDForm) {
    *result = format_nn("vrfip", i->vD, i->vB);
}

EMU(VRFIP, SIMDForm) {
    std::array<float, 4> d;
    auto b = TH->getVf(i->vB);
    for (int i = 0; i < 4; ++i) {
        d[i] = std::ceil(b[i]);
    }
    TH->setVf(i->vD, d);
}

PRINT(VRFIM, SIMDForm) {
    *result = format_nn("vrfim", i->vD, i->vB);
}

EMU(VRFIM, SIMDForm) {
    std::array<float, 4> d;
    auto b = TH->getVf(i->vB);
    for (int i = 0; i < 4; ++i) {
        d[i] = std::floor(b[i]);
    }
    TH->setVf(i->vD, d);
}

PRINT(VRFIZ, SIMDForm) {
    *result = format_nn("vrfiz", i->vD, i->vB);
}

EMU(VRFIZ, SIMDForm) {
    std::array<float, 4> d;
    auto b = TH->getVf(i->vB);
    for (int i = 0; i < 4; ++i) {
        d[i] = std::trunc(b[i]);
    }
    TH->setVf(i->vD, d);
}

PRINT(VMAXFP, SIMDForm) {
    *result = format_nnn("vmaxfp", i->vD, i->vA, i->vB);
}

EMU(VMAXFP, SIMDForm) {
    std::array<float, 4> d;
    auto a = TH->getVf(i->vA);
    auto b = TH->getVf(i->vB);
    for (int i = 0; i < 4; ++i) {
        d[i] = std::max(a[i], b[i]);
    }
    TH->setVf(i->vD, d);
}

PRINT(VMINFP, SIMDForm) {
    *result = format_nnn("vminfp", i->vD, i->vA, i->vB);
}

EMU(VMINFP, SIMDForm) {
    std::array<float, 4> d;
    auto a = TH->getVf(i->vA);
    auto b = TH->getVf(i->vB);
    for (int i = 0; i < 4; ++i) {
        d[i] = std::min(a[i], b[i]);
    }
    TH->setVf(i->vD, d);
}

PRINT(VCTUXS, SIMDForm) {
    *result = format_nnn("vctuxs", i->vD, i->vB, i->UIMM);
}

EMU(VCTUXS, SIMDForm) { // TODO: SAT
    std::array<uint32_t, 4> d;
    auto b = TH->getVf(i->vB);
    auto scale = 1 << i->UIMM.u();
    for (int i = 0; i < 4; ++i) {
        d[i] = b[i] * scale;
    }
    TH->setVuw(i->vD, d);
}

PRINT(STVEWX, SIMDForm) {
    *result = format_nnn("stvewx", i->vS, i->rA, i->rB);
}

EMU(STVEWX, SIMDForm) { // TODO: SAT
    auto b = getB(i->rA, TH);
    auto ea = b + TH->getGPR(i->rB);
    auto s = TH->getVuw(i->vS);
    auto eb = ea & 3;
    MM->store<4>(ea, s[eb]);
}

PRINT(DCBZ, XForm_1) {
    *result = format_nn("dcbz", i->RA, i->RB);
}

EMU(DCBZ, XForm_1) {
    auto b = getB(i->RA, TH);
    auto ea = b + TH->getGPR(i->RB);
    auto line = ea & ~127;
    MM->setMemory(line, 0, 128);
}

struct PPUDasmInstruction {
    const char* mnemonic;
    std::string operands;
};

#define invoke(name) invoke_impl<M>(#name, print##name, emulate##name, &x, cia, state); break

template <DasmMode M, typename S>
void ppu_dasm(void* instr, uint64_t cia, S* state) {
    uint32_t x = big_to_native<uint32_t>(*reinterpret_cast<uint32_t*>(instr));
    auto iform = reinterpret_cast<IForm*>(&x);
    switch (iform->OPCD.u()) {
        case 1: invoke(NCALL);
        case 4: {
            auto simd = reinterpret_cast<SIMDForm*>(&x);
            switch (simd->VA_XO.u()) {
                case 46: invoke(VMADDFP);
                case 47: invoke(VNMSUBFP);
                case 42: invoke(VSEL);
                case 43: invoke(VPERM);
                case 44: invoke(VSLDOI);
                default:
                    switch (simd->VXR_XO.u()) {
                        case 198: invoke(VCMPEQFP);
                        case 646: invoke(VCMPGTUW);
                        case 710: invoke(VCMPGTFP);
                        case 902: invoke(VCMPGTSW);
                        default:
                            switch (simd->VX_XO.u()) {
                                case 1220: invoke(VXOR);
                                case 652: invoke(VSPLTW);
                                case 908: invoke(VSPLTISW);
                                case 10: invoke(VADDFP);
                                case 74: invoke(VSUBFP);
                                case 970: invoke(VCTSXS);
                                case 1028: invoke(VAND);
                                case 1284: invoke(VNOR);
                                case 1156: invoke(VOR);
                                case 128: invoke(VADDUWM);
                                case 1152: invoke(VSUBUWM);
                                case 134: invoke(VCMPEQUW);
                                case 388: invoke(VSLW);
                                case 644: invoke(VSRW);
                                case 842: invoke(VCFSX);
                                case 140: invoke(VMRGHW);
                                case 396: invoke(VMRGLW);
                                case 330: invoke(VRSQRTEFP);
                                case 1092: invoke(VANDC);
                                case 266: invoke(VREFP);
                                case 900: invoke(VSRAW);
                                case 650: invoke(VRFIP);
                                case 714: invoke(VRFIM);
                                case 1034: invoke(VMAXFP);
                                case 1098: invoke(VMINFP);
                                case 586: invoke(VRFIZ);
                                case 906: invoke(VCTUXS);
                                default: throw IllegalInstructionException();
                            }
                    }
            }
            break;
        }
        case 8: invoke(SUBFIC);
        case 7: invoke(MULLI);
        case 10: invoke(CMPLI);
        case 11: invoke(CMPI);
        case 12: invoke(ADDIC);
        case 13: invoke(ADDICD);
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
                case 150: invoke(ISYNC);
                default: throw IllegalInstructionException();
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
                default: throw IllegalInstructionException();
            }
            break;
        }
        case 31: {
            auto xform = reinterpret_cast<XForm_1*>(&x);
            auto xsform = reinterpret_cast<XSForm*>(&x);
            auto xoform = reinterpret_cast<XOForm_1*>(&x);
            if (xoform->XO.u() == 40) {
                invoke(SUBF);
            } else if (xoform->XO.u() == 202) {
                invoke(ADDZE);
            } else if (xsform->XO.u() == 413) {
                invoke(SRADI);
            } else
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
                case 19: invoke(MFCR);
                case 27: invoke(SLD);
                case 24: invoke(SLW);
                case 539: invoke(SRD);
                case 536: invoke(SRW);
                case 104: invoke(NEG);
                case 144: invoke(MTOCRF);
                case 278: invoke(DCBT);
                case 58: invoke(CNTLZD);
                case 26: invoke(CNTLZW);
                case 824: invoke(SRAWI);
                case 459: invoke(DIVWU);
                case 235: invoke(MULLW);
                case 457: invoke(DIVDU);
                case 233: invoke(MULLD);
                case 489: invoke(DIVD);
                case 535: invoke(LFSX);
                case 567: invoke(LFSUX);
                case 599: invoke(LFDX);
                case 631: invoke(LFDUX);
                case 663: invoke(STFSX);
                case 695: invoke(STFSUX);
                case 727: invoke(STFDX);
                case 759: invoke(STFDUX);
                case 75: invoke(MULHW);
                case 983: invoke(STFIWX);
                case 491: invoke(DIVW);
                case 73: invoke(MULHD);
                case 598: invoke(SYNC);
                case 231: invoke(STVX);
                case 487: invoke(STVX);
                case 103: invoke(LVX);
                case 359: invoke(LVXL);
                case 519: invoke(LVLX);
                case 371: invoke(MFTB);
                case 9: invoke(MULHDU);
                case 71: invoke(LVEWX);
                case 11: invoke(MULHWU);
                case 38: invoke(LVSR);
                case 6: invoke(LVSL);
                case 68: invoke(TD);
                case 4: invoke(TW);
                case 10: invoke(ADDC);
                case 8: invoke(SUBFC);
                case 20: invoke(LWARX);
                case 84: invoke(LDARX);
                case 150: invoke(STWCX);
                case 214: invoke(STDCX);
                case 199: invoke(STVEWX);
                case 854: invoke(EIEIO);
                case 1014: invoke(DCBZ);
                case 792: invoke(SRAW);
                default: throw IllegalInstructionException();
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
        case 48: invoke(LFS);
        case 49: invoke(LFSU);
        case 50: invoke(LFD);
        case 51: invoke(LFDU);
        case 52: invoke(STFS);
        case 53: invoke(STFSU);
        case 54: invoke(STFD);
        case 55: invoke(STFDU);
        case 58: {
            auto dsform = reinterpret_cast<DSForm_1*>(&x);
            switch (dsform->XO.u()) {
                case 2: invoke(LWA);
                case 0: invoke(LD);
                case 1: invoke(LDU);
                default: throw IllegalInstructionException();
            }
            break;
        }
        case 59: {
            auto aform = reinterpret_cast<AForm_1*>(&x);
            switch (aform->XO.u()) {
                case 21: invoke(FADDS);
                case 25: invoke(FMULS);
                case 18: invoke(FDIVS);
                case 28: invoke(FMSUBS);
                case 29: invoke(FMADDS);
                case 20: invoke(FSUBS);
                case 31: invoke(FNMADDS);
                case 30: invoke(FNMSUB);
                case 22: invoke(FSQRTS);
                case 24: invoke(FRES);
                case 26: invoke(FRSQRTES);
                default: throw IllegalInstructionException();
            }
            break;
        }
        case 62: {
            auto dsform = reinterpret_cast<DSForm_2*>(&x);
            switch (dsform->XO.u()) {
                case 0: invoke(STD);
                case 1: invoke(STDU);
                default: throw IllegalInstructionException();
            }
            break;
        }
        case 63: {
            auto xform = reinterpret_cast<XForm_1*>(&x);
            auto aform = reinterpret_cast<AForm_1*>(&x);
            if (aform->XO.u() == 21) {
                invoke(FADD);
            } else if (aform->XO.u() == 25) {
                invoke(FMUL);
            } else if (aform->XO.u() == 18) {
                invoke(FDIV);
            } else if (aform->XO.u() == 29) {
                invoke(FMADD);
            } else if (aform->XO.u() == 28) {
                invoke(FMSUB);
            } else if (aform->XO.u() == 20) {
                invoke(FSUB);
            } else if (aform->XO.u() == 31) {
                invoke(FNMADD);
            } else if (aform->XO.u() == 30) {
                invoke(FNMSUB);
            } else if (aform->XO.u() == 22) {
                invoke(FSQRT);
            } else if (aform->XO.u() == 24) {
                invoke(FRE);
            } else if (aform->XO.u() == 26) {
                invoke(FRSQRTE);
            } else
            switch (xform->XO.u()) {
                case 72: invoke(FMR);
                case 40: invoke(FNEG);
                case 264: invoke(FABS);
                case 136: invoke(FNABS);
                case 846: invoke(FCFID);
                case 0: invoke(FCMPU);
                case 15: invoke(FCTIWZ);
                case 815: invoke(FCTIDZ);
                case 12: invoke(FRSP);
                case 583: invoke(MFFS);
                case 711: invoke(MTFSF);
                default: throw IllegalInstructionException();
            }
            break;
        }
        default: throw std::runtime_error(ssnprintf("unknown opcode at %08x", cia));
    }
}

BranchMnemonicType getExtBranchMnemonic(
    bool lr, bool abs, bool tolr, bool toctr, BO_t btbo, BI_t bi, std::string& mnemonic)
{
    auto type = BranchMnemonicType::Generic;
    auto bo = btbo.u();
    // see fig 21
    auto at = (bo >> 2 == 1 || bo >> 2 == 3) ? bo & 3
            : ((bo & 22) == 16 || (bo & 22) == 18) ? (((bo >> 2) & 2) | (bo & 1))
            : 0;
    auto atchar = at == 2 ? "-" : at == 3 ? "+" : "";

    std::string m;
    if (bo >> 2 == 3 || bo >> 2 == 1) { // table 4
        bool crbiSet = ((bo >> 2) & 3) == 3;
        const char* f = nullptr;
        switch (bi.u() % 4) {
        case 0:
            f = crbiSet ? "lt" : "ge";
            break;
        case 1:
            f = crbiSet ? "gt" : "le";
            break;
        case 2:
            f = crbiSet ? "eq" : "ne";
            break;
        case 3:
            f = crbiSet ? "so" : "ns";
        }

        m = ssnprintf("b%s%s%s%s%s",
                      f,
                      tolr ? "lr" : "",
                      toctr ? "ctr" : "",
                      lr ? "l" : "",
                      abs ? "a" : "");
        type = BranchMnemonicType::ExtCondition;
    } else { // table 3
        if ((bo & 20) == 20) { // branch unconditionally
            if (lr) {
                if (tolr) m = "blrl";
                if (toctr) m = "bctrl";
            } else {
                if (tolr) m = "blr";
                if (toctr) m = "bctr";
            }
            if (tolr || toctr) {
                type = BranchMnemonicType::ExtSimple;
            }
        }
        // "Branch if CR_BI=1|0" overlaps with table 4 and table 4 is preferred
        else if (!toctr) {
            const char* sem;
            if ((bo & 22) == 16) {
                sem = "nz";
            } else if ((bo & 22) == 18) {
                sem = "z";
            } else if ((bo >> 1) == 4) {
                sem = "nzt";
            } else if ((bo >> 1) == 5) {
                sem = "zt";
            } else if ((bo >> 1) == 0) {
                sem = "nzf";
            } else if ((bo >> 1) == 1) {
                sem = "zf";
            } else {
                throw std::runtime_error("invalid instruction");
            }
            m = ssnprintf("bd%s%s%s%s",
                          sem,
                          tolr ? "lr" : "",
                          lr ? "l" : "",
                          abs ? "a" : "");
            type = BranchMnemonicType::ExtSimple;
        }
    }
    if (type != BranchMnemonicType::Generic) {
        mnemonic = m + atchar;
    }
    return type;
}

std::string formatCRbit(BI_t bi) {
    const char* crbit = nullptr;
    switch (bi.u() % 4) {
    case 0:
        crbit = "lt";
        break;
    case 1:
        crbit = "gt";
        break;
    case 2:
        crbit = "eq";
        break;
    case 3:
        crbit = "so";
    }
    return ssnprintf("4*cr%d+%s", bi.u() / 4, crbit);
}

bool isAbsoluteBranch(void* instr) {
    uint32_t x = big_to_native<uint32_t>(*reinterpret_cast<uint32_t*>(instr));
    auto iform = reinterpret_cast<IForm*>(&x);
    return iform->OPCD.u() == 18 || iform->OPCD.u() == 16;
}

bool isTaken(void* branchInstr, uint64_t cia, PPUThread* thread) {
    uint32_t x = big_to_native<uint32_t>(*reinterpret_cast<uint32_t*>(branchInstr));
    auto iform = reinterpret_cast<IForm*>(&x);
    if (iform->OPCD.u() == 18) {
        return true;
    } else if (iform->OPCD.u() == 16) {
        auto b = reinterpret_cast<BForm*>(&x);
        return isTaken(b->BO0, b->BO1, b->BO2, b->BO3, thread, b->BI);
    }
    throw std::runtime_error("not absolute branch");
}

uint64_t getTargetAddress(void* branchInstr, uint64_t cia) {
    uint32_t x = big_to_native<uint32_t>(*reinterpret_cast<uint32_t*>(branchInstr));
    auto iform = reinterpret_cast<IForm*>(&x);
    if (iform->OPCD.u() == 18) {
        return getNIA(iform, cia);
    } else if (iform->OPCD.u() == 16) {
        return getNIA(reinterpret_cast<BForm*>(&x), cia);
    }
    throw std::runtime_error("not absolute branch");
}

template void ppu_dasm<DasmMode::Print, std::string>(
    void* instr, uint64_t cia, std::string* state);

template void ppu_dasm<DasmMode::Emulate, PPUThread>(
    void* instr, uint64_t cia, PPUThread* th);

template void ppu_dasm<DasmMode::Name, std::string>(
    void* instr, uint64_t cia, std::string* name);
