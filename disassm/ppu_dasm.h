#pragma once

#include "../ps3emu/PPU.h"
#include "ppu_dasm_forms.h"

#include <boost/type_traits.hpp>
#include <boost/format.hpp>
#include <boost/endian/conversion.hpp>
#include <string>
#include <stdint.h>
#include <bitset>

using namespace boost::endian;
using namespace boost;

#define PRINT(name, form) inline void print##name(form* i, uint64_t cia, std::string* result)
#define EMU(name, form) inline void emulate##name(form* i, uint64_t cia, PPU* ppu)

template <int Pos, int Next>
inline uint8_t bit_test(uint64_t number, int width, BitField<Pos, Next> bf) {
    auto n = bf.u();
    return (number & (1 << (width - n))) >> (width - n);
}

template <typename T, int Pos, int Next>
T bit_test(T number, BitField<Pos, Next> bf) {
    return bit_test(number, sizeof(T) * 8, bf);
}

template <typename T>
uint64_t bit_set(T number, int n) {
    return number | (1 << (sizeof(T) * 8 - n));
}

// Branch I-form, p24

inline uint64_t getNIA(IForm* i, uint64_t cia) {
    auto ext = i->LI << 2;
    return i->AA.u() ? ext : (cia + ext);
}

PRINT(B, IForm) {
    const char* mnemonics[][2] = {
        { "b", "ba" }, { "bl", "bla" }  
    };
    auto mnemonic = mnemonics[i->LK.u()][i->AA.u()];
    *result = str(format("%s %x") % mnemonic % getNIA(i, cia));
}

EMU(B, IForm) {
    ppu->setNIP(getNIA(i, cia));
    if (i->LK.u())
        ppu->setLR(cia + 4);
}

// Branch Conditional B-form, p24

inline uint64_t getNIA(BForm* i, uint64_t cia) {
    auto ext = i->BD << 2;
    return i->AA.u() ? ext : (ext + cia);
}

PRINT(BC, BForm) {
    const char* mnemonics[][2] = {
        { "bc", "bca" }, { "bcl", "bcla" }  
    };
    auto mnemonic = mnemonics[i->LK.u()][i->AA.u()];
    *result = str(format("%s %d,%d,%x") 
        % mnemonic % i->BO.u() % i->BI.u() % getNIA(i, cia));
}

template <int P1, int P2, int P3, int P4, int P5>
inline bool is_taken(BitField<P1, P1 + 1> bo0, 
                     BitField<P2, P2 + 1> bo1,
                     BitField<P3, P3 + 1> bo2,
                     BitField<P4, P4 + 1> bo3,
                     PPU* ppu,
                     BitField<P5, P5 + 5> bi)
{
    auto ctr_ok = bo2.u() || ((ppu->getCTR() != 0) ^ bo3.u());
    auto cond_ok = bo0.u() || bit_test(ppu->getCR(), 32, bi) == bo1.u();
    return ctr_ok && cond_ok;
}

EMU(BC, BForm) {
    if (!i->BO2.u())
        ppu->setCTR(ppu->getCTR() - 1);
    if (is_taken(i->BO0, i->BO1, i->BO2, i->BO3, ppu, i->BI))
        ppu->setNIP(getNIA(i, cia));
    if (i->LK.u())
        ppu->setLR(cia + 4);
}

// Branch Conditional to Link Register XL-form, p25

template <typename OP1, typename OP2, typename OP3>
std::string format_3d(const char* mnemonic, OP1 op1, OP2 op2, OP3 op3) {
    return str(format("%s %d,%d,%d") % mnemonic % getSValue(op1) % getSValue(op2) % getSValue(op3));
}

template <typename OP1, typename OP2, typename OP3, typename OP4>
std::string format_4d(const char* mnemonic, OP1 op1, OP2 op2, OP3 op3, OP4 op4) {
    return str(format("%s %d,%d,%d") 
        % mnemonic % getSValue(op1) % getSValue(op2) % getSValue(op3) % getSValue(op4));
}

template <typename OP1, typename OP2>
std::string format_2d(const char* mnemonic, OP1 op1, OP2 op2) {
    return str(format("%s %d,%d") % mnemonic % getSValue(op1) % getSValue(op2));
}

template <typename OP1>
std::string format_1d(const char* mnemonic, OP1 op1) {
    return str(format("%s %d") % mnemonic % getSValue(op1));
}

template <typename OP1, typename OP2, typename OP3>
std::string format_3d_3bracket(const char* mnemonic, OP1 op1, OP2 op2, OP3 op3) {
    return str(format("%s %d,%d(%d)") % mnemonic % getSValue(op1) % getSValue(op2) % getSValue(op3));
}

template <typename BF>
inline int64_t getB(BF ra, PPU* ppu) {
    return ra.u() == 0 ? 0 : ppu->getGPR(ra);
}

PRINT(BCLR, XLForm_2) {
    auto mnemonic = i->LK.u() ? "bclrl" : "bclr";
    *result = format_3d(mnemonic, i->BO, i->BI, i->BH);
}

EMU(BCLR, XLForm_2) {
    if (!i->BO2.u())
        ppu->setCTR(ppu->getCTR() - 1);
    if (is_taken(i->BO0, i->BO1, i->BO2, i->BO3, ppu, i->BI))
        ppu->setNIP(ppu->getLR() & ~3);
    if (i->LK.u())
        ppu->setLR(cia + 4);
}

// Branch Conditional to Count Register, p25

PRINT(BCCTR, XLForm_2) {
    auto mnemonic = i->LK.u() ? "bcctrl" : "bcctr";
    *result = format_3d(mnemonic, i->BO, i->BI, i->BH);
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
    *result = format_3d("crand", i->BT, i->BA, i->BB);
}

EMU(CRAND, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) & bit_test(cr, i->BB));
    ppu->setCR(cr);
}

// Condition Register OR, p28

PRINT(CROR, XLForm_1) {
    *result = format_3d("cror", i->BT, i->BA, i->BB);
}

EMU(CROR, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) | bit_test(cr, i->BB));
    ppu->setCR(cr);
}

// Condition Register XOR, p28

PRINT(CRXOR, XLForm_1) {
    *result = format_3d("crxor", i->BT, i->BA, i->BB);
}

EMU(CRXOR, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) ^ bit_test(cr, i->BB));
    ppu->setCR(cr);
}

// Condition Register NAND, p28

PRINT(CRNAND, XLForm_1) {
    *result = format_3d("crnand", i->BT, i->BA, i->BB);
}

EMU(CRNAND, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, !(bit_test(cr, i->BA) & bit_test(cr, i->BB)));
    ppu->setCR(cr);
}

// Condition Register NOR, p29

PRINT(CRNOR, XLForm_1) {
    *result = format_3d("crnor", i->BT, i->BA, i->BB);
}

EMU(CRNOR, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, (!bit_test(cr, i->BA)) | bit_test(cr, i->BB));
    ppu->setCR(cr);
}

// Condition Register Equivalent, p29

PRINT(CREQV, XLForm_1) {
    *result = format_3d("creqv", i->BT, i->BA, i->BB);
}

EMU(CREQV, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) == bit_test(cr, i->BB));
    ppu->setCR(cr);
}

// Condition Register AND with Complement, p29

PRINT(CRANDC, XLForm_1) {
    *result = format_3d("crandc", i->BT, i->BA, i->BB);
}

EMU(CRANDC, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) & (!bit_test(cr, i->BB)));
    ppu->setCR(cr);
}

// Condition Register OR with Complement, p29

PRINT(CRORC, XLForm_1) {
    *result = format_3d("crorc", i->BT, i->BA, i->BB);
}

EMU(CRORC, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, i->BA) | (!bit_test(cr, i->BB)));
    ppu->setCR(cr);
}

// Condition Register Field Instruction, p30

PRINT(MCRF, XLForm_3) {
    *result = format_2d("mcrf", i->BF, i->BFA);
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
    *result = format_3d_3bracket("lbz", i->RT, i->D, i->RA);
}

EMU(LBZ, DForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + i->D;
    ppu->setGPR(i->RT, ppu->load<1>(ea));
}

// Load Byte and Zero, p34

PRINT(LBZX, XForm_1) {
    *result = format_3d("lbzx", i->RT, i->RA, i->RB);
}

EMU(LBZX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RA);
    ppu->setGPR(i->RT, ppu->load<1>(ea));
}

// Load Byte and Zero with Update, p34

PRINT(LBZU, DForm_1) {
    *result = format_3d_3bracket("lbzu", i->RT, i->D, i->RA);
}

EMU(LBZU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + i->D;
    ppu->setGPR(i->RT, ppu->load<1>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Byte and Zero with Update Indexed, p34

PRINT(LBZUX, XForm_1) {
    *result = format_3d("lbzux", i->RT, i->RA, i->RB);
}

EMU(LBZUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<1>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Halfword and Zero, p35

PRINT(LHZ, DForm_1) {
    *result = format_3d_3bracket("lhz", i->RT, i->D, i->RA);
}

EMU(LHZ, DForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + i->D;
    ppu->setGPR(i->RT, ppu->load<2>(ea));
}

// Load Halfword and Zero Indexed, p35

PRINT(LHZX, XForm_1) {
    *result = format_3d("lhzx", i->RT, i->RA, i->RB);
}

EMU(LHZX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<2>(ea));
}

// Load Halfword and Zero with Update, p35

PRINT(LHZU, DForm_1) {
    *result = format_3d_3bracket("lhzu", i->RT, i->D, i->RA);
}

EMU(LHZU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + i->D;
    ppu->setGPR(i->RT, ppu->load<2>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Halfword and Zero with Update Indexed, p35

PRINT(LHZUX, XForm_1) {
    *result = format_3d("lhzux", i->RT, i->RA, i->RB);
}

EMU(LHZUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<2>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Halfword Algebraic, p36

PRINT(LHA, DForm_1) {
    *result = format_3d_3bracket("lha", i->RT, i->D, i->RA);
}

EMU(LHA, DForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + i->D;
    ppu->setGPR(i->RT, ppu->loads<2>(ea));
}

// Load Halfword Algebraic Indexed, p36

PRINT(LHAX, XForm_1) {
    *result = format_3d("LHAX", i->RT, i->RA, i->RB);
}

EMU(LHAX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->loads<2>(ea));
}

// Load Halfword Algebraic with Update, p36

PRINT(LHAU, DForm_1) {
    *result = format_3d_3bracket("lhaux", i->RT, i->D, i->RA);
}

EMU(LHAU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + i->D;
    ppu->setGPR(i->RT, ppu->loads<2>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Halfword Algebraic with Update Indexed, p36

PRINT(LHAUX, XForm_1) {
    *result = format_3d("lhaux", i->RT, i->RA, i->RB);
}

EMU(LHAUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->loads<2>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Word and Zero, p37

PRINT(LWZ, DForm_1) {
    *result = format_3d_3bracket("lwz", i->RT, i->D, i->RA);
}

EMU(LWZ, DForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + i->D;
    ppu->setGPR(i->RT, ppu->load<4>(ea));
}

// Load Word and Zero Indexed, p37

PRINT(LWZX, XForm_1) {
    *result = format_3d("lwzx", i->RT, i->RA, i->RB);
}

EMU(LWZX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<4>(ea));
}

// Load Word and Zero with Update, p37

PRINT(LWZU, DForm_1) {
    *result = format_3d_3bracket("lwzu", i->RT, i->D, i->RA);
}

EMU(LWZU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + i->D;
    ppu->setGPR(i->RT, ppu->load<4>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Word and Zero with Update Indexed, p37

PRINT(LWZUX, XForm_1) {
    *result = format_3d("lwzux", i->RT, i->RA, i->RB);
}

EMU(LWZUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<4>(ea));
}

// Load Word Algebraic, p38

PRINT(LWA, DSForm_1) {
    *result = format_3d_3bracket("lwa", i->RT, i->DS << 2, i->RA);
}

EMU(LWA, DSForm_1) {
    auto b = i->RA.u() == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + (i->DS << 2);
    ppu->setGPR(i->RT, ppu->loads<4>(ea));
}

// Load Word Algebraic Indexed, p38

PRINT(LWAX, XForm_1) {
    *result = format_3d("lwax", i->RT, i->RA, i->RB);
}

EMU(LWAX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->loads<4>(ea));
}

// Load Word Algebraic with Update Indexed, p38

PRINT(LWAUX, XForm_1) {
    *result = format_3d("lwaux", i->RT, i->RA, i->RB);
}

EMU(LWAUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->loads<4>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Doubleword, p39

PRINT(LD, DSForm_1) {
    *result = format_3d_3bracket("ld", i->RT, i->DS << 2, i->RA);
}

EMU(LD, DSForm_1) {
    auto b = i->RA.u() == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + (i->DS << 2);
    ppu->setGPR(i->RT, ppu->load<8>(ea));
}

// Load Doubleword Indexed, p39

PRINT(LDX, XForm_1) {
    *result = format_3d("ldx", i->RT, i->RA, i->RB);
}

EMU(LDX, XForm_1) {
    auto b = getB(i->RA, ppu);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<8>(ea));
}

// Load Doubleword with Update, p39

PRINT(LDU, DSForm_1) {
    *result = format_3d_3bracket("ldu", i->RT, i->DS << 2, i->RA);
}

EMU(LDU, DSForm_1) {
    auto ea = ppu->getGPR(i->RA) + (i->DS << 2);
    ppu->setGPR(i->RT, ppu->load<8>(ea));
    ppu->setGPR(i->RA, ea);
}

// Load Doubleword with Update Indexed, p39

PRINT(LDUX, XForm_1) {
    *result = format_3d("ldux", i->RT, i->RA, i->RB);
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
    *result = format_3d_3bracket(mnemonic, i->RS.u(), i->D, i->RA.u());
}

inline void PrintStoreIndexed(const char* mnemonic, XForm_8* i, std::string* result) {
    *result = format_3d(mnemonic, i->RS, i->RA, i->RB);
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
    *result = format_3d_3bracket("std", i->RS, i->DS << 2, i->RA);
}

EMU(STD, DSForm_2) { 
    auto b = getB(i->RA, ppu);
    auto ea = b + (i->DS << 2);
    ppu->store<8>(ea, ppu->getGPR(i->RS));
}

PRINT(STDX, XForm_8) { PrintStoreIndexed("stdx", i, result); }
EMU(STDX, XForm_8) { EmuStoreIndexed<8>(i, ppu); }

PRINT(STDU, DSForm_2) { 
    *result = format_3d_3bracket("stdu", i->RS, i->DS << 2, i->RA);
}

EMU(STDU, DSForm_2) { 
    auto ea = ppu->getGPR(i->RA) + (i->DS << 2);
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

PRINT(LHBRX, XForm_1) { *result = format_3d("lhbrx", i->RT, i->RA, i->RB); }
EMU(LHBRX, XForm_1) { EmuLoadByteReverseIndexed<2>(i, ppu); }
PRINT(LWBRX, XForm_1) { *result = format_3d("lwbrx", i->RT, i->RA, i->RB); }
EMU(LWBRX, XForm_1) { EmuLoadByteReverseIndexed<4>(i, ppu); }

template <int Bytes>
void EmuStoreByteReverseIndexed(XForm_8* i, PPU* ppu) {
    auto b = getB(i->RA, ppu);
    auto ea = (b << 1) + ppu->getGPR(i->RB);
    ppu->store<Bytes>(ea, endian_reverse(i->RS.u()));
}

PRINT(STHBRX, XForm_8) { *result = format_3d("sthbrx", i->RS, i->RA, i->RB); }
EMU(STHBRX, XForm_8) { EmuStoreByteReverseIndexed<2>(i, ppu); }
PRINT(STWBRX, XForm_8) { *result = format_3d("stwbrx", i->RS, i->RA, i->RB); }
EMU(STWBRX, XForm_8) { EmuStoreByteReverseIndexed<4>(i, ppu); }

// Fixed-Point Arithmetic Instructions, p51

PRINT(ADDI, DForm_2) {
    *result = format_3d("addi", i->RT, i->RA, i->SI);
}

EMU(ADDI, DForm_2) {
    auto b = getB(i->RA, ppu);
    ppu->setGPR(i->RT, i->SI + b);
}

PRINT(ADDIS, DForm_2) {
    *result = format_3d("addis", i->RT, i->RA, i->SI);
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
    *result = format_3d(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
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
    *result = format_3d(mnemonics[i->OE.u()][i->Rc.u()], i->RT, i->RA, i->RB);
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
    *result = format_3d("andi.", i->RA, i->RS, i->UI);
}

EMU(ANDID, DForm_4) {
    auto res = ppu->getGPR(i->RS) & i->UI.u();
    update_CR0(res, ppu);
    ppu->setGPR(i->RA, res);
}

PRINT(ANDISD, DForm_4) {
    *result = format_3d("andis.", i->RA, i->RS, i->UI);
}

EMU(ANDISD, DForm_4) {
    auto res = ppu->getGPR(i->RS) & (i->UI << 16);
    update_CR0(res, ppu);
    ppu->setGPR(i->RA, res);
}

// OR

PRINT(ORI, DForm_4) {
    *result = format_3d("ori", i->RA, i->RS, i->UI);
}

EMU(ORI, DForm_4) {
    auto res = ppu->getGPR(i->RS) | i->UI.u();
    ppu->setGPR(i->RA, res);
}

PRINT(ORIS, DForm_4) {
    *result = format_3d("oris", i->RA, i->RS, i->UI);
}

EMU(ORIS, DForm_4) {
    auto res = ppu->getGPR(i->RS) | (i->UI << 16);
    ppu->setGPR(i->RA, res);
}

// XOR

PRINT(XORI, DForm_4) {
    *result = format_3d("xori", i->RA, i->RS, i->UI);
}

EMU(XORI, DForm_4) {
    auto res = ppu->getGPR(i->RS) ^ i->UI.u();
    ppu->setGPR(i->RA, res);
}

PRINT(XORIS, DForm_4) {
    *result = format_3d("xoris", i->RA, i->RS, i->UI);
}

EMU(XORIS, DForm_4) {
    auto res = ppu->getGPR(i->RS) ^ (i->UI << 16);
    ppu->setGPR(i->RA, res);
}

// X-forms

PRINT(AND, XForm_6) {
    *result = format_3d(i->Rc.u() ? "and." : "and", i->RA, i->RS, i->RB);
}

EMU(AND, XForm_6) {
    auto res = ppu->getGPR(i->RS) & ppu->getGPR(i->RB);
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(OR, XForm_6) {
    *result = format_3d(i->Rc.u() ? "or." : "or", i->RA, i->RS, i->RB);
}

EMU(OR, XForm_6) {
    auto res = ppu->getGPR(i->RS) | ppu->getGPR(i->RB);
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(XOR, XForm_6) {
    *result = format_3d(i->Rc.u() ? "xor." : "xor", i->RA, i->RS, i->RB);
}

EMU(XOR, XForm_6) {
    auto res = ppu->getGPR(i->RS) ^ ppu->getGPR(i->RB);
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(NAND, XForm_6) {
    *result = format_3d(i->Rc.u() ? "nand." : "nand", i->RA, i->RS, i->RB);
}

EMU(NAND, XForm_6) {
    auto res = ~(ppu->getGPR(i->RS) & ppu->getGPR(i->RB));
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(NOR, XForm_6) {
    *result = format_3d(i->Rc.u() ? "nor." : "nor", i->RA, i->RS, i->RB);
}

EMU(NOR, XForm_6) {
    auto res = ~(ppu->getGPR(i->RS) | ppu->getGPR(i->RB));
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(EQV, XForm_6) {
    *result = format_3d(i->Rc.u() ? "eqv." : "eqv", i->RA, i->RS, i->RB);
}

EMU(EQV, XForm_6) {
    auto res = ~(ppu->getGPR(i->RS) ^ ppu->getGPR(i->RB));
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(ANDC, XForm_6) {
    *result = format_3d(i->Rc.u() ? "andc." : "andc", i->RA, i->RS, i->RB);
}

EMU(ANDC, XForm_6) {
    auto res = ppu->getGPR(i->RS) & ~ppu->getGPR(i->RB);
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(ORC, XForm_6) {
    *result = format_3d(i->Rc.u() ? "orc." : "orc", i->RA, i->RS, i->RB);
}

EMU(ORC, XForm_6) {
    auto res = ppu->getGPR(i->RS) | ~ppu->getGPR(i->RB);
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

// Extend Sign

PRINT(EXTSB, XForm_11) {
    *result = format_2d(i->Rc.u() ? "extsb." : "extsb", i->RA, i->RS);
}

EMU(EXTSB, XForm_11) {
    ppu->setGPR(i->RA, (int64_t)static_cast<int8_t>(ppu->getGPR(i->RS)));
}

PRINT(EXTSH, XForm_11) {
    *result = format_2d(i->Rc.u() ? "extsh." : "extsh", i->RA, i->RS);
}

EMU(EXTSH, XForm_11) {
    ppu->setGPR(i->RA, (int64_t)static_cast<int16_t>(ppu->getGPR(i->RS)));
}

PRINT(EXTSW, XForm_11) {
    *result = format_2d(i->Rc.u() ? "extsw." : "extsw", i->RA, i->RS);
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
        *result = format_1d(mnemonic, i->RS);
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
        ppu->setCR(rs);
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
        *result = format_1d(mnemonic, i->RS);
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

PRINT(RLDICL, MDForm_1) {
    auto n = getNBE(i->sh04, i->sh5);
    auto b = getNBE(i->mb04, i->mb5);
    auto x = 64 - b;
    auto y = n - 64 + b;
    if (n == 0) {
        *result = format_3d(i->Rc.u() ? "clrldi." : "clrldi", i->RA, i->RS, b);
    } else if (64 - n == b) {
        *result = format_3d(i->Rc.u() ? "srdi." : "srdi", i->RA, i->RS, b);
    } else if (x + y < 64) {
        *result = format_4d(i->Rc.u() ? "extrdi." : "extrdi", i->RA, i->RS, x, y);
    } else {
        *result = format_4d(i->Rc.u() ? "rldicl." : "rldicl", i->RA, i->RS, b + n, 64 - n);
    }
}

inline uint64_t mask(uint8_t x, uint8_t y) {
    if (x > y)
        return ~mask(y + 1, x - 1);
    return (~0ull << x) >> (64 - (y - x));
}

inline uint64_t ror(uint64_t n, uint8_t s) {
    asm("rol %1,%0" : "+r" (n) : "c" (s));
    return n;
}

inline uint64_t rol(uint64_t n, uint8_t s) {
    asm("rol %1,%0" : "+r" (n) : "c" (s));
    return n;
}

EMU(RLDICL, MDForm_1) {
    auto n = getNBE(i->sh04, i->sh5);
    auto b = getNBE(i->mb04, i->mb5);
    auto r = rol(ppu->getGPR(i->RS), n);
    auto m = mask(b, 63);
    auto res = r & m;
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
}

PRINT(RLDICR, MDForm_2) {
    auto n = getNBE(i->sh04, i->sh5);
    auto b = getNBE(i->me04, i->me5);
    if (n == 0) {
        *result = format_3d(i->Rc.u() ? "clrrdi." : "clrrdi", i->RA, i->RS, 63 - b);
    } else if (63 - b == n) {
        *result = format_3d(i->Rc.u() ? "sldi." : "sldi", i->RA, i->RS, n);
    } else if (b > 1) {
        *result = format_4d(i->Rc.u() ? "extldi." : "extldi", i->RA, i->RS, b - 1, n);
    } else {
        *result = format_4d(i->Rc.u() ? "rldicr." : "rldicr", i->RA, i->RS, b + n, 64 - n);
    }
}

EMU(RLDICR, MDForm_2) {
    auto n = getNBE(i->sh04, i->sh5);
    auto e = getNBE(i->me04, i->me5);
    auto r = rol(ppu->getGPR(i->RS), n);
    auto m = mask(0, e);
    auto res = r & m;
    ppu->setGPR(i->RA, res);
    if (i->Rc.u())
        update_CR0(res, ppu);
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
        case 14: invoke(ADDI);
        case 15: invoke(ADDIS);
        case 16: invoke(BC);
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
        case 24: invoke(ORI);
        case 25: invoke(ORIS);
        case 26: invoke(XORI);
        case 27: invoke(XORIS);
        case 28: invoke(ANDID);
        case 29: invoke(ANDISD);
        case 34: invoke(LBZ);
        case 30: {
            auto mdform = reinterpret_cast<MDForm_1*>(&x);
            switch (mdform->XO.u()) {
                case 0: invoke(RLDICL);
                case 1: invoke(RLDICR);
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

#undef PRINT
#undef EMU
#undef invoke