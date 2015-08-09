#include "../ps3emu/PPU.h"
#include "ppu_dasm.h"

#include <boost/type_traits.hpp>
#include <boost/format.hpp>
#include <boost/endian/conversion.hpp>
#include <string>
#include <stdint.h>
#include <bitset>

using namespace boost::endian;
using namespace boost;

struct IForm {
    uint32_t LK : 1;
    uint32_t AA : 1;
    uint32_t LI : 24;
    uint32_t OPCD : 6;
};

static_assert(sizeof(IForm) == 4, "");

struct BForm {
    static constexpr int BO_bits = 6;
    uint32_t LK : 6;
    uint32_t AA : 6;
    uint32_t BD : 6;
    uint32_t BI : 6;
    uint32_t BO : BO_bits;
    uint32_t OPCD : 6;
};

// struct DForm_1 {
//     uint32_t OPCD : 6;
//     uint32_t RT : 5;
//     uint32_t RA : 5;
//     uint32_t D : 16;
// };

struct XLForm_1 {
    uint32_t : 1;
    uint32_t XO : 10;
    uint32_t BB : 5;
    uint32_t BA : 5;
    uint32_t BT : 5;
    uint32_t OPCD : 6;
};

struct XLForm_2 {
    static constexpr int BO_bits = 5;
    uint32_t LK : 1;
    uint32_t XO : 10;
    uint32_t BH : 2;
    uint32_t : 3;
    uint32_t BI : 5;
    uint32_t BO : BO_bits;
    uint32_t OPCD : 6;
};

struct XLForm_3 {
    uint32_t : 1;
    uint32_t XO : 10;
    uint32_t : 5;
    uint32_t : 2;
    uint32_t BFA : 3;
    uint32_t : 2;
    uint32_t BF : 3;
    uint32_t OPCD : 6;
};

struct XLForm_4 {
    uint32_t : 1;
    uint32_t XO : 10;
    uint32_t : 5;
    uint32_t : 5;
    uint32_t : 5;
    uint32_t OPCD : 6;
};

#define PRINT(name, form) void print##name(form* instr, uint64_t cia, std::string* result)
#define EMU(name, form) void emulate##name(form* instr, uint64_t cia, PPU* ppu)

inline uint64_t bit_test(uint64_t number, int width, int n) {
    return (number & (1 << (width - n))) >> (width - n);
}

template <typename T>
T bit_test(T number, int n) {
    return bit_test(number, sizeof(T) * 8, n);
}

template <typename T>
inline uint64_t bit_set(T number, int n) {
    return number | (1 << (sizeof(T) * 8 - n));
}

// Branch I-form, p24

inline uint64_t getNIA(IForm* instr, uint64_t cia) {
    auto ext = instr->LI << 2;
    return instr->AA ? ext : (cia + ext);
}

PRINT(B, IForm) {
    const char* mnemonics[][2] = {
        { "b", "ba" }, { "bl", "bla" }  
    };
    auto mnemonic = mnemonics[instr->LK][instr->AA];
    *result = str(format("%s %x") % mnemonic % getNIA(instr, cia));
}

EMU(B, IForm) {
    ppu->setNIP(getNIA(instr, cia));
    if (instr->LK)
        ppu->setLR(cia + 4);
}

// Branch Conditional B-form, p24

inline uint64_t getNIA(BForm* instr, uint64_t cia) {
    auto ext = instr->BD << 2;
    return instr->AA ? ext : (ext + cia);
}

PRINT(BC, BForm) {
    const char* mnemonics[][2] = {
        { "bc", "bca" }, { "bcl", "bcla" }  
    };
    auto mnemonic = mnemonics[instr->LK][instr->AA];
    *result = str(format("%s %d,%d,%x") 
        % mnemonic % (uint64_t)instr->BO % (uint64_t)instr->BI % getNIA(instr, cia));
}

inline bool is_taken(bool bo0, bool bo1, bool bo2, bool bo3, PPU* ppu, int bi) {
    auto ctr_ok = bo2 || ((ppu->getCTR() != 0) ^ bo3);
    auto cond_ok = bo0 || bit_test(ppu->getCR(), 32, bi) == bo1;
    return ctr_ok && cond_ok;
}

EMU(BC, BForm) {
    auto bo0 = bit_test(instr->BO, BForm::BO_bits, 0);
    auto bo1 = bit_test(instr->BO, BForm::BO_bits, 1);
    auto bo2 = bit_test(instr->BO, BForm::BO_bits, 2);
    auto bo3 = bit_test(instr->BO, BForm::BO_bits, 3);
    if (!bo2)
        ppu->setCTR(ppu->getCTR() - 1);
    if (is_taken(bo0, bo1, bo2, bo3, ppu, instr->BI))
        ppu->setNIP(getNIA(instr, cia));
    if (instr->LK)
        ppu->setLR(cia + 4);
}

// Branch Conditional to Link Register XL-form, p25

std::string format_3d(const char* mnemonic, uint64_t op1, uint64_t op2, uint64_t op3) {
    return str(format("%s %d,%d,%d") % mnemonic % op1 % op2 % op3);
}

std::string format_2d(const char* mnemonic, uint64_t op1, uint64_t op2) {
    return str(format("%s %d,%d") % mnemonic % op1 % op2);
}

PRINT(BCLR, XLForm_2) {
    auto mnemonic = instr->LK ? "bclrl" : "bclr";
    *result = format_3d(mnemonic, instr->BO, instr->BI, instr->BH);
}

EMU(BCLR, XLForm_2) {
    auto bo0 = bit_test(instr->BO, XLForm_2::BO_bits, 0);
    auto bo1 = bit_test(instr->BO, XLForm_2::BO_bits, 1);
    auto bo2 = bit_test(instr->BO, XLForm_2::BO_bits, 2);
    auto bo3 = bit_test(instr->BO, XLForm_2::BO_bits, 3);
    if (!bo2)
        ppu->setCTR(ppu->getCTR() - 1);
    if (is_taken(bo0, bo1, bo2, bo3, ppu, instr->BI))
        ppu->setNIP(ppu->getLR() & ~3);
    if (instr->LK)
        ppu->setLR(cia + 4);
}

// Branch Conditional to Count Register XL-form, p25

PRINT(BCCTR, XLForm_2) {
    auto mnemonic = instr->LK ? "bcctrl" : "bcctr";
    *result = format_3d(mnemonic, instr->BO, instr->BI, instr->BH);
}

EMU(BCCTR, XLForm_2) {
    auto bo0 = bit_test(instr->BO, XLForm_2::BO_bits, 0);
    auto bo1 = bit_test(instr->BO, XLForm_2::BO_bits, 1);
    auto cond_ok = bo0 || bit_test(ppu->getCR(), 32, instr->BI) == bo1;
    if (cond_ok)
        ppu->setNIP(ppu->getCTR() & ~3);
    if (instr->LK)
        ppu->setLR(cia + 4);
}

// Condition Register AND XL-form, p28

PRINT(CRAND, XLForm_1) {
    *result = format_3d("crand", instr->BT, instr->BA, instr->BB);
}

EMU(CRAND, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, instr->BA) & bit_test(cr, instr->BB));
    ppu->setCR(cr);
}

// Condition Register OR XL-form, p28

PRINT(CROR, XLForm_1) {
    *result = format_3d("cror", instr->BT, instr->BA, instr->BB);
}

EMU(CROR, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, instr->BA) | bit_test(cr, instr->BB));
    ppu->setCR(cr);
}

// Condition Register XOR XL-form, p28

PRINT(CRXOR, XLForm_1) {
    *result = format_3d("crxor", instr->BT, instr->BA, instr->BB);
}

EMU(CRXOR, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, instr->BA) ^ bit_test(cr, instr->BB));
    ppu->setCR(cr);
}

// Condition Register NAND XL-form, p28

PRINT(CRNAND, XLForm_1) {
    *result = format_3d("crnand", instr->BT, instr->BA, instr->BB);
}

EMU(CRNAND, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, !(bit_test(cr, instr->BA) & bit_test(cr, instr->BB)));
    ppu->setCR(cr);
}

// Condition Register NOR XL-form, p29

PRINT(CRNOR, XLForm_1) {
    *result = format_3d("crnor", instr->BT, instr->BA, instr->BB);
}

EMU(CRNOR, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, (!bit_test(cr, instr->BA)) | bit_test(cr, instr->BB));
    ppu->setCR(cr);
}

// Condition Register Equivalent XL-form, p29

PRINT(CREQV, XLForm_1) {
    *result = format_3d("creqv", instr->BT, instr->BA, instr->BB);
}

EMU(CREQV, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, instr->BA) == bit_test(cr, instr->BB));
    ppu->setCR(cr);
}

// Condition Register AND with Complement XL-form, p29

PRINT(CRANDC, XLForm_1) {
    *result = format_3d("crandc", instr->BT, instr->BA, instr->BB);
}

EMU(CRANDC, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, instr->BA) & (!bit_test(cr, instr->BB)));
    ppu->setCR(cr);
}

// Condition Register OR with Complement XL-form, p29

PRINT(CRORC, XLForm_1) {
    *result = format_3d("crorc", instr->BT, instr->BA, instr->BB);
}

EMU(CRORC, XLForm_1) {
    auto cr = ppu->getCR();
    bit_set(cr, bit_test(cr, instr->BA) | (!bit_test(cr, instr->BB)));
    ppu->setCR(cr);
}

// Condition Register Field Instruction XL-form, p30

PRINT(MCRF, XLForm_3) {
    *result = format_2d("mcrf", instr->BF, instr->BFA);
}

EMU(MCRF, XLForm_3) {
    auto cr = ppu->getCR();
    std::bitset<64> bs(cr);
    std::bitset<64> new_bs(cr);
    for (int i = 0; i <= 3; ++i) {
        new_bs[i*instr->BF] = bs[i*instr->BFA];
    }
    ppu->setCR(new_bs.to_ullong());
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