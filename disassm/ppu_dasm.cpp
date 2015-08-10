#include "ppu_dasm.h"
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

#define PRINT(name, form) void print##name(form* i, uint64_t cia, std::string* result)
#define EMU(name, form) void emulate##name(form* i, uint64_t cia, PPU* ppu)

inline uint64_t bit_test(uint64_t number, int width, int n) {
    return (number & (1 << (width - n))) >> (width - n);
}

template <typename T>
T bit_test(T number, int n) {
    return bit_test(number, sizeof(T) * 8, n);
}

template <typename T>
uint64_t bit_set(T number, int n) {
    return number | (1 << (sizeof(T) * 8 - n));
}

template <int Bits>
struct BitsToSignedType { };
template <>
struct BitsToSignedType<8> { typedef int8_t value; };
template <>
struct BitsToSignedType<16> { typedef int16_t value; };
template <>
struct BitsToSignedType<32> { typedef int32_t value; };

template <int Bits>
int64_t exts(uint64_t number) {
    return (typename BitsToSignedType<Bits>::value)number;
}

template <>
int64_t exts<24>(uint64_t number) {
    auto extendedThirdByte = (int64_t)(char)((number >> 16) | 0xff);
    return number & (extendedThirdByte << 16);
}

// Branch I-form, p24

inline uint64_t getNIA(IForm* i, uint64_t cia) {
    auto ext = exts<IForm::LI_bits>(i->LI) << 2;
    return i->AA ? ext : (cia + ext);
}

PRINT(B, IForm) {
    const char* mnemonics[][2] = {
        { "b", "ba" }, { "bl", "bla" }  
    };
    auto mnemonic = mnemonics[i->LK][i->AA];
    *result = str(format("%s %x") % mnemonic % getNIA(i, cia));
}

EMU(B, IForm) {
    ppu->setNIP(getNIA(i, cia));
    if (i->LK)
        ppu->setLR(cia + 4);
}

// Branch Conditional B-form, p24

inline uint64_t getNIA(BForm* i, uint64_t cia) {
    auto ext = exts<BForm::BD_bits + 2>(i->BD << 2);
    return i->AA ? ext : (ext + cia);
}

PRINT(BC, BForm) {
    const char* mnemonics[][2] = {
        { "bc", "bca" }, { "bcl", "bcla" }  
    };
    auto mnemonic = mnemonics[i->LK][i->AA];
    *result = str(format("%s %d,%d,%x") 
        % mnemonic % (uint64_t)i->BO % (uint64_t)i->BI % getNIA(i, cia));
}

inline bool is_taken(bool bo0, bool bo1, bool bo2, bool bo3, PPU* ppu, int bi) {
    auto ctr_ok = bo2 || ((ppu->getCTR() != 0) ^ bo3);
    auto cond_ok = bo0 || bit_test(ppu->getCR(), 32, bi) == bo1;
    return ctr_ok && cond_ok;
}

EMU(BC, BForm) {
    auto bo0 = bit_test(i->BO, BForm::BO_bits, 0);
    auto bo1 = bit_test(i->BO, BForm::BO_bits, 1);
    auto bo2 = bit_test(i->BO, BForm::BO_bits, 2);
    auto bo3 = bit_test(i->BO, BForm::BO_bits, 3);
    if (!bo2)
        ppu->setCTR(ppu->getCTR() - 1);
    if (is_taken(bo0, bo1, bo2, bo3, ppu, i->BI))
        ppu->setNIP(getNIA(i, cia));
    if (i->LK)
        ppu->setLR(cia + 4);
}

// Branch Conditional to Link Register XL-form, p25

std::string format_3d(const char* mnemonic, uint64_t op1, uint64_t op2, uint64_t op3) {
    return str(format("%s %d,%d,%d") % mnemonic % op1 % op2 % op3);
}

std::string format_2d(const char* mnemonic, uint64_t op1, uint64_t op2) {
    return str(format("%s %d,%d") % mnemonic % op1 % op2);
}

std::string format_3d_3bracket(const char* mnemonic, uint64_t op1, uint64_t op2, uint64_t op3) {
    return str(format("%s %d,%d(%d)") % mnemonic % op1 % op2 % op3);
}

PRINT(BCLR, XLForm_2) {
    auto mnemonic = i->LK ? "bclrl" : "bclr";
    *result = format_3d(mnemonic, i->BO, i->BI, i->BH);
}

EMU(BCLR, XLForm_2) {
    auto bo0 = bit_test(i->BO, XLForm_2::BO_bits, 0);
    auto bo1 = bit_test(i->BO, XLForm_2::BO_bits, 1);
    auto bo2 = bit_test(i->BO, XLForm_2::BO_bits, 2);
    auto bo3 = bit_test(i->BO, XLForm_2::BO_bits, 3);
    if (!bo2)
        ppu->setCTR(ppu->getCTR() - 1);
    if (is_taken(bo0, bo1, bo2, bo3, ppu, i->BI))
        ppu->setNIP(ppu->getLR() & ~3);
    if (i->LK)
        ppu->setLR(cia + 4);
}

// Branch Conditional to Count Register, p25

PRINT(BCCTR, XLForm_2) {
    auto mnemonic = i->LK ? "bcctrl" : "bcctr";
    *result = format_3d(mnemonic, i->BO, i->BI, i->BH);
}

EMU(BCCTR, XLForm_2) {
    auto bo0 = bit_test(i->BO, XLForm_2::BO_bits, 0);
    auto bo1 = bit_test(i->BO, XLForm_2::BO_bits, 1);
    auto cond_ok = bo0 || bit_test(ppu->getCR(), 32, i->BI) == bo1;
    if (cond_ok)
        ppu->setNIP(ppu->getCTR() & ~3);
    if (i->LK)
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
        new_bs[j*i->BF] = bs[j*i->BFA];
    }
    ppu->setCR(new_bs.to_ullong());
}

// Load Byte and Zero, p34

PRINT(LBZ, DForm_1) {
    *result = format_3d_3bracket("lbz", i->RT, i->D, i->RA);
}

EMU(LBZ, DForm_1) {
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + exts<DForm_1::D_bits>(i->D);
    ppu->setGPR(i->RT, ppu->load<1>(ea));
}

// Load Byte and Zero, p34

PRINT(LBZX, XForm_1) {
    *result = format_3d("lbzx", i->RT, i->RA, i->RB);
}

EMU(LBZX, XForm_1) {
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + ppu->getGPR(i->RA);
    ppu->setGPR(i->RT, ppu->load<1>(ea));
}

// Load Byte and Zero with Update, p34

PRINT(LBZU, DForm_1) {
    *result = format_3d_3bracket("lbzu", i->RT, i->D, i->RA);
}

EMU(LBZU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + exts<DForm_1::D_bits>(i->D);
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
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + exts<DForm_1::D_bits>(i->D);
    ppu->setGPR(i->RT, ppu->load<2>(ea));
}

// Load Halfword and Zero Indexed, p35

PRINT(LHZX, XForm_1) {
    *result = format_3d("lhzx", i->RT, i->RA, i->RB);
}

EMU(LHZX, XForm_1) {
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<2>(ea));
}

// Load Halfword and Zero with Update, p35

PRINT(LHZU, DForm_1) {
    *result = format_3d_3bracket("lhzu", i->RT, i->D, i->RA);
}

EMU(LHZU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + exts<DForm_1::D_bits>(i->D);
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
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + exts<DForm_1::D_bits>(i->D);
    ppu->setGPR(i->RT, exts<16>(ppu->load<2>(ea)));
}

// Load Halfword Algebraic Indexed, p36

PRINT(LHAX, XForm_1) {
    *result = format_3d("LHAX", i->RT, i->RA, i->RB);
}

EMU(LHAX, XForm_1) {
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, exts<16>(ppu->load<2>(ea)));
}

// Load Halfword Algebraic with Update, p36

PRINT(LHAU, DForm_1) {
    *result = format_3d_3bracket("lhaux", i->RT, i->D, i->RA);
}

EMU(LHAU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + exts<DForm_1::D_bits>(i->D);
    ppu->setGPR(i->RT, exts<16>(ppu->load<2>(ea)));
    ppu->setGPR(i->RA, ea);
}

// Load Halfword Algebraic with Update Indexed, p36

PRINT(LHAUX, XForm_1) {
    *result = format_3d("lhaux", i->RT, i->RA, i->RB);
}

EMU(LHAUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, exts<16>(ppu->load<2>(ea)));
    ppu->setGPR(i->RA, ea);
}

// Load Word and Zero, p37

PRINT(LWZ, DForm_1) {
    *result = format_3d_3bracket("lwz", i->RT, i->D, i->RA);
}

EMU(LWZ, DForm_1) {
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + exts<DForm_1::D_bits>(i->D);
    ppu->setGPR(i->RT, ppu->load<4>(ea));
}

// Load Word and Zero Indexed, p37

PRINT(LWZX, XForm_1) {
    *result = format_3d("lwzx", i->RT, i->RA, i->RB);
}

EMU(LWZX, XForm_1) {
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<4>(ea));
}

// Load Word and Zero with Update, p37

PRINT(LWZU, DForm_1) {
    *result = format_3d_3bracket("lwzu", i->RT, i->D, i->RA);
}

EMU(LWZU, DForm_1) {
    auto ea = ppu->getGPR(i->RA) + exts<DForm_1::D_bits>(i->D);
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
    *result = format_3d_3bracket("lwa", i->RT, i->DS, i->RA);
}

EMU(LWA, DSForm_1) {
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + exts<DSForm_1::DS_bits + 2>(i->DS << 2);
    ppu->setGPR(i->RT, exts<32>(ppu->load<4>(ea)));
}

// Load Word Algebraic Indexed, p38

PRINT(LWAX, XForm_1) {
    *result = format_3d("lwax", i->RT, i->RA, i->RB);
}

EMU(LWAX, XForm_1) {
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, exts<32>(ppu->load<4>(ea)));
}

// Load Word Algebraic with Update Indexed, p38

PRINT(LWAUX, XForm_1) {
    *result = format_3d("lwaux", i->RT, i->RA, i->RB);
}

EMU(LWAUX, XForm_1) {
    auto ea = ppu->getGPR(i->RA) + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, exts<32>(ppu->load<4>(ea)));
    ppu->setGPR(i->RA, ea);
}

// Load Doubleword, p39

PRINT(LD, DSForm_1) {
    *result = format_3d_3bracket("ld", i->RT, i->RA, i->DS);
}

EMU(LD, DSForm_1) {
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + exts<DSForm_1::DS_bits + 2>(i->DS << 2);
    ppu->setGPR(i->RT, ppu->load<8>(ea));
}

// Load Doubleword Indexed, p39

PRINT(LDX, XForm_1) {
    *result = format_3d("ldx", i->RT, i->RA, i->RB);
}

EMU(LDX, XForm_1) {
    auto b = i->RA == 0 ? 0 : ppu->getGPR(i->RA);
    auto ea = b + ppu->getGPR(i->RB);
    ppu->setGPR(i->RT, ppu->load<8>(ea));
}

// Load Doubleword with Update, p39

PRINT(LDU, DSForm_1) {
    *result = format_3d_3bracket("ldu", i->RT, i->RA, i->DS);
}

EMU(LDU, DSForm_1) {
    auto ea = ppu->getGPR(i->RA) + exts<DSForm_1::DS_bits + 2>(i->DS << 2);
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
    switch (iform->OPCD) {
        case 16: invoke(BC);
        case 18: invoke(B);
        case 19: {
            auto xlform = reinterpret_cast<XLForm_1*>(&x);
            switch (xlform->XO) {
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
        case 34: invoke(LBZ);
        case 31: {
            auto xform = reinterpret_cast<XForm_1*>(&x);
            switch (xform->XO) {
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
            }
            break;
        }
        case 35: invoke(LBZU);
        case 32: invoke(LWZ);
        case 33: invoke(LWZU);
        case 40: invoke(LHZ);
        case 41: invoke(LHZU);
        case 42: invoke(LHA);
        case 43: invoke(LHAU);
        case 58: {
            auto dsform = reinterpret_cast<DSForm_1*>(&x);
            switch (dsform->XO) {
                case 2: invoke(LWA);
                case 0: invoke(LD);
                case 1: invoke(LDU);
            }
            break;
        }
        default: throw std::runtime_error("unknown opcode");
    }
}

std::string ftest() {
    //uint8_t instr[] = { 0x48, 0x00, 0x04, 0x49 };
    uint8_t instr[] = { 0x48, 0x00, 0x04, 0x49 };
    std::string str;
    ppu_dasm<DasmMode::Print>(instr, 0x10214, &str);
    return str;
}