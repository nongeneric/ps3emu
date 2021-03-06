#include "ppu_dasm.h"

#include "../Process.h"
#include "../MainMemory.h"
#include "../dasm_utils.h"
#include "ppu_dasm_forms.h"
#include "../utils.h"
#include "ps3emu/state.h"
#include "ps3emu/utils/asm.h"

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
#include <type_traits>

#include <boost/preprocessor/variadic/to_list.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/punctuation.hpp>
#include <boost/preprocessor/logical.hpp>
#include <boost/preprocessor/control.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/list/enum.hpp>
#include <boost/preprocessor/list/rest_n.hpp>

using namespace boost::endian;

#ifdef EMU_REWRITER
    #define RETURN_REWRITER_NCALL return
    #define SET_REWRITER_NCALL g_state.rewriter_ncall = true
    #define SET_NIP_INDIRECT(x) {TH->setNIP(x & ~3ul); return;}
    #define SET_NIP_INITIAL(x) goto _##x
    #define BRANCH_TO_LR(lr) TH->setNIP(lr & ~3ul); LOG_RET(lr); return;
#else
    #define SET_REWRITER_NCALL g_state.rewriter_ncall = false
    #define RETURN_REWRITER_NCALL
    #define SET_NIP_INITIAL(x) TH->setNIP(x)
    #define SET_NIP_INDIRECT(x) SET_NIP(x)
    #define BRANCH_TO_LR(lr) TH->setNIP(lr & ~3ul)
#endif

#ifdef EMU_REWRITER_NOFEXCEPT
#define FECLEAR_EXCEPT
#else
#define FECLEAR_EXCEPT std::feclearexcept(FE_ALL_EXCEPT)
#endif

#define SET_NIP SET_NIP_INITIAL

#define MM thread->mm()
#define TH thread
#define invoke(name) invoke_impl<M>(#name, print##name, emulate##name, rewrite##name, &x, cia, state); break
#define PRINT(name, form) inline void print##name(form* i, uint64_t cia, std::string* result)

#ifdef EMU_REWRITER
#define EMU_REWRITE(...)
#else
#define EMU_REWRITE(...) \
    inline void BOOST_PP_CAT(rewrite, BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)) \
        (BOOST_PP_VARIADIC_ELEM(1, __VA_ARGS__)* i, uint64_t cia, std::string* result) { \
            *result = rewrite_print( \
                BOOST_PP_STRINGIZE(BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)), \
                BOOST_PP_LIST_ENUM( \
                    BOOST_PP_LIST_REST_N(2, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__))) \
            ); \
        } \
    inline void BOOST_PP_CAT(emulate, BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)) \
        (BOOST_PP_VARIADIC_ELEM(1, __VA_ARGS__)* i, uint64_t cia, PPUThread* thread) { \
            BOOST_PP_EXPAND(BOOST_PP_CAT(_, BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)) BOOST_PP_LPAREN() \
                BOOST_PP_LIST_ENUM( \
                    BOOST_PP_LIST_REST_N(2, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__)))) \
            ); \
        }
#endif

namespace {

alignas(16) static const __m128i BYTE_SHIFT_RIGHT_SHUFFLE_CONTROL[17] {
    _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0),
    _mm_set_epi8(-1, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1),
    _mm_set_epi8(-1, -1, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2),
    _mm_set_epi8(-1, -1, -1, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3),
    _mm_set_epi8(-1, -1, -1, -1, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4),
    _mm_set_epi8(-1, -1, -1, -1, -1, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, 15, 14, 13, 12, 11, 10, 9, 8, 7),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 15, 14, 13, 12, 11, 10, 9, 8),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, 15, 14, 13, 12, 11, 10, 9),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, 14, 13, 12, 11, 10),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, 14, 13, 12, 11),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, 14, 13, 12),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, 14, 13),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, 14),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
};

alignas(16) static const __m128i VSPLTW_SHUFFLE_CONTROL[4] {
    _mm_set_epi8(15, 14, 13, 12, 15, 14, 13, 12, 15, 14, 13, 12, 15, 14, 13, 12),
    _mm_set_epi8(11, 10, 9, 8, 11, 10, 9, 8, 11, 10, 9, 8, 11, 10, 9, 8),
    _mm_set_epi8(7, 6, 5, 4, 7, 6, 5, 4, 7, 6, 5, 4, 7, 6, 5, 4),
    _mm_set_epi8(3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0),
};

alignas(16) static const __m128i VSPLTH_SHUFFLE_CONTROL[8] {
    _mm_set_epi8(15, 14, 15, 14, 15, 14, 15, 14, 15, 14, 15, 14, 15, 14, 15, 14),
    _mm_set_epi8(13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12),
    _mm_set_epi8(11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10),
    _mm_set_epi8(9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8),
    _mm_set_epi8(7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6),
    _mm_set_epi8(5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4),
    _mm_set_epi8(3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2),
    _mm_set_epi8(1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0),
};

static const __m128i VMRGHH1_SHUFFLE_CONTROL =
    _mm_set_epi8(15, 14, -1, -1, 13, 12, -1, -1, 11, 10, -1, -1, 9, 8, -1, -1);

static const __m128i VMRGHH2_SHUFFLE_CONTROL =
    _mm_set_epi8(-1, -1, 15, 14, -1, -1, 13, 12, -1, -1, 11, 10, -1, -1, 9, 8);

static const __m128i VMRGLH1_SHUFFLE_CONTROL =
    _mm_set_epi8(7, 6, -1, -1, 5, 4, -1, -1, 3, 2, -1, -1, 1, 0, -1, -1);

static const __m128i VMRGLH2_SHUFFLE_CONTROL =
    _mm_set_epi8(-1, -1, 7, 6, -1, -1, 5, 4, -1, -1, 3, 2, -1, -1, 1, 0);

static const __m128i VMRGHW1_SHUFFLE_CONTROL =
    _mm_set_epi8(15, 14, 13, 12, -1, -1, -1, -1, 11, 10, 9, 8, -1, -1, -1, -1);

static const __m128i VMRGHW2_SHUFFLE_CONTROL =
    _mm_set_epi8(-1, -1, -1, -1, 15, 14, 13, 12, -1, -1, -1, -1, 11, 10, 9, 8);

static const __m128i VMRGLW1_SHUFFLE_CONTROL =
    _mm_set_epi8(7, 6, 5, 4, -1, -1, -1, -1, 3, 2, 1, 0, -1, -1, -1, -1);

static const __m128i VMRGLW2_SHUFFLE_CONTROL =
    _mm_set_epi8(-1, -1, -1, -1, 7, 6, 5, 4, -1, -1, -1, -1, 3, 2, 1, 0);

static const __m128i LVSR_TABLE[] = {
    _mm_set_epi64x(0x1011121314151617ull, 0x18191A1B1C1D1E1Full),
    _mm_set_epi64x(0x0F10111213141516ull, 0x1718191A1B1C1D1Eull),
    _mm_set_epi64x(0x0E0F101112131415ull, 0x161718191A1B1C1Dull),
    _mm_set_epi64x(0x0D0E0F1011121314ull, 0x15161718191A1B1Cull),
    _mm_set_epi64x(0x0C0D0E0F10111213ull, 0x1415161718191A1Bull),
    _mm_set_epi64x(0x0B0C0D0E0F101112ull, 0x131415161718191Aull),
    _mm_set_epi64x(0x0A0B0C0D0E0F1011ull, 0x1213141516171819ull),
    _mm_set_epi64x(0x090A0B0C0D0E0F10ull, 0x1112131415161718ull),
    _mm_set_epi64x(0x08090A0B0C0D0E0Full, 0x1011121314151617ull),
    _mm_set_epi64x(0x0708090A0B0C0D0Eull, 0x0F10111213141516ull),
    _mm_set_epi64x(0x060708090A0B0C0Dull, 0x0E0F101112131415ull),
    _mm_set_epi64x(0x05060708090A0B0Cull, 0x0D0E0F1011121314ull),
    _mm_set_epi64x(0x0405060708090A0Bull, 0x0C0D0E0F10111213ull),
    _mm_set_epi64x(0x030405060708090Aull, 0x0B0C0D0E0F101112ull),
    _mm_set_epi64x(0x0203040506070809ull, 0x0A0B0C0D0E0F1011ull),
    _mm_set_epi64x(0x0102030405060708ull, 0x090A0B0C0D0E0F10ull),
};

static const __m128i LVSL_TABLE[] = {
    _mm_set_epi64x(0x0001020304050607ull, 0x08090A0B0C0D0E0Full),
    _mm_set_epi64x(0x0102030405060708ull, 0x090A0B0C0D0E0F10ull),
    _mm_set_epi64x(0x0203040506070809ull, 0x0A0B0C0D0E0F1011ull),
    _mm_set_epi64x(0x030405060708090Aull, 0x0B0C0D0E0F101112ull),
    _mm_set_epi64x(0x0405060708090A0Bull, 0x0C0D0E0F10111213ull),
    _mm_set_epi64x(0x05060708090A0B0Cull, 0x0D0E0F1011121314ull),
    _mm_set_epi64x(0x060708090A0B0C0Dull, 0x0E0F101112131415ull),
    _mm_set_epi64x(0x0708090A0B0C0D0Eull, 0x0F10111213141516ull),
    _mm_set_epi64x(0x08090A0B0C0D0E0Full, 0x1011121314151617ull),
    _mm_set_epi64x(0x090A0B0C0D0E0F10ull, 0x1112131415161718ull),
    _mm_set_epi64x(0x0A0B0C0D0E0F1011ull, 0x1213141516171819ull),
    _mm_set_epi64x(0x0B0C0D0E0F101112ull, 0x131415161718191Aull),
    _mm_set_epi64x(0x0C0D0E0F10111213ull, 0x1415161718191A1Bull),
    _mm_set_epi64x(0x0D0E0F1011121314ull, 0x15161718191A1B1Cull),
    _mm_set_epi64x(0x0E0F101112131415ull, 0x161718191A1B1C1Dull),
    _mm_set_epi64x(0x0F10111213141516ull, 0x1718191A1B1C1D1Eull),
};

}
// Branch I-form, p24

inline uint64_t getNIA(IForm* i, uint64_t cia) {
    auto ext = i->LI_native();
    return i->AA_u() ? ext : (cia + ext);
}

#define _B(nia, lk, cia) { \
    if (lk) \
        TH->setLR(cia + 4); \
    SET_NIP(nia); \
}

EMU_REWRITE(B, IForm, getNIA(i, cia), i->LK_u(), cia)

PRINT(B, IForm) {
    const char* mnemonics[][2] = {
        { "b", "ba" }, { "bl", "bla" }
    };
    auto mnemonic = mnemonics[i->LK_u()][i->AA_u()];
    *result = format_u(mnemonic, getNIA(i, cia));
}

// Branch Conditional B-form, p24

enum class BranchMnemonicType {
    ExtSimple, ExtCondition, Generic
};

BranchMnemonicType getExtBranchMnemonic(bool lr,
                                        bool abs,
                                        bool tolr,
                                        bool toctr,
                                        BitField<6, 11> btbo,
                                        BitField<11, 16> bi,
                                        std::string& mnemonic);
std::string formatCRbit(BitField<11, 16> bi);

inline uint64_t getNIA(BForm* i, uint64_t cia) {
    auto ext = i->BD() << 2;
    return i->AA_u() ? ext : (ext + cia);
}

PRINT(BC, BForm) {
    std::string extMnemonic;
    auto mtype = getExtBranchMnemonic(
        i->LK_u(), i->AA_u(), false, false, i->BO(), i->BI(), extMnemonic);
    if (mtype == BranchMnemonicType::Generic) {
        const char* mnemonics[][2] = {
            { "bc", "bca" }, { "bcl", "bcla" }
        };
        auto mnemonic = mnemonics[i->LK_u()][i->AA_u()];
        *result = sformat("{} {},{},{:x}",
                            mnemonic, i->BO_u(), formatCRbit(i->BI()).c_str(), getNIA(i, cia));
    } else if (i->BI_u() > 3) {
        if (mtype == BranchMnemonicType::ExtCondition) {
            *result = sformat("{} cr{},{:x}",
                                extMnemonic.c_str(), i->BI_u() / 4, getNIA(i, cia));
        } else if (mtype == BranchMnemonicType::ExtSimple) {
            *result = sformat("{} {},{:x}",
                                extMnemonic.c_str(), formatCRbit(i->BI()).c_str(), getNIA(i, cia));
        }
    } else {
        *result = format_u(extMnemonic.c_str(), getNIA(i, cia));
    }
}

inline bool isTaken(unsigned bo0,
                    unsigned bo1,
                    unsigned bo2,
                    unsigned bo3,
                    PPUThread* thread,
                    unsigned bi)
{
    auto ctr_ok = bo2 | ((TH->getCTR() != 0) ^ bo3);
    auto cond_ok = bo0 | (bit_test(TH->getCR(), 32, bi) == bo1);
    return ctr_ok && cond_ok;
}

#define _BC(bo0, bo1, bo2, bo3, bi, lk, nia, cia) { \
    if (!bo2) \
        TH->setCTR(TH->getCTR() - 1); \
    if (lk) \
        TH->setLR(cia + 4); \
    if (isTaken(bo0, bo1, bo2, bo3, TH, bi)) { \
        SET_NIP(nia); \
    } \
}

EMU_REWRITE(BC, BForm, i->BO0_u(), i->BO1_u(), i->BO2_u(), i->BO3_u(), i->BI_u(), i->LK_u(), getNIA(i, cia), cia)

// Branch Conditional to Link Register XL-form, p25

inline int64_t getB(unsigned ra, PPUThread* thread) {
    return ra == 0 ? 0 : TH->getGPR(ra);
}

PRINT(BCLR, XLForm_2) {
    std::string extMnemonic;
    auto mtype = getExtBranchMnemonic(i->LK_u(), 0, true, false, i->BO(), i->BI(), extMnemonic);
    if (mtype == BranchMnemonicType::Generic) {
        auto mnemonic = i->LK_u() ? "bclrl" : "bclr";
        *result = sformat("{} {},{},{}",
                            mnemonic, i->BO_u(), formatCRbit(i->BI()).c_str(), i->BH_u());
    } else if (i->BI_u() > 3) {
        *result = sformat("{} cr{}", extMnemonic.c_str(), i->BI_u() / 4);
    } else {
        *result = extMnemonic;
    }
}

#define _BCLR(bo0, bo1, bo2, bo3, bi, lk, cia) { \
    if (!bo2) \
        TH->setCTR(TH->getCTR() - 1); \
    auto lr = TH->getLR(); \
    if (lk) \
        TH->setLR(cia + 4); \
    if (isTaken(bo0, bo1, bo2, bo3, TH, bi)) { \
        BRANCH_TO_LR(lr); \
    } \
}

EMU_REWRITE(BCLR, XLForm_2, i->BO0_u(), i->BO1_u(), i->BO2_u(), i->BO3_u(), i->BI_u(), i->LK_u(), cia)

// Branch Conditional to Count Register, p25

PRINT(BCCTR, XLForm_2) {
    std::string extMnemonic;
    auto mtype = getExtBranchMnemonic(i->LK_u(), 0, false, true, i->BO(), i->BI(), extMnemonic);
    if (mtype == BranchMnemonicType::Generic) {
        auto mnemonic = i->LK_u() ? "bcctrl" : "bcctr";
        *result = sformat("{} {},{},{}",
                            mnemonic, i->BO_u(), formatCRbit(i->BI()).c_str(), i->BH_u());
    } else if (i->BI_u() > 3) {
        *result = sformat("{} cr{}", extMnemonic.c_str(), i->BI_u() / 4);
    } else {
        *result = extMnemonic;
    }
}

#define _BCCTR(bo0, bo1, bi, lk, cia) { \
    auto cond_ok = bo0 || bit_test(TH->getCR(), 32, bi) == bo1; \
    if (lk) \
        TH->setLR(cia + 4); \
    if (cond_ok) \
        SET_NIP_INDIRECT(TH->getCTR() & ~3); \
}

EMU_REWRITE(BCCTR, XLForm_2, i->BO0_u(), i->BO1_u(), i->BI_u(), i->LK_u(), cia)

// Condition Register AND, p28

PRINT(CRAND, XLForm_1) {
    *result = format_nnn("crand", i->BT(), i->BA(), i->BB());
}

#define _CRAND(bt, ba, bb) { \
    auto cr = TH->getCR(); \
    cr = bit_set(cr, bt, bit_test(cr, 32, ba) & bit_test(cr, 32, bb)); \
    TH->setCR(cr); \
}

EMU_REWRITE(CRAND, XLForm_1, i->BT_u(), i->BA_u(), i->BB_u())

// Condition Register OR, p28

PRINT(CROR, XLForm_1) {
    *result = format_nnn("cror", i->BT(), i->BA(), i->BB());
}

#define _CROR(_bt, _ba, _bb) { \
    auto cr = TH->getCR(); \
    cr = bit_set(cr, _bt, bit_test(cr, 32, _ba) | bit_test(cr, 32, _bb)); \
    TH->setCR(cr); \
}
EMU_REWRITE(CROR, XLForm_1, i->BT_u(), i->BA_u(), i->BB_u())


// Condition Register XOR, p28

PRINT(CRXOR, XLForm_1) {
    *result = format_nnn("crxor", i->BT(), i->BA(), i->BB());
}

#define _CRXOR(_bt, _ba, _bb) { \
    auto cr = TH->getCR(); \
    cr = bit_set(cr, _bt, bit_test(cr, 32, _ba) ^ bit_test(cr, 32, _bb)); \
    TH->setCR(cr); \
}
EMU_REWRITE(CRXOR, XLForm_1, i->BT_u(), i->BA_u(), i->BB_u())


// Condition Register NAND, p28

PRINT(CRNAND, XLForm_1) {
    *result = format_nnn("crnand", i->BT(), i->BA(), i->BB());
}

#define _CRNAND(_bt, _ba, _bb) { \
    auto cr = TH->getCR(); \
    cr = bit_set(cr, _bt, !(bit_test(cr, 32, _ba) & bit_test(cr, 32, _bb))); \
    TH->setCR(cr); \
}
EMU_REWRITE(CRNAND, XLForm_1, i->BT_u(), i->BA_u(), i->BB_u())


// Condition Register NOR, p29

PRINT(CRNOR, XLForm_1) {
    *result = format_nnn("crnor", i->BT(), i->BA(), i->BB());
}

#define _CRNOR(_bt, _ba, _bb) { \
    auto cr = TH->getCR(); \
    cr = bit_set(cr, _bt, (!bit_test(cr, 32, _ba)) | bit_test(cr, 32, _bb)); \
    TH->setCR(cr); \
}
EMU_REWRITE(CRNOR, XLForm_1, i->BT_u(), i->BA_u(), i->BB_u())


// Condition Register Equivalent, p29

PRINT(CREQV, XLForm_1) {
    *result = format_nnn("creqv", i->BT(), i->BA(), i->BB());
}

#define _CREQV(_bt, _ba, _bb) { \
    auto cr = TH->getCR(); \
    cr = bit_set(cr, _bt, bit_test(cr, 32, _ba) == bit_test(cr, 32, _bb)); \
    TH->setCR(cr); \
}
EMU_REWRITE(CREQV, XLForm_1, i->BT_u(), i->BA_u(), i->BB_u())


// Condition Register AND with Complement, p29

PRINT(CRANDC, XLForm_1) {
    *result = format_nnn("crandc", i->BT(), i->BA(), i->BB());
}

#define _CRANDC(_bt, _ba, _bb) { \
    auto cr = TH->getCR(); \
    cr = bit_set(cr, _bt, bit_test(cr, 32, _ba) & (!bit_test(cr, 32, _bb))); \
    TH->setCR(cr); \
}
EMU_REWRITE(CRANDC, XLForm_1, i->BT_u(), i->BA_u(), i->BB_u())


// Condition Register OR with Complement, p29

PRINT(CRORC, XLForm_1) {
    *result = format_nnn("crorc", i->BT(), i->BA(), i->BB());
}

#define _CRORC(_bt, _ba, _bb) { \
    auto cr = TH->getCR(); \
    cr = bit_set(cr, _bt, bit_test(cr, 32, _ba) | (!bit_test(cr, 32, _bb))); \
    TH->setCR(cr); \
}
EMU_REWRITE(CRORC, XLForm_1, i->BT_u(), i->BA_u(), i->BB_u())


// Condition Register Field Instruction, p30

PRINT(MCRF, XLForm_3) {
    *result = format_nn("mcrf", i->BF(), i->BFA());
}

#define _MCRF(_bf, _bfa) { \
    TH->setCRF(_bf, TH->getCRF(_bfa)); \
}
EMU_REWRITE(MCRF, XLForm_3, i->BF_u(), i->BFA_u())


// Load Byte and Zero, p34

PRINT(LBZ, DForm_1) {
    *result = format_br_nnn("lbz", i->RT(), i->D(), i->RA());
}

#define _LBZ(_ra, _ds, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    TH->setGPR(_rt, MM->load8(ea)); \
}
EMU_REWRITE(LBZ, DForm_1, i->RA_u(), i->D_s(), i->RT_u())


// Load Byte and Zero, p34

PRINT(LBZX, XForm_1) {
    *result = format_nnn("lbzx", i->RT(), i->RA(), i->RB());
}

#define _LBZX(_ra, _rb, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    TH->setGPR(_rt, MM->load8(ea)); \
}
EMU_REWRITE(LBZX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// Load Byte and Zero with Update, p34

PRINT(LBZU, DForm_1) {
    *result = format_br_nnn("lbzu", i->RT(), i->D(), i->RA());
}

#define _LBZU(_ds, _ra, _rt) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    TH->setGPR(_rt, MM->load8(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LBZU, DForm_1, i->D_s(), i->RA_u(), i->RT_u())


// Load Byte and Zero with Update Indexed, p34

PRINT(LBZUX, XForm_1) {
    *result = format_nnn("lbzux", i->RT(), i->RA(), i->RB());
}

#define _LBZUX(_ra, _rb, _rt) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    TH->setGPR(_rt, MM->load8(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LBZUX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// Load Halfword and Zero, p35

PRINT(LHZ, DForm_1) {
    *result = format_br_nnn("lhz", i->RT(), i->D(), i->RA());
}

#define _LHZ(_ra, _ds, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    TH->setGPR(_rt, MM->load16(ea)); \
}
EMU_REWRITE(LHZ, DForm_1, i->RA_u(), i->D_s(), i->RT_u())


// Load Halfword and Zero Indexed, p35

PRINT(LHZX, XForm_1) {
    *result = format_nnn("lhzx", i->RT(), i->RA(), i->RB());
}

#define _LHZX(_ra, _rb, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    TH->setGPR(_rt, MM->load16(ea)); \
}
EMU_REWRITE(LHZX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// Load Halfword and Zero with Update, p35

PRINT(LHZU, DForm_1) {
    *result = format_br_nnn("lhzu", i->RT(), i->D(), i->RA());
}

#define _LHZU(_ds, _ra, _rt) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    TH->setGPR(_rt, MM->load16(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LHZU, DForm_1, i->D_s(), i->RA_u(), i->RT_u())


// Load Halfword and Zero with Update Indexed, p35

PRINT(LHZUX, XForm_1) {
    *result = format_nnn("lhzux", i->RT(), i->RA(), i->RB());
}

#define _LHZUX(_ra, _rb, _rt) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    TH->setGPR(_rt, MM->load16(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LHZUX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// Load Halfword Algebraic, p36

PRINT(LHA, DForm_1) {
    *result = format_br_nnn("lha", i->RT(), i->D(), i->RA());
}

#define _LHA(_ra, _ds, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    TH->setGPR(_rt, (int16_t)MM->load16(ea)); \
}
EMU_REWRITE(LHA, DForm_1, i->RA_u(), i->D_s(), i->RT_u())


// Load Halfword Algebraic Indexed, p36

PRINT(LHAX, XForm_1) {
    *result = format_nnn("LHAX", i->RT(), i->RA(), i->RB());
}

#define _LHAX(_ra, _rb, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    TH->setGPR(_rt, (int16_t)MM->load16(ea)); \
}
EMU_REWRITE(LHAX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// Load Halfword Algebraic with Update, p36

PRINT(LHAU, DForm_1) {
    *result = format_br_nnn("lhaux", i->RT(), i->D(), i->RA());
}

#define _LHAU(_ds, _ra, _rt) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    TH->setGPR(_rt, (int16_t)MM->load16(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LHAU, DForm_1, i->D_s(), i->RA_u(), i->RT_u())


// Load Halfword Algebraic with Update Indexed, p36

PRINT(LHAUX, XForm_1) {
    *result = format_nnn("lhaux", i->RT(), i->RA(), i->RB());
}

#define _LHAUX(_ra, _rb, _rt) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    TH->setGPR(_rt, (int16_t)MM->load16(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LHAUX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// Load Word and Zero, p37

PRINT(LWZ, DForm_1) {
    *result = format_br_nnn("lwz", i->RT(), i->D(), i->RA());
}

#define _LWZ(_ra, _ds, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    TH->setGPR(_rt, MM->load32(ea)); \
}
EMU_REWRITE(LWZ, DForm_1, i->RA_u(), i->D_s(), i->RT_u())


// Load Word and Zero Indexed, p37

PRINT(LWZX, XForm_1) {
    *result = format_nnn("lwzx", i->RT(), i->RA(), i->RB());
}

#define _LWZX(_ra, _rb, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    TH->setGPR(_rt, MM->load32(ea)); \
}
EMU_REWRITE(LWZX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// Load Word and Zero with Update, p37

PRINT(LWZU, DForm_1) {
    *result = format_br_nnn("lwzu", i->RT(), i->D(), i->RA());
}

#define _LWZU(_ds, _ra, _rt) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    TH->setGPR(_rt, MM->load32(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LWZU, DForm_1, i->D_s(), i->RA_u(), i->RT_u())


// Load Word and Zero with Update Indexed, p37

PRINT(LWZUX, XForm_1) {
    *result = format_nnn("lwzux", i->RT(), i->RA(), i->RB());
}

#define _LWZUX(_ra, _rb, _rt) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    TH->setGPR(_rt, MM->load32(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LWZUX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// Load Word Algebraic, p38

PRINT(LWA, DSForm_1) {
    *result = format_br_nnn("lwa", i->RT(), i->DS(), i->RA());
}

#define _LWA(_ra, _ds, _rt) { \
    auto b = _ra == 0 ? 0 : TH->getGPR(_ra); \
    auto ea = b + _ds; \
    TH->setGPR(_rt, (int32_t)MM->load32(ea)); \
}
EMU_REWRITE(LWA, DSForm_1, i->RA_u(), i->DS_native(), i->RT_u())


// Load Word Algebraic Indexed, p38

PRINT(LWAX, XForm_1) {
    *result = format_nnn("lwax", i->RT(), i->RA(), i->RB());
}

#define _LWAX(_ra, _rb, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    TH->setGPR(_rt, (int32_t)MM->load32(ea)); \
}
EMU_REWRITE(LWAX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// Load Word Algebraic with Update Indexed, p38

PRINT(LWAUX, XForm_1) {
    *result = format_nnn("lwaux", i->RT(), i->RA(), i->RB());
}

#define _LWAUX(_ra, _rb, _rt) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    TH->setGPR(_rt, (int32_t)MM->load32(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LWAUX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// Load Doubleword, p39

PRINT(LD, DSForm_1) {
    *result = format_br_nnn("ld", i->RT(), i->DS(), i->RA());
}

#define _LD(_ra, _ds, _rt) { \
    auto b = _ra == 0 ? 0 : TH->getGPR(_ra); \
    auto ea = b + _ds; \
    TH->setGPR(_rt, MM->load64(ea)); \
}
EMU_REWRITE(LD, DSForm_1, i->RA_u(), i->DS_native(), i->RT_u())


// Load Doubleword Indexed, p39

PRINT(LDX, XForm_1) {
    *result = format_nnn("ldx", i->RT(), i->RA(), i->RB());
}

#define _LDX(_ra, _rb, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    TH->setGPR(_rt, MM->load64(ea)); \
}
EMU_REWRITE(LDX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// Load Doubleword with Update, p39

PRINT(LDU, DSForm_1) {
    *result = format_br_nnn("ldu", i->RT(), i->DS(), i->RA());
}

#define _LDU(_ds, _ra, _rt) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    TH->setGPR(_rt, MM->load64(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LDU, DSForm_1, i->DS_native(), i->RA_u(), i->RT_u())


// Load Doubleword with Update Indexed, p39

PRINT(LDUX, XForm_1) {
    *result = format_nnn("ldux", i->RT(), i->RA(), i->RB());
}

#define _LDUX(_ra, _rb, _rt) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    TH->setGPR(_rt, MM->load64(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LDUX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


// STORES
//

inline void PrintStore(const char* mnemonic, DForm_3* i, std::string* result) {
    *result = format_br_nnn(mnemonic, i->RS(), i->D(), i->RA());
}

inline void PrintStoreIndexed(const char* mnemonic, XForm_8* i, std::string* result) {
    *result = format_nnn(mnemonic, i->RS(), i->RA(), i->RB());
}

PRINT(STB, DForm_3) {
    PrintStore("stb", i, result);
}
#define _STB(_ra, _ds, _rs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    MM->store8(ea, TH->getGPR(_rs), TH->granule()); \
}
EMU_REWRITE(STB, DForm_3, i->RA_u(), i->D_s(), i->RS_u())

PRINT(STBX, XForm_8) {
    PrintStoreIndexed("stbx", i, result);
}
#define _STBX(_ra, _rb, _rs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    MM->store8(ea, TH->getGPR(_rs), TH->granule()); \
}
EMU_REWRITE(STBX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())

PRINT(STBU, DForm_3) {
    PrintStore("stbu", i, result);
}
#define _STBU(_ds, _ra, _rs) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    MM->store8(ea, TH->getGPR(_rs), TH->granule()); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(STBU, DForm_3, i->D_s(), i->RA_u(), i->RS_u())

PRINT(STBUX, XForm_8) {
    PrintStoreIndexed("stbux", i, result);
}
#define _STBUX(_ra, _rb, _rs) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    MM->store8(ea, TH->getGPR(_rs), TH->granule()); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(STBUX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())


PRINT(STH, DForm_3) {
    PrintStore("sth", i, result);
}
#define _STH(_ra, _ds, _rs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    MM->store16(ea, TH->getGPR(_rs), TH->granule()); \
}
EMU_REWRITE(STH, DForm_3, i->RA_u(), i->D_s(), i->RS_u())

PRINT(STHX, XForm_8) {
    PrintStoreIndexed("sthx", i, result);
}
#define _STHX(_ra, _rb, _rs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    MM->store16(ea, TH->getGPR(_rs), TH->granule()); \
}
EMU_REWRITE(STHX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())

PRINT(STHU, DForm_3) {
    PrintStore("sthu", i, result);
}
#define _STHU(_ds, _ra, _rs) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    MM->store16(ea, TH->getGPR(_rs), TH->granule()); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(STHU, DForm_3, i->D_s(), i->RA_u(), i->RS_u())

PRINT(STHUX, XForm_8) {
    PrintStoreIndexed("sthux", i, result);
}
#define _STHUX(_ra, _rb, _rs) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    MM->store16(ea, TH->getGPR(_rs), TH->granule()); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(STHUX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())


PRINT(STW, DForm_3) {
    PrintStore("stw", i, result);
}
#define _STW(_ra, _ds, _rs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    MM->store32(ea, TH->getGPR(_rs), TH->granule()); \
}
EMU_REWRITE(STW, DForm_3, i->RA_u(), i->D_s(), i->RS_u())

PRINT(STWX, XForm_8) {
    PrintStoreIndexed("stwx", i, result);
}
#define _STWX(_ra, _rb, _rs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    MM->store32(ea, TH->getGPR(_rs), TH->granule()); \
}
EMU_REWRITE(STWX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())

PRINT(STWU, DForm_3) {
    PrintStore("stwu", i, result);
}
#define _STWU(_ds, _ra, _rs) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    MM->store32(ea, TH->getGPR(_rs), TH->granule()); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(STWU, DForm_3, i->D_s(), i->RA_u(), i->RS_u())

PRINT(STWUX, XForm_8) {
    PrintStoreIndexed("stwux", i, result);
}
#define _STWUX(_ra, _rb, _rs) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    MM->store32(ea, TH->getGPR(_rs), TH->granule()); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(STWUX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())


PRINT(STD, DSForm_2) {
    *result = format_br_nnn("std", i->RS(), i->DS(), i->RA());
}

#define _STD(_ra, _ds, _rs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    MM->store64(ea, TH->getGPR(_rs), TH->granule()); \
}
EMU_REWRITE(STD, DSForm_2, i->RA_u(), i->DS_native(), i->RS_u())


PRINT(STDX, XForm_8) {
    PrintStoreIndexed("stdx", i, result);
}

#define _STDX(_ra, _rb, _rs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    MM->store64(ea, TH->getGPR(_rs), TH->granule()); \
}
EMU_REWRITE(STDX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())


PRINT(STDU, DSForm_2) {
    *result = format_br_nnn("stdu", i->RS(), i->DS(), i->RA());
}

#define _STDU(_ds, _ra, _rs) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    MM->store64(ea, TH->getGPR(_rs), TH->granule()); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(STDU, DSForm_2, i->DS_native(), i->RA_u(), i->RS_u())


PRINT(STDUX, XForm_8) {
    PrintStoreIndexed("stdux", i, result);
}

#define _STDUX(_ra, _rb, _rs) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    MM->store64(ea, TH->getGPR(_rs), TH->granule()); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(STDUX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())


// Fixed-Point Load and Store with Byte Reversal Instructions, p44

PRINT(LHBRX, XForm_1) {
    *result = format_nnn("lhbrx", i->RT(), i->RA(), i->RB());
}
#define _LHBRX(_ra, _rb, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    TH->setGPR(_rt, endian_reverse(MM->load16(ea))); \
}
EMU_REWRITE(LHBRX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())

PRINT(LWBRX, XForm_1) {
    *result = format_nnn("lwbrx", i->RT(), i->RA(), i->RB());
}
#define _LWBRX(_ra, _rb, _rt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    TH->setGPR(_rt, endian_reverse(MM->load32(ea))); \
}
EMU_REWRITE(LWBRX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


PRINT(STHBRX, XForm_8) {
    *result = format_nnn("sthbrx", i->RS(), i->RA(), i->RB());
}
#define _STHBRX(_ra, _rb, _rs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    MM->store16(ea, endian_reverse(TH->getGPR(_rs)), TH->granule()); \
}
EMU_REWRITE(STHBRX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())

PRINT(STWBRX, XForm_8) {
    *result = format_nnn("stwbrx", i->RS(), i->RA(), i->RB());
}
#define _STWBRX(_ra, _rb, _rs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    MM->store32(ea, endian_reverse(TH->getGPR(_rs)), TH->granule()); \
}
EMU_REWRITE(STWBRX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())


// Fixed-Point Arithmetic Instructions, p51

PRINT(ADDI, DForm_2) {
    *result = format_nnn("addi", i->RT(), i->RA(), i->SI());
}

#define _ADDI(_ra, _sis, _rt) { \
    auto b = getB(_ra, TH); \
    TH->setGPR(_rt, _sis + b); \
}
EMU_REWRITE(ADDI, DForm_2, i->RA_u(), i->SI_s(), i->RT_u())


PRINT(ADDIS, DForm_2) {
    *result = format_nnn("addis", i->RT(), i->RA(), i->SI());
}

#define _ADDIS(_ra, _siu, _rt) { \
    auto b = getB(_ra, TH); \
    TH->setGPR(_rt, (int32_t)(_siu << 16) + b); \
}
EMU_REWRITE(ADDIS, DForm_2, i->RA_u(), i->SI_u(), i->RT_u())


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

inline void update_CR0_OV(unsigned oe,
                          unsigned rc,
                          bool ov, int64_t result, PPUThread* thread) {
    if (oe && ov) {
        TH->setOV();
    }
    if (rc) {
        update_CR0(result, TH);
    }
}

PRINT(ADD, XOForm_1) {
    const char* mnemonics[][2] = {
        { "add", "add." }, { "addo", "addo." }
    };
    *result = format_nnn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA(), i->RB());
}

#define _ADD(_ra, _rb, _rt, _oe, _rc) { \
    auto ra = TH->getGPR(_ra); \
    auto rb = TH->getGPR(_rb); \
    unsigned __int128 res = (__int128)ra + rb; \
    bool ov = res > 0xffffffffffffffff; \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ov, res, TH); \
}
EMU_REWRITE(ADD, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->OE_u(), i->Rc_u())


PRINT(ADDZE, XOForm_3) {
    const char* mnemonics[][2] = {
        { "addze", "addze." }, { "addzeo", "addzeo." }
    };
    *result = format_nn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA());
}

#define _ADDZE(_ra, _rt, _oe, _rc) { \
    auto ra = TH->getGPR(_ra); \
    unsigned __int128 res = (__int128)ra + TH->getCA(); \
    bool ov = res > 0xffffffffffffffff; \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ov, res, TH); \
}
EMU_REWRITE(ADDZE, XOForm_3, i->RA_u(), i->RT_u(), i->OE_u(), i->Rc_u())


PRINT(SUBF, XOForm_1) {
    const char* mnemonics[][2] = {
        { "subf", "subf." }, { "subfo", "subfo." }
    };
    *result = format_nnn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA(), i->RB());
}

#define _SUBF(_ra, _rb, _oe, _rt, _rc) { \
    auto ra = TH->getGPR(_ra); \
    auto rb = TH->getGPR(_rb); \
    uint64_t res; \
    bool ov = 0; \
    if (_oe) \
        ov = __builtin_usubl_overflow(rb, ra, &res); \
    else \
        res = rb - ra; \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ov, res, TH); \
}
EMU_REWRITE(SUBF, XOForm_1, i->RA_u(), i->RB_u(), i->OE_u(), i->RT_u(), i->Rc_u())


PRINT(ADDIC, DForm_2) {
    *result = format_nnn("addic", i->RT(), i->RA(), i->SI());
}

#define _ADDIC(_ra, _sis, _rt) { \
    auto ra = TH->getGPR(_ra); \
    auto si = _sis; \
    uint64_t res; \
    auto ov = __builtin_uaddl_overflow(ra, si, &res); \
    TH->setGPR(_rt, res); \
    TH->setCA(ov); \
}
EMU_REWRITE(ADDIC, DForm_2, i->RA_u(), i->SI_s(), i->RT_u())


PRINT(ADDICD, DForm_2) {
    *result = format_nnn("addic.", i->RT(), i->RA(), i->SI());
}

#define _ADDICD(_ra, _sis, _rt) { \
    auto ra = TH->getGPR(_ra); \
    auto si = _sis; \
    uint64_t res; \
    auto ov = __builtin_uaddl_overflow(ra, si, &res); \
    TH->setGPR(_rt, res); \
    TH->setCA(ov); \
    update_CR0(res, TH); \
}
EMU_REWRITE(ADDICD, DForm_2, i->RA_u(), i->SI_s(), i->RT_u())


PRINT(SUBFIC, DForm_2) {
    *result = format_nnn("subfic", i->RT(), i->RA(), i->SI());
}

#define _SUBFIC(_ra, _sis, _rt) { \
    auto ra = TH->getGPR(_ra); \
    auto si = _sis; \
    int64_t res; \
    auto ov = __builtin_ssubl_overflow(si, ra, &res); \
    TH->setGPR(_rt, res); \
    TH->setCA(ov); \
}
EMU_REWRITE(SUBFIC, DForm_2, i->RA_u(), i->SI_s(), i->RT_u())


PRINT(ADDC, XOForm_1) {
    const char* mnemonics[][2] = {
        { "addc", "addc." }, { "addco", "addco." }
    };
    *result = format_nnn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA(), i->RB());
}

#define _ADDC(_ra, _rb, _rt, _oe, _rc) { \
    auto ra = TH->getGPR(_ra); \
    auto rb = TH->getGPR(_rb); \
    unsigned __int128 res = ra; \
    res += rb; \
    auto ca = res >> 64; \
    TH->setCA(ca); \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ca, res, TH); \
}
EMU_REWRITE(ADDC, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->OE_u(), i->Rc_u())


PRINT(SUBFC, XOForm_1) {
    const char* mnemonics[][2] = {
        { "subfc", "subfc." }, { "subfco", "subfco." }
    };
    *result = format_nnn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA(), i->RB());
}

#define _SUBFC(_ra, _rb, _rt, _oe, _rc) { \
    auto ra = TH->getGPR(_ra); \
    auto rb = TH->getGPR(_rb); \
    unsigned __int128 res = ~ra; \
    res += rb; \
    res += 1; \
    auto ca = res >> 64; \
    TH->setCA(ca); \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ca, res, TH); \
}
EMU_REWRITE(SUBFC, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->OE_u(), i->Rc_u())


PRINT(SUBFE, XOForm_1) {
    const char* mnemonics[][2] = {
        { "subfe", "subfe." }, { "subfeo", "subfeo." }
    };
    *result = format_nnn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA(), i->RB());
}

#define _SUBFE(_ra, _rb, _rt, _oe, _rc) { \
    auto ra = TH->getGPR(_ra); \
    auto rb = TH->getGPR(_rb); \
    unsigned __int128 res = ~ra; \
    res += rb; \
    res += TH->getCA(); \
    auto ca = res >> 64; \
    TH->setCA(ca); \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ca, res, TH); \
}
EMU_REWRITE(SUBFE, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->OE_u(), i->Rc_u())


// Fixed-Point Logical Instructions, p65

// AND

PRINT(ANDID, DForm_4) {
    *result = format_nnn("andi.", i->RA(), i->RS(), i->UI());
}

#define _ANDID(_ui, _rs, _ra) { \
    auto res = TH->getGPR(_rs) & _ui; \
    update_CR0(res, TH); \
    TH->setGPR(_ra, res); \
}
EMU_REWRITE(ANDID, DForm_4, i->UI_u(), i->RS_u(), i->RA_u())


PRINT(ANDISD, DForm_4) {
    *result = format_nnn("andis.", i->RA(), i->RS(), i->UI());
}

#define _ANDISD(_ui, _rs, _ra) { \
    auto res = TH->getGPR(_rs) & (_ui << 16); \
    update_CR0(res, TH); \
    TH->setGPR(_ra, res); \
}
EMU_REWRITE(ANDISD, DForm_4, i->UI_u(), i->RS_u(), i->RA_u())


// OR

PRINT(ORI, DForm_4) {
    if (i->RS_u() == 0 && i->RA_u() == 0 && i->UI_u() == 0) {
        *result = "nop";
    } else {
        *result = format_nnn("ori", i->RA(), i->RS(), i->UI());
    }
}

#define _ORI(_ui, _rs, _ra) { \
    auto res = TH->getGPR(_rs) | _ui; \
    TH->setGPR(_ra, res); \
}
EMU_REWRITE(ORI, DForm_4, i->UI_u(), i->RS_u(), i->RA_u())


PRINT(ORIS, DForm_4) {
    *result = format_nnn("oris", i->RA(), i->RS(), i->UI());
}

#define _ORIS(_ui, _rs, _ra) { \
    auto res = TH->getGPR(_rs) | (_ui << 16); \
    TH->setGPR(_ra, res); \
}
EMU_REWRITE(ORIS, DForm_4, i->UI_u(), i->RS_u(), i->RA_u())


// XOR

PRINT(XORI, DForm_4) {
    *result = format_nnn("xori", i->RA(), i->RS(), i->UI());
}

#define _XORI(_ui, _rs, _ra) { \
    auto res = TH->getGPR(_rs) ^ _ui; \
    TH->setGPR(_ra, res); \
}
EMU_REWRITE(XORI, DForm_4, i->UI_u(), i->RS_u(), i->RA_u())


PRINT(XORIS, DForm_4) {
    *result = format_nnn("xoris", i->RA(), i->RS(), i->UI());
}

#define _XORIS(_ui, _rs, _ra) { \
    auto res = TH->getGPR(_rs) ^ (_ui << 16); \
    TH->setGPR(_ra, res); \
}
EMU_REWRITE(XORIS, DForm_4, i->UI_u(), i->RS_u(), i->RA_u())


// X-forms

PRINT(AND, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "and." : "and", i->RA(), i->RS(), i->RB());
}

#define _AND(_rs, _rb, _ra, _rc) { \
    auto res = TH->getGPR(_rs) & TH->getGPR(_rb); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(AND, XForm_6, i->RS_u(), i->RB_u(), i->RA_u(), i->Rc_u())


PRINT(OR, XForm_6) {
    if (i->RS_u() == i->RB_u()) {
        *result = format_nn(i->Rc_u() ? "mr." : "mr", i->RA(), i->RS());
    } else {
        *result = format_nnn(i->Rc_u() ? "or." : "or", i->RA(), i->RS(), i->RB());
    }
}

#define _OR(_rs, _rb, _ra, _rc) { \
    auto res = TH->getGPR(_rs) | TH->getGPR(_rb); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(OR, XForm_6, i->RS_u(), i->RB_u(), i->RA_u(), i->Rc_u())


PRINT(XOR, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "xor." : "xor", i->RA(), i->RS(), i->RB());
}

#define _XOR(_rs, _rb, _ra, _rc) { \
    auto res = TH->getGPR(_rs) ^ TH->getGPR(_rb); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(XOR, XForm_6, i->RS_u(), i->RB_u(), i->RA_u(), i->Rc_u())


PRINT(NAND, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "nand." : "nand", i->RA(), i->RS(), i->RB());
}

#define _NAND(_rs, _rb, _ra, _rc) { \
    auto res = ~(TH->getGPR(_rs) & TH->getGPR(_rb)); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(NAND, XForm_6, i->RS_u(), i->RB_u(), i->RA_u(), i->Rc_u())


PRINT(NOR, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "nor." : "nor", i->RA(), i->RS(), i->RB());
}

#define _NOR(_rs, _rb, _ra, _rc) { \
    auto res = ~(TH->getGPR(_rs) | TH->getGPR(_rb)); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(NOR, XForm_6, i->RS_u(), i->RB_u(), i->RA_u(), i->Rc_u())


PRINT(EQV, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "eqv." : "eqv", i->RA(), i->RS(), i->RB());
}

#define _EQV(_rs, _rb, _ra, _rc) { \
    auto res = ~(TH->getGPR(_rs) ^ TH->getGPR(_rb)); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(EQV, XForm_6, i->RS_u(), i->RB_u(), i->RA_u(), i->Rc_u())


PRINT(ANDC, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "andc." : "andc", i->RA(), i->RS(), i->RB());
}

#define _ANDC(_rs, _rb, _ra, _rc) { \
    auto res = TH->getGPR(_rs) & ~TH->getGPR(_rb); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(ANDC, XForm_6, i->RS_u(), i->RB_u(), i->RA_u(), i->Rc_u())


PRINT(ORC, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "orc." : "orc", i->RA(), i->RS(), i->RB());
}

#define _ORC(_rs, _rb, _ra, _rc) { \
    auto res = TH->getGPR(_rs) | ~TH->getGPR(_rb); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(ORC, XForm_6, i->RS_u(), i->RB_u(), i->RA_u(), i->Rc_u())


// Extend Sign

PRINT(EXTSB, XForm_11) {
    *result = format_nn(i->Rc_u() ? "extsb." : "extsb", i->RA(), i->RS());
}

#define _EXTSB(_rs, _ra) { \
    TH->setGPR(_ra, (int64_t)static_cast<int8_t>(TH->getGPR(_rs))); \
}
EMU_REWRITE(EXTSB, XForm_11, i->RS_u(), i->RA_u())


PRINT(EXTSH, XForm_11) {
    *result = format_nn(i->Rc_u() ? "extsh." : "extsh", i->RA(), i->RS());
}

#define _EXTSH(_rs, _ra) { \
    TH->setGPR(_ra, (int64_t)static_cast<int16_t>(TH->getGPR(_rs))); \
}
EMU_REWRITE(EXTSH, XForm_11, i->RS_u(), i->RA_u())


PRINT(EXTSW, XForm_11) {
    *result = format_nn(i->Rc_u() ? "extsw." : "extsw", i->RA(), i->RS());
}

#define _EXTSW(_rs, _ra) { \
    TH->setGPR(_ra, (int64_t)static_cast<int32_t>(TH->getGPR(_rs))); \
}
EMU_REWRITE(EXTSW, XForm_11, i->RS_u(), i->RA_u())


// Move To/From System Register Instructions, p81

enum {
    SPR_XER = 1u << 5,
    SPR_LR = 8u << 5,
    SPR_CTR = 9u << 5,
    SPR_VRSAVE = 8,
};

PRINT(MTSPR, XFXForm_7) {
    switch (i->spr_u()) {
        case SPR_XER: *result = format_n("mtxer", i->RS()); break;
        case SPR_LR: *result = format_n("mtlr", i->RS()); break;
        case SPR_CTR: *result = format_n("mtctr", i->RS()); break;
        case SPR_VRSAVE: *result = format_n("mfvrsave", i->RS()); break;
        default: throw IllegalInstructionException();
    }
}

#define _MTSPR(_rs, _spr) { \
    auto rs = TH->getGPR(_rs); \
    switch (_spr) { \
        case SPR_XER: TH->setXER(rs); break; \
        case SPR_LR: TH->setLR(rs); break; \
        case SPR_CTR: TH->setCTR(rs); break; \
        case SPR_VRSAVE: TH->setVRSAVE(rs); break; \
        default: throw IllegalInstructionException(); \
    } \
}
EMU_REWRITE(MTSPR, XFXForm_7, i->RS_u(), i->spr_u())


PRINT(MFSPR, XFXForm_7) {
    switch (i->spr_u()) {
        case SPR_XER: *result = format_n("mfxer", i->RS()); break;
        case SPR_LR: *result = format_n("mflr", i->RS()); break;
        case SPR_CTR: *result = format_n("mfctr", i->RS()); break;
        case SPR_VRSAVE: *result = format_n("mfvrsave", i->RS()); break;
        default: throw IllegalInstructionException();
    }
}

#define _MFSPR(_spr, _rs) { \
    uint32_t v; \
    switch (_spr) { \
        case SPR_XER: v = TH->getXER(); break; \
        case SPR_LR: v = TH->getLR(); break; \
        case SPR_CTR: v = TH->getCTR(); break; \
        case SPR_VRSAVE: v = TH->getVRSAVE(); break; \
        default: throw IllegalInstructionException(); \
    } \
    TH->setGPR(_rs, v); \
}
EMU_REWRITE(MFSPR, XFXForm_7, i->spr_u(), i->RS_u())


template <int Pos04, int Pos5>
inline uint8_t getNBE(BitField<Pos04, Pos04 + 5> _04, BitField<Pos5, Pos5 + 1> _05) {
    return (_05.u() << 5) | _04.u();
}

PRINT(RLDICL, MDForm_1) {
    auto n = getNBE(i->sh04(), i->sh5());
    auto b = getNBE(i->mb04(), i->mb5());
    if (b == 0 && n > 32) {
        *result = format_nnu(i->Rc_u() ? "rotrdi." : "rotrdi", i->RA(), i->RS(), 64 - n);
    } else if (b == 0 && n <= 32 ) {
        *result = format_nnu(i->Rc_u() ? "rotldi." : "rotldi", i->RA(), i->RS(), n);
    } else if (b == 64 - n) {
        *result = format_nnu(i->Rc_u() ? "srdi." : "srdi", i->RA(), i->RS(), b);
    } else if (n == 0) {
        *result = format_nnu(i->Rc_u() ? "clrldi." : "clrldi", i->RA(), i->RS(), b);
    } else if (64 - b > 0) {
        *result = format_nnuu(i->Rc_u() ? "extrdi." : "extrdi", i->RA(), i->RS(), 64 - b, n + b - 64);
    } else {
        *result = format_nnuu(i->Rc_u() ? "rldicl." : "rldicl", i->RA(), i->RS(), b + n, 64 - n);
    }
}

#define _RLDICL(nbe_sh, nbe_mb, _rs, _ra, _rc) { \
    auto n = nbe_sh; \
    auto b = nbe_mb; \
    auto r = rol<uint64_t>(TH->getGPR(_rs), n); \
    auto m = mask<64>(b, 63); \
    auto res = r & m; \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(RLDICL, MDForm_1, getNBE(i->sh04(), i->sh5()), getNBE(i->mb04(), i->mb5()), i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(RLDICR, MDForm_2) {
    auto n = getNBE(i->sh04(), i->sh5());
    auto b = getNBE(i->me04(), i->me5());
    if (63 - b == n) {
        *result = format_nnu(i->Rc_u() ? "sldi." : "sldi", i->RA(), i->RS(), n);
    } else if (n == 0) {
        *result = format_nnu(i->Rc_u() ? "clrrdi." : "clrrdi", i->RA(), i->RS(), 63 - b);
    } else if (b > 1) {
        *result = format_nnuu(i->Rc_u() ? "extldi." : "extldi", i->RA(), i->RS(), b + 1, n);
    } else {
        *result = format_nnuu(i->Rc_u() ? "rldicr." : "rldicr", i->RA(), i->RS(), b + n, 64 - n);
    }
}

#define _RLDICR(nbe_sh, nbe_me, _rs, _ra, _rc) { \
    auto n = nbe_sh; \
    auto e = nbe_me; \
    auto r = rol<uint64_t>(TH->getGPR(_rs), n); \
    auto m = mask<64>(0, e); \
    auto res = r & m; \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(RLDICR, MDForm_2, getNBE(i->sh04(), i->sh5()), getNBE(i->me04(), i->me5()), i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(RLDIMI, MDForm_1) {
    auto n = getNBE(i->sh04(), i->sh5());
    auto b = getNBE(i->mb04(), i->mb5());
    if (n + b < 64) {
        *result = format_nnuu(i->Rc_u() ? "insrdi." : "insrdi", i->RA(), i->RS(), 64 - (n + b), b);
    } else {
        *result = format_nnuu(i->Rc_u() ? "rldimi." : "rldimi", i->RA(), i->RS(), n, b);
    }
}

#define _RLDIMI(nbe_sh, nbe_mb, _rs, _ra, _rc) { \
    auto n = nbe_sh; \
    auto b = nbe_mb; \
    auto r = rol<uint64_t>(TH->getGPR(_rs), n); \
    auto m = mask<64>(b, ~n & 63); \
    auto res = (r & m) | (TH->getGPR(_ra) & (~m)); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(RLDIMI, MDForm_1, getNBE(i->sh04(), i->sh5()), getNBE(i->mb04(), i->mb5()), i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(RLDCL, MDSForm_1) {
    if (i->mb_u() == 0) {
        *result = format_nnn(i->Rc_u() ? "rotld." : "rotld", i->RA(), i->RS(), i->RB());
    } else {
        *result = format_nnnn(i->Rc_u() ? "rotld." : "rotld", i->RA(), i->RS(), i->RB(), i->mb());
    }
}

#define _RLDCL(_rb, _rs, _mb, _ra, _rc) { \
    auto n = TH->getGPR(_rb) & 127; \
    auto r = rol<uint64_t>(TH->getGPR(_rs), n); \
    auto b = _mb; \
    auto m = mask<64>(b, 63); \
    auto res = r & m; \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(RLDCL, MDSForm_1, i->RB_u(), i->RS_u(), i->mb_u(), i->RA_u(), i->Rc_u())


PRINT(RLDCR, MDSForm_2) {
    *result = format_nnnn(i->Rc_u() ? "rldcr." : "rldcr", i->RA(), i->RS(), i->RB(), i->me());
}

#define _RLDCR(_rb, _rs, _me, _ra, _rc) { \
    auto n = TH->getGPR(_rb) & 127; \
    auto r = rol<uint64_t>(TH->getGPR(_rs), n); \
    auto e = _me; \
    auto m = mask<64>(0, e); \
    auto res = r & m; \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(RLDCR, MDSForm_2, i->RB_u(), i->RS_u(), i->me_u(), i->RA_u(), i->Rc_u())


PRINT(RLDIC, MDForm_1) {
    auto n = getNBE(i->sh04(), i->sh5());
    auto b = getNBE(i->mb04(), i->mb5());
    if (n <= b + n && b + n < 64) {
        *result = format_nnuu(i->Rc_u() ? "clrlsldi." : "clrlsldi", i->RA(), i->RS(), b + n, n);
    } else {
        *result = format_nnuu(i->Rc_u() ? "rldic." : "rldic", i->RA(), i->RS(), n, b);
    }
}

#define _RLDIC(nbe_sh, nbe_mb, _rs, _ra, _rc) { \
    auto n = nbe_sh; \
    auto b = nbe_mb; \
    auto r = rol<uint64_t>(TH->getGPR(_rs), n); \
    auto m = mask<64>(b, ~n & 63); \
    auto res = r & m; \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(RLDIC, MDForm_1, getNBE(i->sh04(), i->sh5()), getNBE(i->mb04(), i->mb5()), i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(RLWINM, MForm_2) {
    auto s = i->SH_u();
    auto b = i->mb_u();
    auto e = i->me_u();
    if (b == 0 && e == 31) {
        if (s < 16) {
            *result = format_nnn(i->Rc_u() ? "rotlwi." : "rotlwi", i->RA(), i->RS(), i->SH());
        } else {
            *result = format_nnu(i->Rc_u() ? "rotrwi." : "rotrwi", i->RA(), i->RS(), 32 - s);
        }
    } else if (b == 0 && e == 31 - s) {
        *result = format_nnu(i->Rc_u() ? "slwi." : "slwi", i->RA(), i->RS(), s);
    } else if (e == 31 && 32 - s == b) {
        *result = format_nnu(i->Rc_u() ? "srwi." : "srwi", i->RA(), i->RS(), b);
    } else if (s == 0 && e == 31) {
        *result = format_nnu(i->Rc_u() ? "clrlwi." : "clrlwi", i->RA(), i->RS(), b);
    } else if (s == 0 && b == 0) {
        *result = format_nnu(i->Rc_u() ? "clrrwi." : "clrrwi", i->RA(), i->RS(), 31 - e);
    } else if (e == 31 - s && s <= s + b && s + b < 32) {
        *result = format_nnuu(i->Rc_u() ? "clrlslwi." : "clrlslwi", i->RA(), i->RS(), s + b, s);
    } else if (b == 0 && e <= 30) {
        *result = format_nnuu(i->Rc_u() ? "extlwi." : "extlwi", i->RA(), i->RS(), e + 1, s);
    } else if (e == 31) {
        *result = format_nnuu(i->Rc_u() ? "extrwi." : "extrwi", i->RA(), i->RS(), 32 - b, s + b - 32);
    } else {
        *result = format_nnnnn(i->Rc_u() ? "rlwinm." : "rlwinm", i->RA(), i->RS(), i->SH(), i->mb(), i->me());
    }
}

#define _RLWINM(_shu, _rs, _mb, _me, _ra, _rc) { \
    auto n = _shu; \
    auto r = rol<uint32_t>(TH->getGPR(_rs), n); \
    auto m = mask<64>(_mb + 32, _me + 32); \
    auto res = r & m; \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(RLWINM, MForm_2, i->SH_u(), i->RS_u(), i->mb_u(), i->me_u(), i->RA_u(), i->Rc_u())


PRINT(RLWNM, MForm_1) {
    if (i->mb_u() == 0 && i->me_u() == 31) {
        *result = format_nnn(i->Rc_u() ? "rotlw." : "rotlw", i->RA(), i->RS(), i->RB());
    } else {
        *result = format_nnnnn(i->Rc_u() ? "rlwnm." : "rlwnm", i->RA(), i->RS(), i->RB(), i->mb(), i->me());
    }
}

#define _RLWNM(_rb, _rs, _mb, _me, _ra, _rc) { \
    auto n = TH->getGPR(_rb) & 31; \
    auto r = rol<uint32_t>(TH->getGPR(_rs), n); \
    auto m = mask<64>(_mb + 32, _me + 32); \
    auto res = r & m; \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(RLWNM, MForm_1, i->RB_u(), i->RS_u(), i->mb_u(), i->me_u(), i->RA_u(), i->Rc_u())


PRINT(RLWIMI, MForm_2) {
    auto s = i->SH_u();
    auto b = i->mb_u();
    auto e = i->me_u();
    if (s == 32 - b && b <= e) {
        *result = format_nnuu(i->Rc_u() ? "inslwi." : "inslwi", i->RA(), i->RS(), e - b + 1, b);
    } else if (32 - s == e + 1 && e + 1 > b) {
        *result = format_nnuu(i->Rc_u() ? "insrwi." : "insrwi", i->RA(), i->RS(), e + 1 - b, b);
    } else {
        *result = format_nnnnn(i->Rc_u() ? "rlwimi." : "rlwimi", i->RA(), i->RS(), i->SH(), i->mb(), i->me());
    }
}

#define _RLWIMI(_shu, _rs, _mb, _me, _ra, _rc) { \
    auto n = _shu; \
    auto r = rol<uint32_t>(TH->getGPR(_rs), n); \
    auto m = mask<64>(_mb + 32, _me + 32); \
    auto res = (r & m) | (TH->getGPR(_ra) & ~m); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(RLWIMI, MForm_2, i->SH_u(), i->RS_u(), i->mb_u(), i->me_u(), i->RA_u(), i->Rc_u())


PRINT(SC, SCForm) {
    *result = "sc";
}

#define _SC(_) { \
    TH->scall(); \
}
EMU_REWRITE(SC, SCForm, 0)


PRINT(NCALL, NCallForm) {
    auto entry = findNCallEntryByIndex(i->idx_u());
    auto name = entry ? entry->name : "???";
    *result = sformat("ncall {} ({:x})", name, i->idx_u());
}

#define _NCALL(_idx) { \
    SET_REWRITER_NCALL; \
    TH->ncall(_idx); \
    RETURN_REWRITER_NCALL; \
}
EMU_REWRITE(NCALL, NCallForm, i->idx_u())

PRINT(BBCALL, BBCallForm) {
    *result = format_nn("bbcall", i->Segment(), i->Label());
}

#define _BBCALL(_so, _label) { \
    assert(false); \
}
EMU_REWRITE(BBCALL, BBCallForm, i->Segment_u(), i->Label_u());

inline int64_t get_cmp_ab(unsigned l, uint64_t value) {
    return l == 0 ? (int64_t)static_cast<int32_t>(value) : value;
}

PRINT(CMPI, DForm_5) {
    if (i->L_u() == 1) {
        if (i->BF_u() == 0) {
            *result = format_nn("cmpdi", i->RA(), i->SI());
        } else {
            *result = format_nnn("cmpdi", i->BF(), i->RA(), i->SI());
        }
    } else if (i->L_u() == 0) {
        *result = format_nnn("cmpwi", i->BF(), i->RA(), i->SI());
    } else {
        *result = format_nnnn("cmpi", i->BF(), i->L(), i->RA(), i->SI());
    }
}

#define _CMPI(_ra, _l, _sis, _bf) { \
    auto a = get_cmp_ab(_l, TH->getGPR(_ra)); \
    auto c = a < _sis ? 4 \
           : a > _sis ? 2 \
           : 1; \
    TH->setCRF_sign(_bf, c); \
}
EMU_REWRITE(CMPI, DForm_5, i->RA_u(), i->L_u(), i->SI_s(), i->BF_u())


PRINT(CMP, XForm_16) {
    auto mnemonic = i->L_u() ? "cmpd" : "cmpw";
    if (i->BF_u() == 0) {
        *result = format_nn(mnemonic, i->RA(), i->RB());
    } else {
        *result = format_nnn(mnemonic, i->BF(), i->RA(), i->RB());
    }
}

#define _CMP(_ra, _l, _rb, _bf) { \
    auto a = get_cmp_ab(_l, TH->getGPR(_ra)); \
    auto b = get_cmp_ab(_l, TH->getGPR(_rb)); \
    auto c = a < b ? 4 \
           : a > b ? 2 \
           : 1; \
    TH->setCRF_sign(_bf, c); \
}
EMU_REWRITE(CMP, XForm_16, i->RA_u(), i->L_u(), i->RB_u(), i->BF_u())


inline uint64_t get_cmpl_ab(unsigned l, uint64_t value) {
    return l == 0 ? static_cast<uint32_t>(value) : value;
}

PRINT(CMPLI, DForm_6) {
    auto mnemonic = i->L_u() ? "cmpldi" : "cmplwi";
    if (i->BF_u() == 0) {
        *result = format_nn(mnemonic, i->RA(), i->UI());
    } else {
        *result = format_nnn(mnemonic, i->BF(), i->RA(), i->UI());
    }
}

#define _CMPLI(_ra, _l, _ui, _bf) { \
    auto a = get_cmpl_ab(_l, TH->getGPR(_ra)); \
    auto c = a < _ui ? 4 \
           : a > _ui ? 2 \
           : 1; \
    TH->setCRF_sign(_bf, c); \
}
EMU_REWRITE(CMPLI, DForm_6, i->RA_u(), i->L_u(), i->UI_u(), i->BF_u())


PRINT(CMPL, XForm_16) {
    auto mnemonic = i->L_u() ? "cmpld" : "cmplw";
    if (i->BF_u() == 0) {
        *result = format_nn(mnemonic, i->RA(), i->RB());
    } else {
        *result = format_nnn(mnemonic, i->BF(), i->RA(), i->RB());
    }
}

#define _CMPL(_ra, _l, _rb, _bf) { \
    auto a = get_cmpl_ab(_l, TH->getGPR(_ra)); \
    auto b = get_cmpl_ab(_l, TH->getGPR(_rb)); \
    auto c = a < b ? 4 \
           : a > b ? 2 \
           : 1; \
    TH->setCRF_sign(_bf, c); \
}
EMU_REWRITE(CMPL, XForm_16, i->RA_u(), i->L_u(), i->RB_u(), i->BF_u())


PRINT(MFCR, XFXForm_3) {
    *result = format_n("mfcr", i->RT());
}

#define _MFCR(_rt) { \
    TH->setGPR(_rt, TH->getCR()); \
}
EMU_REWRITE(MFCR, XFXForm_3, i->RT_u())


PRINT(SLD, XForm_6) {
   *result = format_nnn(i->Rc_u() ? "sld." : "sld", i->RA(), i->RS(), i->RB());
}

#define _SLD(_rb, _rs, _ra, _rc) { \
    auto b = TH->getGPR(_rb) & 127; \
    auto res = b > 63 ? 0 : TH->getGPR(_rs) << b; \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(SLD, XForm_6, i->RB_u(), i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(SLW, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "slw." : "slw", i->RA(), i->RS(), i->RB());
}

#define _SLW(_rb, _rs, _ra, _rc) { \
    auto b = TH->getGPR(_rb) & 63; \
    auto res = b > 31 ? 0 : (uint32_t)TH->getGPR(_rs) << b; \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(SLW, XForm_6, i->RB_u(), i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(SRD, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "srd." : "srd", i->RA(), i->RS(), i->RB());
}

#define _SRD(_rb, _rs, _ra, _rc) { \
    auto b = TH->getGPR(_rb) & 127; \
    auto res = b > 63 ? 0 : TH->getGPR(_rs) >> b; \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(SRD, XForm_6, i->RB_u(), i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(SRW, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "srw." : "srw", i->RA(), i->RS(), i->RB());
}

#define _SRW(_rb, _rs, _ra, _rc) { \
    auto b = TH->getGPR(_rb) & 63; \
    auto res = b > 31 ? 0 : (uint32_t)TH->getGPR(_rs) >> b; \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(SRW, XForm_6, i->RB_u(), i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(SRADI, XSForm) {
    auto n =  getNBE(i->sh04(), i->sh5());
    *result = format_nnu(i->Rc_u() ? "sradi." : "sradi", i->RA(), i->RS(), n);
}

#define _SRADI(nbe_sh, _rs, _ra, _rc) { \
    auto n =  nbe_sh; \
    auto rs = TH->getGPR(_rs); \
    auto r = rol<uint64_t>(rs, 64 - n); \
    auto m = mask<64>(n, 63); \
    auto s = bit_test(rs, 64, 0); \
    auto ra = (r & m) | ((s ? -1ull : 0ull) & ~m); \
    auto ca = s & ((r & ~m) != 0); \
    TH->setGPR(_ra, ra); \
    TH->setCA(ca); \
    if (_rc) \
        update_CR0(ra, TH); \
}
EMU_REWRITE(SRADI, XSForm, getNBE(i->sh04(), i->sh5()), i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(SRAWI, XForm_10) {
    *result = format_nnn(i->Rc_u() ? "srawi." : "srawi", i->RA(), i->RS(), i->SH());
}

#define _SRAWI(_shu, _rs, _ra, _rc) { \
    auto n =  _shu; \
    auto rs = TH->getGPR(_rs) & 0xffffffff; \
    uint64_t r = rol<uint32_t>(rs, 64 - n); \
    auto m = mask<64>(n + 32, 63); \
    auto s = bit_test(rs, 64, 32); \
    auto ra = (r & m) | ((s ? -1ull : 0ull) & ~m); \
    auto ca = s & ((r & ~m) != 0); \
    TH->setGPR(_ra, ra); \
    TH->setCA(ca); \
    if (_rc) \
        update_CR0(ra, TH); \
}
EMU_REWRITE(SRAWI, XForm_10, i->SH_u(), i->RS_u(), i->RA_u(), i->Rc_u())

PRINT(SRAD, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "srad." : "srad", i->RA(), i->RS(), i->RB());
}

#define _SRAD(_rb, _rs, _ra, _rc) { \
    auto rb = TH->getGPR(_rb); \
    auto n = rb & 0b111111; \
    auto rs = TH->getGPR(_rs); \
    auto r = rol<uint64_t>(rs, 64 - n); \
    uint64_t m = (rb & 0b1000000) == 0 ? mask<64>(n, 63) : 0; \
    auto s = bit_test(rs, 64, 0); \
    auto ra = (r & m) | ((s ? -1ull : 0ull) & ~m); \
    auto ca = s & (((r & ~m)) != 0); \
    TH->setGPR(_ra, ra); \
    TH->setCA(ca); \
    if (_rc) \
        update_CR0(ra, TH); \
}
EMU_REWRITE(SRAD, XForm_6, i->RB_u(), i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(SRAW, XForm_6) {
    *result = format_nnn(i->Rc_u() ? "sraw." : "sraw", i->RA(), i->RS(), i->RB());
}

#define _SRAW(_rb, _rs, _ra, _rc) { \
    auto rb = TH->getGPR(_rb); \
    auto n = rb & 0b11111; \
    auto rs = TH->getGPR(_rs) & 0xffffffff; \
    auto r = rol<uint32_t>(rs, 64 - n); \
    uint64_t m = (rb & 0b100000) == 0 ? mask<64>(n + 32, 63) : 0; \
    auto s = bit_test(rs, 64, 32); \
    auto ra = (r & m) | ((s ? -1ull : 0ull) & ~m); \
    auto ca = s & (((r & ~m) & 0xffffffff) != 0); \
    TH->setGPR(_ra, ra); \
    TH->setCA(ca); \
    if (_rc) \
        update_CR0(ra, TH); \
}
EMU_REWRITE(SRAW, XForm_6, i->RB_u(), i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(NEG, XOForm_3) {
    const char* mnemonics[][2] = {
        { "neg", "neg." }, { "nego", "nego." }
    };
    *result = format_nn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA());
}

#define _NEG(_ra, _rt, _oe, _rc) { \
    auto ra = TH->getGPR(_ra); \
    auto ov = ra == (1ull << 63); \
    auto res = ov ? ra : (~ra + 1); \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ov, res, TH); \
}
EMU_REWRITE(NEG, XOForm_3, i->RA_u(), i->RT_u(), i->OE_u(), i->Rc_u())


PRINT(DIVD, XOForm_1) {
    const char* mnemonics[][2] = {
        { "divd", "divd." }, { "divdo", "divdo." }
    };
    *result = format_nnn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA(), i->RB());
}

#define _DIVD(_ra, _rb, _rt, _oe, _rc) { \
    int64_t dividend = TH->getGPR(_ra); \
    int64_t divisor = TH->getGPR(_rb); \
    auto ov = divisor == 0 || ((uint64_t)dividend == 0x8000000000000000 && divisor == -1ll); \
    int64_t res = ov ? 0 : dividend / divisor; \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ov, res, TH); \
}
EMU_REWRITE(DIVD, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->OE_u(), i->Rc_u())


PRINT(DIVW, XOForm_1) {
    const char* mnemonics[][2] = {
        { "divw", "divw." }, { "divwo", "divwo." }
    };
    *result = format_nnn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA(), i->RB());
}

#define _DIVW(_ra, _rb, _rt, _oe, _rc) { \
    int64_t dividend = (int32_t)(TH->getGPR(_ra) & 0xffffffff); \
    int64_t divisor = (int32_t)(TH->getGPR(_rb) & 0xffffffff); \
    auto ov = divisor == 0 || ((uint64_t)dividend == 0x80000000 && divisor == -1l); \
    int64_t res = ov ? 0 : dividend / divisor; \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ov, res, TH); \
}
EMU_REWRITE(DIVW, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->OE_u(), i->Rc_u())


PRINT(DIVDU, XOForm_1) {
    const char* mnemonics[][2] = {
        { "divdu", "divdu." }, { "divduo", "divduo." }
    };
    *result = format_nnn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA(), i->RB());
}

#define _DIVDU(_ra, _rb, _rt, _oe, _rc) { \
    auto dividend = TH->getGPR(_ra); \
    auto divisor = TH->getGPR(_rb); \
    auto ov = divisor == 0; \
    auto res = ov ? 0 : dividend / divisor; \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ov, res, TH); \
}
EMU_REWRITE(DIVDU, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->OE_u(), i->Rc_u())


PRINT(DIVWU, XOForm_1) {
    const char* mnemonics[][2] = {
        { "divwu", "divwu." }, { "divwuo", "divwuo." }
    };
    *result = format_nnn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA(), i->RB());
}

#define _DIVWU(_ra, _rb, _rt, _oe, _rc) { \
    auto dividend = static_cast<uint32_t>(TH->getGPR(_ra)); \
    auto divisor = static_cast<uint32_t>(TH->getGPR(_rb)); \
    auto ov = divisor == 0; \
    auto res = ov ? 0 : dividend / divisor; \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ov, res, TH); \
}
EMU_REWRITE(DIVWU, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->OE_u(), i->Rc_u())


PRINT(MULLD, XOForm_1) {
    const char* mnemonics[][2] = {
        { "mulld", "mulld." }, { "mulldo", "mulldo." }
    };
    *result = format_nnn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA(), i->RB());
}

#define _MULLD(_ra, _rb, _rt, _oe, _rc) { \
    auto a = TH->getGPR(_ra); \
    auto b = TH->getGPR(_rb); \
    unsigned __int128 res = a * b; \
    auto ov = res > 0xffffffffffffffff; \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ov, res, TH); \
}
EMU_REWRITE(MULLD, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->OE_u(), i->Rc_u())



PRINT(MULLW, XOForm_1) {
    const char* mnemonics[][2] = {
        { "mullw", "mullw." }, { "mullwo", "mullwo." }
    };
    *result = format_nnn(mnemonics[i->OE_u()][i->Rc_u()], i->RT(), i->RA(), i->RB());
}

#define _MULLW(_ra, _rb, _rt, _oe, _rc) { \
    auto res = (TH->getGPR(_ra) & 0xffffffff) \
             * (TH->getGPR(_rb) & 0xffffffff); \
    auto ov = res > 0xffffffff; \
    TH->setGPR(_rt, res); \
    update_CR0_OV(_oe, _rc, ov, res, TH); \
}
EMU_REWRITE(MULLW, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->OE_u(), i->Rc_u())


PRINT(MULHD, XOForm_1) {
    *result = format_nnn(i->Rc_u() ? "mulhd." : "mulhd", i->RT(), i->RA(), i->RB());
}

#define _MULHD(_ra, _rb, _rt, _rc) { \
    unsigned __int128 prod = (__int128)TH->getGPR(_ra) * TH->getGPR(_rb); \
    auto res = prod >> 64; \
    TH->setGPR(_rt, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(MULHD, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->Rc_u())


PRINT(MULHW, XOForm_1) {
    *result = format_nnn(i->Rc_u() ? "mulhw." : "mulhw", i->RT(), i->RA(), i->RB());
}

#define _MULHW(_ra, _rb, _rt, _rc) { \
    int64_t prod = (int64_t)(int32_t)TH->getGPR(_ra) \
                 * (int64_t)(int32_t)TH->getGPR(_rb); \
    auto res = (int64_t)((uint64_t)prod >> 32); \
    TH->setGPR(_rt, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(MULHW, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->Rc_u())


PRINT(MULHWU, XOForm_1) {
    *result = format_nnn(i->Rc_u() ? "mulhwu." : "mulhwu", i->RT(), i->RA(), i->RB());
}

#define _MULHWU(_ra, _rb, _rt, _rc) { \
    uint64_t prod = (uint64_t)(uint32_t)TH->getGPR(_ra) \
                  * (uint64_t)(uint32_t)TH->getGPR(_rb); \
    auto res = (uint64_t)((uint64_t)prod >> 32); \
    TH->setGPR(_rt, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(MULHWU, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->Rc_u())


PRINT(MULHDU, XOForm_1) {
    *result = format_nnn(i->Rc_u() ? "mulhdu." : "mulhdu", i->RT(), i->RA(), i->RB());
}

#define _MULHDU(_ra, _rb, _rt, _rc) { \
    unsigned __int128 prod = (unsigned __int128)TH->getGPR(_ra) * \
                             (unsigned __int128)TH->getGPR(_rb); \
    auto res = prod >> 64; \
    TH->setGPR(_rt, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(MULHDU, XOForm_1, i->RA_u(), i->RB_u(), i->RT_u(), i->Rc_u())


PRINT(MULLI, DForm_2) {
    *result = format_nnn("mulli", i->RT(), i->RA(), i->SI());
}

#define _MULLI(_ra, _sis, _rt) { \
    __int128 a = (int64_t)TH->getGPR(_ra); \
    auto prod = a * _sis; \
    TH->setGPR(_rt, prod); \
}
EMU_REWRITE(MULLI, DForm_2, i->RA_u(), i->SI_s(), i->RT_u())


PRINT(MTOCRF, XFXForm_6) {
    *result = format_nn("mtocrf", i->FXM(), i->RS());
}

#define _MTOCRF(_fxm, _rs) { \
    auto fxm = _fxm; \
    auto n = fxm & 128 ? 0 \
           : fxm & 64 ? 1 \
           : fxm & 32 ? 2 \
           : fxm & 16 ? 3 \
           : fxm & 8 ? 4 \
           : fxm & 4 ? 5 \
           : fxm & 2 ? 6 \
           : 7; \
    auto cr = TH->getCR() & ~mask<32>(4*n, 4*n + 3); \
    auto rs = TH->getGPR(_rs) & mask<32>(4*n, 4*n + 3); \
    TH->setCR(cr | rs); \
}
EMU_REWRITE(MTOCRF, XFXForm_6, i->FXM_u(), i->RS_u())


PRINT(DCBT, XForm_31) {
    *result = format_nn("dcbt", i->RA(), i->RB());
}

#define _DCBT(_) { \
}
EMU_REWRITE(DCBT, XForm_31, 0)


PRINT(CNTLZD, XForm_11) {
    *result = format_nn(i->Rc_u() ? "cntlzd." : "cntlzd", i->RA(), i->RS());
}

#define _CNTLZD(_rs, _ra, _rc) { \
    static_assert(sizeof(unsigned long long int) == 8, ""); \
    auto rs = TH->getGPR(_rs); \
    auto res = !rs ? 64 : __builtin_clzll(rs); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(CNTLZD, XForm_11, i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(CNTLZW, XForm_11) {
    *result = format_nn(i->Rc_u() ? "cntlzw." : "cntlzw", i->RA(), i->RS());
}

#define _CNTLZW(_rs, _ra, _rc) { \
    static_assert(sizeof(unsigned int) == 4, ""); \
    uint32_t rs = TH->getGPR(_rs); \
    auto res = !rs ? 32 : __builtin_clz(rs); \
    TH->setGPR(_ra, res); \
    if (_rc) \
        update_CR0(res, TH); \
}
EMU_REWRITE(CNTLZW, XForm_11, i->RS_u(), i->RA_u(), i->Rc_u())


PRINT(LFS, DForm_8) {
    *result = format_br_nnn("lfs", i->FRT(), i->D(), i->RA());
}

#define _LFS(_ra, _ds, _frt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    TH->setFPRd(_frt, MM->loadf(ea)); \
}
EMU_REWRITE(LFS, DForm_8, i->RA_u(), i->D_s(), i->FRT_u())


PRINT(LFSX, XForm_26) {
    *result = format_nnn("lfsx", i->FRT(), i->RA(), i->RB());
}

#define _LFSX(_ra, _rb, _frt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    TH->setFPRd(_frt, MM->loadf(ea)); \
}
EMU_REWRITE(LFSX, XForm_26, i->RA_u(), i->RB_u(), i->FRT_u())


PRINT(LFSU, DForm_8) {
    *result = format_br_nnn("lfsu", i->FRT(), i->D(), i->RA());
}

#define _LFSU(_ds, _ra, _frt) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    TH->setFPRd(_frt, MM->loadf(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LFSU, DForm_8, i->D_s(), i->RA_u(), i->FRT_u())


PRINT(LFSUX, XForm_26) {
    *result = format_nnn("lfsux", i->FRT(), i->RA(), i->RB());
}

#define _LFSUX(_ra, _rb, _frt) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    TH->setFPRd(_frt, MM->loadf(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LFSUX, XForm_26, i->RA_u(), i->RB_u(), i->FRT_u())


PRINT(LFD, DForm_8) {
    *result = format_br_nnn("lfd", i->FRT(), i->D(), i->RA());
}

#define _LFD(_ra, _ds, _frt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    TH->setFPRd(_frt, MM->loadd(ea)); \
}
EMU_REWRITE(LFD, DForm_8, i->RA_u(), i->D_s(), i->FRT_u())


PRINT(LFDX, XForm_26) {
    *result = format_nnn("lfdx", i->FRT(), i->RA(), i->RB());
}

#define _LFDX(_ra, _rb, _frt) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    TH->setFPRd(_frt, MM->loadd(ea)); \
}
EMU_REWRITE(LFDX, XForm_26, i->RA_u(), i->RB_u(), i->FRT_u())


PRINT(LFDU, DForm_8) {
    *result = format_br_nnn("lfdu", i->FRT(), i->D(), i->RA());
}

#define _LFDU(_ds, _ra, _frt) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    TH->setFPRd(_frt, MM->loadd(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LFDU, DForm_8, i->D_s(), i->RA_u(), i->FRT_u())


PRINT(LFDUX, XForm_26) {
    *result = format_nnn("lfdux", i->FRT(), i->RA(), i->RB());
}

#define _LFDUX(_ra, _rb, _frt) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    TH->setFPRd(_frt, MM->loadd(ea)); \
    TH->setGPR(_ra, ea); \
}
EMU_REWRITE(LFDUX, XForm_26, i->RA_u(), i->RB_u(), i->FRT_u())


PRINT(STFS, DForm_9) {
    *result = format_br_nnn("stfs", i->FRS(), i->D(), i->RA());
}

#define _STFS(_ra, _ds, _frs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    auto frs = TH->getFPRd(_frs); \
    MM->storef(ea, frs, TH->granule()); \
}
EMU_REWRITE(STFS, DForm_9, i->RA_u(), i->D_s(), i->FRS_u())


PRINT(STFSX, XForm_29) {
    *result = format_br_nnn("stfsx", i->FRS(), i->RA(), i->RB());
}

#define _STFSX(_ra, _rb, _frs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    auto frs = TH->getFPRd(_frs); \
    MM->storef(ea, frs, TH->granule()); \
}
EMU_REWRITE(STFSX, XForm_29, i->RA_u(), i->RB_u(), i->FRS_u())


PRINT(STFSU, DForm_9) {
    *result = format_br_nnn("stfsu", i->FRS(), i->D(), i->RA());
}

#define _STFSU(_ds, _ra, _frs) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    auto frs = TH->getFPRd(_frs); \
    TH->setGPR(_ra, ea); \
    MM->storef(ea, frs, TH->granule()); \
}
EMU_REWRITE(STFSU, DForm_9, i->D_s(), i->RA_u(), i->FRS_u())


PRINT(STFSUX, XForm_29) {
    *result = format_br_nnn("sftsux", i->FRS(), i->RA(), i->RB());
}

#define _STFSUX(_ra, _rb, _frs) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    auto frs = TH->getFPRd(_frs); \
    TH->setGPR(_ra, ea); \
    MM->storef(ea, frs, TH->granule()); \
}
EMU_REWRITE(STFSUX, XForm_29, i->RA_u(), i->RB_u(), i->FRS_u())


PRINT(STFD, DForm_9) {
    *result = format_br_nnn("stfd", i->FRS(), i->D(), i->RA());
}

#define _STFD(_ra, _ds, _frs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + _ds; \
    auto frs = TH->getFPRd(_frs); \
    MM->stored(ea, frs, TH->granule()); \
}
EMU_REWRITE(STFD, DForm_9, i->RA_u(), i->D_s(), i->FRS_u())


PRINT(STFDX, XForm_29) {
    *result = format_br_nnn("stfdx", i->FRS(), i->RA(), i->RB());
}

#define _STFDX(_ra, _rb, _frs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    auto frs = TH->getFPRd(_frs); \
    MM->stored(ea, frs, TH->granule()); \
}
EMU_REWRITE(STFDX, XForm_29, i->RA_u(), i->RB_u(), i->FRS_u())


PRINT(STFDU, DForm_9) {
    *result = format_br_nnn("stfdu", i->FRS(), i->D(), i->RA());
}

#define _STFDU(_ds, _ra, _frs) { \
    auto ea = TH->getGPR(_ra) + _ds; \
    auto frs = TH->getFPRd(_frs); \
    TH->setGPR(_ra, ea); \
    MM->stored(ea, frs, TH->granule()); \
}
EMU_REWRITE(STFDU, DForm_9, i->D_s(), i->RA_u(), i->FRS_u())


PRINT(STFDUX, XForm_29) {
    *result = format_br_nnn("sftdux", i->FRS(), i->RA(), i->RB());
}

#define _STFDUX(_ra, _rb, _frs) { \
    auto ea = TH->getGPR(_ra) + TH->getGPR(_rb); \
    auto frs = TH->getFPRd(_frs); \
    TH->setGPR(_ra, ea); \
    MM->stored(ea, frs, TH->granule()); \
}
EMU_REWRITE(STFDUX, XForm_29, i->RA_u(), i->RB_u(), i->FRS_u())


PRINT(STFIWX, XForm_29) {
    *result = format_br_nnn("stfiwx", i->FRS(), i->RA(), i->RB());
}

#define _STFIWX(_ra, _rb, _frs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    auto frs = (uint32_t)TH->getFPR(_frs); \
    MM->store32(ea, frs, TH->granule()); \
}
EMU_REWRITE(STFIWX, XForm_29, i->RA_u(), i->RB_u(), i->FRS_u())


PRINT(FMR, XForm_27) {
    *result = format_nn(i->Rc_u() ? "fmr." : "fmr", i->FRT(), i->FRB());
}

#define _FMR(_frb, _frt, _rc) { \
    auto res = TH->getFPRd(_frb); \
    TH->setFPRd(_frt, res); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FMR, XForm_27, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FNEG, XForm_27) {
    *result = format_nn(i->Rc_u() ? "fneg." : "fneg", i->FRT(), i->FRB());
}

#define _FNEG(_frb, _frt, _rc) { \
    auto res = -TH->getFPRd(_frb); \
    TH->setFPRd(_frt, res); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FNEG, XForm_27, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FABS, XForm_27) {
    *result = format_nn(i->Rc_u() ? "fabs." : "fabs", i->FRT(), i->FRB());
}

#define _FABS(_frb, _frt, _rc) { \
    auto res = std::abs(TH->getFPRd(_frb)); \
    TH->setFPRd(_frt, res); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FABS, XForm_27, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FNABS, XForm_27) {
    *result = format_nn(i->Rc_u() ? "fnabs." : "fnabs", i->FRT(), i->FRB());
}

#define _FNABS(_frb, _frt, _rc) { \
    auto res = -std::abs(TH->getFPRd(_frb)); \
    TH->setFPRd(_frt, res); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FNABS, XForm_27, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FSQRT, AForm_4) {
    *result = format_nn(i->Rc_u() ? "fsqrt." : "fsqrt", i->FRT(), i->FRB());
}

#define _FSQRT(_frb, _frt, _rc) { \
    auto rb = TH->getFPRd(_frb); \
    auto res = sqrt(rb); \
    TH->setFPRd(_frt, res); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FSQRT, AForm_4, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FSQRTS, AForm_4) {
    *result = format_nn(i->Rc_u() ? "fsqrts." : "fsqrts", i->FRT(), i->FRB());
}

#define _FSQRTS(_frb, _frt, _rc) { \
    float rb = TH->getFPRd(_frb); \
    auto res = sqrt(rb); \
    TH->setFPRd(_frt, res); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FSQRTS, AForm_4, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FRE, AForm_4) {
    *result = format_nn(i->Rc_u() ? "fre." : "fre", i->FRT(), i->FRB());
}

#define _FRE(_frb, _frt, _rc) { \
    auto rb = TH->getFPRd(_frb); \
    auto res = 1. / rb; \
    TH->setFPRd(_frt, res); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FRE, AForm_4, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FRES, AForm_4) {
    *result = format_nn(i->Rc_u() ? "fres." : "fres", i->FRT(), i->FRB());
}

#define _FRES(_frb, _frt, _rc) { \
    float rb = TH->getFPRd(_frb); \
    auto res = 1.f / rb; \
    TH->setFPRd(_frt, res); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FRES, AForm_4, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FRSQRTE, AForm_4) {
    *result = format_nn(i->Rc_u() ? "frsqrte." : "frsqrte", i->FRT(), i->FRB());
}

#define _FRSQRTE(_frb, _frt, _rc) { \
    auto rb = TH->getFPRd(_frb); \
    auto res = 1. / sqrt(rb); \
    TH->setFPRd(_frt, res); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FRSQRTE, AForm_4, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FRSQRTES, AForm_4) {
    *result = format_nn(i->Rc_u() ? "frsqrtes." : "frsqrtes", i->FRT(), i->FRB());
}

#define _FRSQRTES(_frb, _frt, _rc) { \
    float rb = TH->getFPRd(_frb); \
    auto res = 1.f / sqrt(rb); \
    TH->setFPRd(_frt, res); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FRSQRTES, AForm_4, i->FRB_u(), i->FRT_u(), i->Rc_u())

PRINT(FSEL, AForm_1) {
    *result = format_nnnn(i->Rc_u() ? "fsel." : "fsel", i->FRT(), i->FRA(), i->FRC(), i->FRB());
}

#define _FSEL(_frt, _fra, _frc, _frb, _rc) { \
    auto ra = TH->getFPRd(_fra); \
    auto rc = TH->getFPRd(_frc); \
    auto rb = TH->getFPRd(_frb); \
    auto res = ra > 0.f ? rc : rb; \
    TH->setFPRd(_frt, res); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FSEL, AForm_1, i->FRT_u(), i->FRA_u(), i->FRC_u(), i->FRB_u(), i->Rc_u())

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

template <typename A, typename B>
void completeFPInstr(
    A a, B b, unsigned c, unsigned r, unsigned rc, PPUThread* thread) {
#ifndef EMU_REWRITER_NOFEXCEPT
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
    if (rc)
        update_CRFSign<1>(r, TH);
#endif
}

PRINT(FADD, AForm_2) {
    *result = format_nnn(i->Rc_u() ? "fadd." : "fadd", i->FRT(), i->FRA(), i->FRB());
}

#define _FADD(_fra, _frb, _frt, _rc) { \
    auto a = TH->getFPRd(_fra); \
    auto b = TH->getFPRd(_frb); \
    FECLEAR_EXCEPT; \
    auto r = a + b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, .0, r, _rc, TH); \
}
EMU_REWRITE(FADD, AForm_2, i->FRA_u(), i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FSUB, AForm_2) {
    *result = format_nnn(i->Rc_u() ? "fsub." : "fsub", i->FRT(), i->FRA(), i->FRB());
}

#define _FSUB(_fra, _frb, _frt, _rc) { \
    auto a = TH->getFPRd(_fra); \
    auto b = TH->getFPRd(_frb); \
    FECLEAR_EXCEPT; \
    auto r = a - b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, .0, r, _rc, TH); \
}
EMU_REWRITE(FSUB, AForm_2, i->FRA_u(), i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FSUBS, AForm_2) {
    *result = format_nnn(i->Rc_u() ? "fsubs." : "fsubs", i->FRT(), i->FRA(), i->FRB());
}

#define _FSUBS(_fra, _frb, _frt, _rc) { \
    auto a = TH->getFPRd(_fra); \
    auto b = TH->getFPRd(_frb); \
    FECLEAR_EXCEPT; \
    auto r = a - b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, .0, r, _rc, TH); \
}
EMU_REWRITE(FSUBS, AForm_2, i->FRA_u(), i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FADDS, AForm_2) {
    *result = format_nnn(i->Rc_u() ? "fadds." : "fadds", i->FRT(), i->FRA(), i->FRB());
}

#define _FADDS(_fra, _frb, _frt, _rc) { \
    float a = TH->getFPRd(_fra); \
    float b = TH->getFPRd(_frb); \
    FECLEAR_EXCEPT; \
    auto r = a + b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, .0f, r, _rc, TH); \
}
EMU_REWRITE(FADDS, AForm_2, i->FRA_u(), i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FMUL, AForm_3) {
    *result = format_nnn(i->Rc_u() ? "fmul." : "fmul", i->FRT(), i->FRA(), i->FRC());
}

#define _FMUL(_fra, _frc, _frt, _rc) { \
    auto a = TH->getFPRd(_fra); \
    auto b = TH->getFPRd(_frc); \
    FECLEAR_EXCEPT; \
    auto r = a * b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, .0, r, _rc, TH); \
}
EMU_REWRITE(FMUL, AForm_3, i->FRA_u(), i->FRC_u(), i->FRT_u(), i->Rc_u())


PRINT(FMULS, AForm_3) {
    *result = format_nnn(i->Rc_u() ? "fmuls." : "fmuls", i->FRT(), i->FRA(), i->FRC());
}

#define _FMULS(_fra, _frc, _frt, _rc) { \
    float a = TH->getFPRd(_fra); \
    float b = TH->getFPRd(_frc); \
    FECLEAR_EXCEPT; \
    auto r = a * b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, .0f, r, _rc, TH); \
}
EMU_REWRITE(FMULS, AForm_3, i->FRA_u(), i->FRC_u(), i->FRT_u(), i->Rc_u())


PRINT(FDIV, AForm_2) {
    *result = format_nnn(i->Rc_u() ? "fdiv." : "fdiv", i->FRT(), i->FRA(), i->FRB());
}

#define _FDIV(_fra, _frb, _frt, _rc) { \
    auto a = TH->getFPRd(_fra); \
    auto b = TH->getFPRd(_frb); \
    FECLEAR_EXCEPT; \
    auto r = a / b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, .0, r, _rc, TH); \
}
EMU_REWRITE(FDIV, AForm_2, i->FRA_u(), i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FDIVS, AForm_2) {
    *result = format_nnn(i->Rc_u() ? "fdivs." : "fdivs", i->FRT(), i->FRA(), i->FRB());
}

#define _FDIVS(_fra, _frb, _frt, _rc) { \
    float a = TH->getFPRd(_fra); \
    float b = TH->getFPRd(_frb); \
    FECLEAR_EXCEPT; \
    auto r = a / b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, .0f, r, _rc, TH); \
}
EMU_REWRITE(FDIVS, AForm_2, i->FRA_u(), i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FMADD, AForm_1) {
    *result = format_nnnn(i->Rc_u() ? "fmadd." : "fmadd", i->FRT(), i->FRA(), i->FRC(), i->FRB());
}

#define _FMADD(_fra, _frb, _frc, _frt, _rc) { \
    auto a = TH->getFPRd(_fra); \
    auto b = TH->getFPRd(_frb); \
    auto c = TH->getFPRd(_frc); \
    FECLEAR_EXCEPT; \
    auto r = a * c + b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, c, r, _rc, TH); \
}
EMU_REWRITE(FMADD, AForm_1, i->FRA_u(), i->FRB_u(), i->FRC_u(), i->FRT_u(), i->Rc_u())


PRINT(FMADDS, AForm_1) {
    *result = format_nnnn(i->Rc_u() ? "fmadds." : "fmadds", i->FRT(), i->FRA(), i->FRC(), i->FRB());
}

#define _FMADDS(_fra, _frb, _frc, _frt, _rc) { \
    float a = TH->getFPRd(_fra); \
    float b = TH->getFPRd(_frb); \
    float c = TH->getFPRd(_frc); \
    FECLEAR_EXCEPT; \
    auto r = a * c + b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, c, r, _rc, TH); \
}
EMU_REWRITE(FMADDS, AForm_1, i->FRA_u(), i->FRB_u(), i->FRC_u(), i->FRT_u(), i->Rc_u())


PRINT(FMSUB, AForm_1) {
    *result = format_nnnn(i->Rc_u() ? "fsub." : "fmsub", i->FRT(), i->FRA(), i->FRC(), i->FRB());
}

#define _FMSUB(_fra, _frb, _frc, _frt, _rc) { \
    double a = TH->getFPRd(_fra); \
    double b = TH->getFPRd(_frb); \
    double c = TH->getFPRd(_frc); \
    FECLEAR_EXCEPT; \
    auto r = a * c - b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, c, r, _rc, TH); \
}
EMU_REWRITE(FMSUB, AForm_1, i->FRA_u(), i->FRB_u(), i->FRC_u(), i->FRT_u(), i->Rc_u())



PRINT(FMSUBS, AForm_1) {
    *result = format_nnnn(i->Rc_u() ? "fsubs." : "fmsubs", i->FRT(), i->FRA(), i->FRC(), i->FRB());
}

#define _FMSUBS(_fra, _frb, _frc, _frt, _rc) { \
    float a = TH->getFPRd(_fra); \
    float b = TH->getFPRd(_frb); \
    float c = TH->getFPRd(_frc); \
    FECLEAR_EXCEPT; \
    auto r = a * c - b; \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, c, r, _rc, TH); \
}
EMU_REWRITE(FMSUBS, AForm_1, i->FRA_u(), i->FRB_u(), i->FRC_u(), i->FRT_u(), i->Rc_u())


PRINT(FNMADD, AForm_1) {
    *result = format_nnnn(i->Rc_u() ? "fnmadd." : "fnmadd", i->FRT(), i->FRA(), i->FRC(), i->FRB());
}

#define _FNMADD(_fra, _frb, _frc, _frt, _rc) { \
    auto a = TH->getFPRd(_fra); \
    auto b = TH->getFPRd(_frb); \
    auto c = TH->getFPRd(_frc); \
    FECLEAR_EXCEPT; \
    auto r = -(a * c + b); \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, c, r, _rc, TH); \
}
EMU_REWRITE(FNMADD, AForm_1, i->FRA_u(), i->FRB_u(), i->FRC_u(), i->FRT_u(), i->Rc_u())


PRINT(FNMADDS, AForm_1) {
    *result = format_nnnn(i->Rc_u() ? "fnmadds." : "fnmadds", i->FRT(), i->FRA(), i->FRC(), i->FRB());
}

#define _FNMADDS(_fra, _frb, _frc, _frt, _rc) { \
    float a = TH->getFPRd(_fra); \
    float b = TH->getFPRd(_frb); \
    float c = TH->getFPRd(_frc); \
    FECLEAR_EXCEPT; \
    auto r = -(a * c + b); \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, c, r, _rc, TH); \
}
EMU_REWRITE(FNMADDS, AForm_1, i->FRA_u(), i->FRB_u(), i->FRC_u(), i->FRT_u(), i->Rc_u())


PRINT(FNMSUB, AForm_1) {
    *result = format_nnnn(i->Rc_u() ? "fnsub." : "fnmsub", i->FRT(), i->FRA(), i->FRC(), i->FRB());
}

#define _FNMSUB(_fra, _frb, _frc, _frt, _rc) { \
    double a = TH->getFPRd(_fra); \
    double b = TH->getFPRd(_frb); \
    double c = TH->getFPRd(_frc); \
    FECLEAR_EXCEPT; \
    auto r = -(a * c - b); \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, c, r, _rc, TH); \
}
EMU_REWRITE(FNMSUB, AForm_1, i->FRA_u(), i->FRB_u(), i->FRC_u(), i->FRT_u(), i->Rc_u())



PRINT(FNMSUBS, AForm_1) {
    *result = format_nnnn(i->Rc_u() ? "fnsubs." : "fnmsubs", i->FRT(), i->FRA(), i->FRC(), i->FRB());
}

#define _FNMSUBS(_fra, _frb, _frc, _frt, _rc) { \
    float a = TH->getFPRd(_fra); \
    float b = TH->getFPRd(_frb); \
    float c = TH->getFPRd(_frc); \
    FECLEAR_EXCEPT; \
    auto r = -(a * c - b); \
    TH->setFPRd(_frt, r); \
    completeFPInstr(a, b, c, r, _rc, TH); \
}
EMU_REWRITE(FNMSUBS, AForm_1, i->FRA_u(), i->FRB_u(), i->FRC_u(), i->FRT_u(), i->Rc_u())


PRINT(FCFID, XForm_27) {
    *result = format_nn(i->Rc_u() ? "fcfid." : "fcfid", i->FRT(), i->FRB());
}

#define _FCFID(_frb, _frt, _rc) { \
    double b = (int64_t)TH->getFPR(_frb); \
    TH->setFPRd(_frt, b); \
    auto fpscr = TH->getFPSCR(); \
    fpscr.fprf.v = getFPRF(b); \
    TH->setFPSCR(fpscr.v); \
    /* TODO: FX XX */ \
    if (_rc) \
        update_CRFSign<1>(b, TH); \
}
EMU_REWRITE(FCFID, XForm_27, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FCMPU, XForm_17) {
    *result = format_nnn("fcmpu", i->BF(), i->FRA(), i->FRB());
}

#define _FCMPU(_fra, _frb, _bf) { \
    auto a = TH->getFPRd(_fra); \
    auto b = TH->getFPRd(_frb); \
    uint32_t c = std::isnan(a) || std::isnan(b) ? 1 \
               : a < b ? 8 \
               : a > b ? 4 \
               : 2; \
    auto fpscr = TH->getFPSCR(); \
    fpscr.fpcc.v = c; \
    TH->setCRF(_bf, c); \
    if (issignaling(a) || issignaling(b)) { \
        fpscr.f.FX |= 1; \
        fpscr.f.VXSNAN = 1; \
    } \
    TH->setFPSCR(fpscr.v); \
}
EMU_REWRITE(FCMPU, XForm_17, i->FRA_u(), i->FRB_u(), i->BF_u())


PRINT(FCTIWZ, XForm_27) {
    *result = format_nn(i->Rc_u() ? "fctiwz." : "fctiwz", i->FRT(), i->FRB());
}

#define _FCTIWZ(_frb, _frt, _rc) { \
    auto b = TH->getFPRd(_frb); \
    int32_t res = b > 2147483647. ? 0x7fffffff \
                : b < -2147483648. ? 0x80000000 \
                : (int32_t)b; \
    /* TODO invalid operation, XX */ \
    auto fpscr = TH->getFPSCR(); \
    if (issignaling(b)) { \
        fpscr.f.FX = 1; \
        fpscr.f.VXSNAN = 1; \
    } \
    TH->setFPR(_frt, res); \
    TH->setFPSCR(fpscr.v); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FCTIWZ, XForm_27, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FCTIDZ, XForm_27) { // test, change rounding mode
    *result = format_nn(i->Rc_u() ? "fctidz." : "fctidz", i->FRT(), i->FRB());
}

#define _FCTIDZ(_frb, _frt, _rc) { \
    auto b = TH->getFPRd(_frb); \
    int32_t res = b > 2147483647. ? 0x7fffffff \
    : b < -2147483648. ? 0x80000000 \
    : (int32_t)b; \
    /* TODO invalid operation, XX */ \
    auto fpscr = TH->getFPSCR(); \
    if (issignaling(b)) { \
        fpscr.f.FX = 1; \
        fpscr.f.VXSNAN = 1; \
    } \
    TH->setFPR(_frt, res); \
    TH->setFPSCR(fpscr.v); \
    if (_rc) \
        update_CRFSign<1>(res, TH); \
}
EMU_REWRITE(FCTIDZ, XForm_27, i->FRB_u(), i->FRT_u(), i->Rc_u())


PRINT(FRSP, XForm_27) {
    *result = format_nn(i->Rc_u() ? "frsp." : "frsp", i->FRT(), i->FRB());
}

#define _FRSP(_frb, _frt) { \
    /* TODO: set class etc */ \
    float b = TH->getFPRd(_frb); \
    TH->setFPRd(_frt, b); \
}
EMU_REWRITE(FRSP, XForm_27, i->FRB_u(), i->FRT_u())


PRINT(MFFS, XForm_28) {
    *result = format_n(i->Rc_u() ? "mffs." : "mffs", i->FRT());
}

#define _MFFS(_frt, _rc) { \
    uint32_t fpscr = TH->getFPSCR().v; \
    TH->setFPR(_frt, fpscr); \
    if (_rc) \
        update_CRFSign<1>(fpscr, TH); \
}
EMU_REWRITE(MFFS, XForm_28, i->FRT_u(), i->Rc_u())


PRINT(MTFSF, XFLForm) {
    *result = format_nn(i->Rc_u() ? "mtfsf." : "mtfsf", i->FLM(), i->FRB());
}

#define _MTFSF(_flm, _rc) { \
    auto n = __builtin_clzll(_flm) - 64 + XFLForm::FLM_t::W; \
    auto r = TH->getFPSCRF(n); \
    TH->setFPSCRF(n, r); \
    if (_rc) \
        update_CRFSign<1>(r, TH); \
}
EMU_REWRITE(MTFSF, XFLForm, i->FLM_u(), i->Rc_u())


PRINT(LWARX, XForm_1) {
    *result = format_nnn("lwarx", i->RT(), i->RA(), i->RB());
}

#define _LWARX(_ra, _rb, _rt) { \
    auto ra = getB(_ra, TH); \
    auto ea = ra + TH->getGPR(_rb); \
    big_uint32_t val; \
    MM->loadReserve<4>(ea, &val); \
    TH->setGPR(_rt, val); \
}
EMU_REWRITE(LWARX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


PRINT(STWCX, XForm_8) {
    *result = format_nnn("stwcx.", i->RS(), i->RA(), i->RB());
}

#define _STWCX(_ra, _rb, _rs) { \
    auto ra = getB(_ra, TH); \
    auto ea = ra + TH->getGPR(_rb); \
    big_uint32_t val = TH->getGPR(_rs); \
    auto stored = MM->writeCond<4>(ea, &val); \
    TH->setCRF_sign(0, stored); \
}
EMU_REWRITE(STWCX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())


PRINT(LDARX, XForm_1) {
    *result = format_nnn("ldarx", i->RT(), i->RA(), i->RB());
}

#define _LDARX(_ra, _rb, _rt) { \
    auto ra = getB(_ra, TH); \
    auto ea = ra + TH->getGPR(_rb); \
    big_uint64_t val; \
    MM->loadReserve<8>(ea, &val); \
    TH->setGPR(_rt, val); \
}
EMU_REWRITE(LDARX, XForm_1, i->RA_u(), i->RB_u(), i->RT_u())


PRINT(STDCX, XForm_8) {
    *result = format_nnn("stdcx.", i->RS(), i->RA(), i->RB());
}

#define _STDCX(_ra, _rb, _rs) { \
    auto ra = getB(_ra, TH); \
    auto ea = ra + TH->getGPR(_rb); \
    big_uint64_t val = TH->getGPR(_rs); \
    auto stored = MM->writeCond<8>(ea, &val); \
    TH->setCRF_sign(0, stored); \
}
EMU_REWRITE(STDCX, XForm_8, i->RA_u(), i->RB_u(), i->RS_u())


PRINT(SYNC, XForm_24) {
    *result = format_n("sync", i->L());
}

#define _SYNC(_) { \
    __sync_synchronize(); \
}
EMU_REWRITE(SYNC, XForm_24, 0)


PRINT(ISYNC, XLForm_1) {
    *result = "sync";
}

#define _ISYNC(_) { \
    __sync_synchronize(); \
}
EMU_REWRITE(ISYNC, XLForm_1, 0)


PRINT(EIEIO, XForm_24) {
    *result = "eieio";
}

#define _EIEIO(_) { \
    __sync_synchronize(); \
}
EMU_REWRITE(EIEIO, XForm_24, 0)


PRINT(TD, XForm_25) {
    *result = format_nnn("td", i->TO(), i->RA(), i->RB());
}

#define _TD(_to, _ra, _rb) { \
    if (_to == 31 && _ra == _rb) \
        throw BreakpointException(); \
    throw IllegalInstructionException(); \
}
EMU_REWRITE(TD, XForm_25, i->TO_u(), i->RA_u(), i->RB_u())


PRINT(TW, XForm_25) {
    *result = format_nnn("tw", i->TO(), i->RA(), i->RB());
}

#define _TW(_to, _ra, _rb) { \
    if (_to == 31 && _ra == _rb) \
        throw BreakpointException(); \
    throw IllegalInstructionException(); \
}
EMU_REWRITE(TW, XForm_25, i->TO_u(), i->RA_u(), i->RB_u())


PRINT(MFTB, XFXForm_2) {
    *result = format_nn("mftb", i->RT(), i->tbr());
}

#define _MFTB(_tbr, _rt) { \
    auto tbr = _tbr; \
    auto tb = g_state.proc->getTimeBase(); \
    assert(tbr == 392 || tbr == 424); \
    if (tbr == 424) \
        tb &= 0xffffffff; \
    TH->setGPR(_rt, tb); \
}
EMU_REWRITE(MFTB, XFXForm_2, i->tbr_u(), i->RT_u())


PRINT(STVX, SIMDForm) {
    *result = format_nnn("stvx", i->vS(), i->rA(), i->rB());
}

#define _STVX(_ra, _rb, _vs) { \
    auto b = getB(_ra, TH); \
    auto ea = (b + TH->getGPR(_rb)) & 0xfffffff0; \
    auto v = TH->r(_vs).xmm(); \
    MM->store128(ea, v, TH->granule()); \
}
EMU_REWRITE(STVX, SIMDForm, i->rA_u(), i->rB_u(), i->vS_u())


PRINT(STVXL, SIMDForm) {
    *result = format_nnn("stvxl", i->vS(), i->rA(), i->rB());
}

#define _STVXL(_ra, _rb, _vs) { \
    _STVX(_ra, _rb, _vs); \
}
EMU_REWRITE(STVXL, SIMDForm, i->rA_u(), i->rB_u(), i->vS_u())


PRINT(LVX, SIMDForm) {
    *result = format_nnn("lvx", i->vD(), i->rA(), i->rB());
}

#define _LVX(_ra, _rb, _vd) { \
    auto b = getB(_ra, TH); \
    auto ea = (b + TH->getGPR(_rb)) & 0xfffffff0; \
    auto xmm = MM->load128(ea); \
    TH->r(_vd).set_xmm(xmm); \
}
EMU_REWRITE(LVX, SIMDForm, i->rA_u(), i->rB_u(), i->vD_u())


PRINT(LVXL, SIMDForm) {
    *result = format_nnn("lvxl", i->vD(), i->rA(), i->rB());
}

#define _LVXL(_ra, _rb, _vs) { \
    _LVX(_ra, _rb, _vs); \
}
EMU_REWRITE(LVXL, SIMDForm, i->rA_u(), i->rB_u(), i->vD_u())


PRINT(LVLX, SIMDForm) {
    *result = format_nnn("lvlx", i->vD(), i->rA(), i->rB());
}

#define _LVLX(_ra, _rb, _vd) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    auto eb = ea & 0b1111ull; \
    __m128i v = _mm_setzero_si128(); \
    MM->readMemory(ea, &v, 16 - eb); \
    v = _mm_shuffle_epi8(v, ENDIAN_SWAP_MASK128); \
    TH->r(_vd).set_xmm(v); \
}
EMU_REWRITE(LVLX, SIMDForm, i->rA_u(), i->rB_u(), i->vD_u())


PRINT(LVRX, SIMDForm) {
    *result = format_nnn("lvrx", i->vD(), i->rA(), i->rB());
}

#define _LVRX(_ra, _rb, _vd) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    auto eb = ea & 0b1111ull; \
    __m128i v = _mm_setzero_si128(); \
    if (eb) { \
        MM->readMemory(ea - eb, (char*)&v + 16 - eb, eb); \
    } \
    v = _mm_shuffle_epi8(v, ENDIAN_SWAP_MASK128); \
    TH->r(_vd).set_xmm(v); \
}
EMU_REWRITE(LVRX, SIMDForm, i->rA_u(), i->rB_u(), i->vD_u())


PRINT(VSLDOI, SIMDForm) {
    *result = format_nnnn("vsldoi", i->vD(), i->vA(), i->vB(), i->SHB());
}

#ifdef EMU_REWRITER
#define _VSLDOI(_shb, _va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    a = _mm_bslli_si128(a, _shb); \
    b = _mm_bsrli_si128(b, 16 - _shb); \
    auto d = _mm_or_si128(a, b); \
    TH->r(_vd).set_xmm(d); \
}
#else
#define _VSLDOI(_shb, _va, _vb, _vd) { \
    assert(_shb <= 16); \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    a = _mm_shuffle_epi8(a, BYTE_SHIFT_LEFT_SHUFFLE_CONTROL[_shb]); \
    b = _mm_shuffle_epi8(b, BYTE_SHIFT_RIGHT_SHUFFLE_CONTROL[16 - _shb]); \
    auto d = _mm_or_si128(a, b); \
    TH->r(_vd).set_xmm(d); \
}
#endif
EMU_REWRITE(VSLDOI, SIMDForm, i->SHB_u(), i->vA_u(), i->vB_u(), i->vD_u())


PRINT(LVEWX, SIMDForm) {
    *result = format_nnn("lvewx", i->vD(), i->rA(), i->rB());
}

#define _LVEWX(_ra, _rb, _vd) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    auto xmm = MM->load128(ea); \
    TH->r(_vd).set_xmm(xmm); \
}
EMU_REWRITE(LVEWX, SIMDForm, i->rA_u(), i->rB_u(), i->vD_u())


PRINT(VSPLTW, SIMDForm) {
    *result = format_nnn("vspltw", i->vD(), i->vB(), i->UIMM2());
}

#define _VSPLTW(_vb, _uimm, _vd) { \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_shuffle_epi8(b, VSPLTW_SHUFFLE_CONTROL[_uimm]); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSPLTW, SIMDForm, i->vB_u(), i->UIMM2_u(), i->vD_u())

PRINT(VSPLTH, SIMDForm) {
    *result = format_nnn("vsplth", i->vD(), i->vB(), i->UIMM3());
}

#define _VSPLTH(_vb, _uimm, _vd) { \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_shuffle_epi8(b, VSPLTH_SHUFFLE_CONTROL[_uimm]); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSPLTH, SIMDForm, i->vB_u(), i->UIMM3_u(), i->vD_u())

PRINT(VSPLTISH, SIMDForm) {
    *result = format_nn("vspltish", i->vD(), i->SIMM());
}

#define _VSPLTISH(_simms, _vd) { \
    auto d = _mm_set1_epi16(_simms); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSPLTISH, SIMDForm, i->SIMM_s(), i->vD_u())

PRINT(VSPLTISB, SIMDForm) {
    *result = format_nn("vspltisb", i->vD(), i->SIMM());
}

#define _VSPLTISB(_simms, _vd) { \
    auto d = _mm_set1_epi8(_simms); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSPLTISB, SIMDForm, i->SIMM_s(), i->vD_u())

PRINT(VMRGHH, SIMDForm) {
    *result = format_nnn("vmrghh", i->vD(), i->vA(), i->vB());
}

#define _VMRGHH(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    a = _mm_shuffle_epi8(a, VMRGHH1_SHUFFLE_CONTROL); \
    b = _mm_shuffle_epi8(b, VMRGHH2_SHUFFLE_CONTROL); \
    auto d = _mm_or_si128(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VMRGHH, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VMULOSH, SIMDForm) { // TODO: test
    *result = format_nnn("vmulosh", i->vD(), i->vA(), i->vB());
}

#define _VMULOSH(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    a = _mm_slli_epi32(a, 16); \
    a = _mm_srai_epi32(a, 16); \
    b = _mm_slli_epi32(b, 16); \
    b = _mm_srai_epi32(b, 16); \
    auto d = _mm_mullo_epi32(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VMULOSH, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VMRGLH, SIMDForm) { // TODO: test
    *result = format_nnn("vmrglh", i->vD(), i->vA(), i->vB());
}

#define _VMRGLH(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    a = _mm_shuffle_epi8(a, VMRGLH1_SHUFFLE_CONTROL); \
    b = _mm_shuffle_epi8(b, VMRGLH2_SHUFFLE_CONTROL); \
    auto d = _mm_or_si128(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VMRGLH, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VSPLTISW, SIMDForm) { // TODO: test
    *result = format_nn("vspltisw", i->vD(), i->SIMM());
}

#define _VSPLTISW(_simms, _vd) { \
    auto d = _mm_set1_epi32(_simms); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSPLTISW, SIMDForm, i->SIMM_s(), i->vD_u())


PRINT(VMADDFP, SIMDForm) {
    *result = format_nnnn("vmaddfp", i->vD(), i->vA(), i->vC(), i->vB());
}

#define _VMADDFP(_va, _vb, _vc, _vd) { \
    auto a = TH->r(_va).xmm_f(); \
    auto b = TH->r(_vb).xmm_f(); \
    auto c = TH->r(_vc).xmm_f(); \
    auto d = _mm_fmadd_ps(a, c, b); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VMADDFP, SIMDForm, i->vA_u(), i->vB_u(), i->vC_u(), i->vD_u())


PRINT(VNMSUBFP, SIMDForm) {
    *result = format_nnnn("vnmsubfp", i->vD(), i->vA(), i->vC(), i->vB());
}

#define _VNMSUBFP(_va, _vb, _vc, _vd) { \
    auto a = TH->r(_va).xmm_f(); \
    auto b = TH->r(_vb).xmm_f(); \
    auto c = TH->r(_vc).xmm_f(); \
    auto d = _mm_fmsub_ps(a, c, b); \
    d = _mm_xor_ps(d, _mm_set1_ps(-0.f)); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VNMSUBFP, SIMDForm, i->vA_u(), i->vB_u(), i->vC_u(), i->vD_u())


PRINT(VXOR, SIMDForm) {
    *result = format_nnn("vxor", i->vD(), i->vA(), i->vB());
}

#define _VXOR(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_xor_si128(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VXOR, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VNOR, SIMDForm) {
    *result = format_nnn("vnor", i->vD(), i->vA(), i->vB());
}

#define _VNOR(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_or_si128(a, b); \
    d = _mm_xor_si128(d, _mm_set1_epi8(-1)); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VNOR, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VOR, SIMDForm) {
    *result = format_nnn("vor", i->vD(), i->vA(), i->vB());
}

#define _VOR(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_or_si128(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VOR, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VAND, SIMDForm) {
    *result = format_nnn("vand", i->vD(), i->vA(), i->vB());
}

#define _VAND(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_and_si128(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VAND, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VSEL, SIMDForm) {
    *result = format_nnnn("vsel", i->vD(), i->vA(), i->vB(), i->vC());
}

#define _VSEL(_vc, _va, _vb, _vd) { \
    auto m = TH->r(_vc).xmm(); \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = (m & b) | (~m & a); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSEL, SIMDForm, i->vC_u(), i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VADDFP, SIMDForm) {
    *result = format_nnn("vaddfp", i->vD(), i->vA(), i->vB());
}

#define _VADDFP(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm_f(); \
    auto b = TH->r(_vb).xmm_f(); \
    auto d = _mm_add_ps(a, b); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VADDFP, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VSUBFP, SIMDForm) {
    *result = format_nnn("vsubfp", i->vD(), i->vA(), i->vB());
}

#define _VSUBFP(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm_f(); \
    auto b = TH->r(_vb).xmm_f(); \
    auto d = _mm_sub_ps(a, b); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VSUBFP, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VRLW, SIMDForm) {
    *result = format_nnn("vrlw", i->vD(), i->vA(), i->vB());
}

#define _VRLW(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    b = _mm_and_si128(b, _mm_set1_epi32(0b11111)); \
    auto left = _mm_sllv_epi32(a, b); \
    b = _mm_sub_epi32(_mm_set1_epi32(32), b); \
    auto right = _mm_srlv_epi32(a, b); \
    auto d = _mm_or_si128(left, right); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VRLW, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VRFIN, SIMDForm) {
    *result = format_nn("vrfin", i->vD(), i->vB());
}

#define _VRFIN(_vd, _vb) { \
    auto b = TH->r(_vb).xmm_f(); \
    auto d = _mm_round_ps(b, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VRFIN, SIMDForm, i->vD_u(), i->vB_u())


PRINT(VRSQRTEFP, SIMDForm) {
    *result = format_nn("vrsqrtefp", i->vD(), i->vB());
}

#define _VRSQRTEFP(_vb, _vd) { \
    auto b = TH->r(_vb).xmm_f(); \
    auto abs_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)); \
    b = _mm_and_ps(b, abs_mask); \
    auto d = _mm_rsqrt_ps(b); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VRSQRTEFP, SIMDForm, i->vB_u(), i->vD_u())


PRINT(VCTSXS, SIMDForm) {
    *result = format_nnn("vctsxs", i->vD(), i->vB(), i->UIMM());
}

#define _VCTSXS(_vb, _uimm, _vd) { \
    auto b = TH->r(_vb).xmm_f(); \
    auto m = _mm_set1_ps(1u << _uimm); \
    auto x = _mm_mul_ps(b, m); \
    auto d = _mm_cvtps_epi32(x);\
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VCTSXS, SIMDForm, i->vB_u(), i->UIMM_u(), i->vD_u())


PRINT(VCFSX, SIMDForm) {
    *result = format_nnn("vcfsx", i->vD(), i->vB(), i->UIMM());
}

#define _VCFSX(_vb, _uimm, _vd) { \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_cvtepi32_ps(b); \
    auto div = _mm_set1_ps(1u << _uimm); \
    d = _mm_div_ps(d, div); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VCFSX, SIMDForm, i->vB_u(), i->UIMM_u(), i->vD_u())


PRINT(VADDUWM, SIMDForm) {
    *result = format_nnn("vadduwm", i->vD(), i->vA(), i->vB());
}

#define _VADDUWM(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_add_epi32(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VADDUWM, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())

PRINT(VSUBUHM, SIMDForm) {
    *result = format_nn("vsubuhm", i->vD(), i->vB());
}

#define _VSUBUHM(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_sub_epi16(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSUBUHM, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())

PRINT(VSUBUWM, SIMDForm) {
    *result = format_nnn("vsubuwm", i->vD(), i->vA(), i->vB());
}

#define _VSUBUWM(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_sub_epi32(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSUBUWM, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VCMPEQUW, SIMDForm) {
    *result = format_nnn(i->Rc_u() ? "vcmpequw." : "vcmpequw", i->vD(), i->vA(), i->vB());
}

#define updateCRF(d, rc) \
    {if (rc) { \
        bool t = _mm_test_all_ones(d); \
        bool f = _mm_test_all_ones(~d); \
        auto c = t << 3 | f << 1; \
        TH->setCRF(6, c); \
    }}

#define _VCMPEQUW(_va, _vb, _vd, _rc) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_cmpeq_epi32(a, b); \
    TH->r(_vd).set_xmm(d); \
    updateCRF(d, _rc); \
}
EMU_REWRITE(VCMPEQUW, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u(), i->Rc_u())

PRINT(VCMPEQUB, SIMDForm) {
    *result = format_nnn(i->Rc_u() ? "vcmpequb." : "vcmpequb", i->vD(), i->vA(), i->vB());
}

#define _VCMPEQUB(_va, _vb, _vd, _rc) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_cmpeq_epi8(a, b); \
    TH->r(_vd).set_xmm(d); \
    updateCRF(d, _rc); \
}
EMU_REWRITE(VCMPEQUB, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u(), i->Rc_u())


PRINT(VCMPGTFP, SIMDForm) {
    *result = format_nnn(i->Rc_u() ? "vcmpgtfp." : "vcmpgtfp", i->vD(), i->vA(), i->vB());
}

#define _VCMPGTFP(_va, _vb, _vd, _rc) { \
    auto a = TH->r(_va).xmm_f(); \
    auto b = TH->r(_vb).xmm_f(); \
    auto d = _mm_cmpgt_ps(a, b); \
    TH->r(_vd).set_xmm_f(d); \
    updateCRF(_mm_castps_si128(d), _rc); \
}
EMU_REWRITE(VCMPGTFP, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u(), i->Rc_u())


PRINT(VCMPGTUB, SIMDForm) {
    *result = format_nnn(i->Rc_u() ? "vcmpgtub." : "vcmpgtub", i->vD(), i->vA(), i->vB());
}

#define _VCMPGTUB(_va, _vb, _vd, _rc) { \
    auto val = _mm_set1_epi8(0x80); \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    a = _mm_sub_epi8(a, val); \
    b = _mm_sub_epi8(b, val); \
    auto d = _mm_cmpgt_epi8(a, b); \
    TH->r(_vd).set_xmm(d); \
    updateCRF(d, _rc); \
}
EMU_REWRITE(VCMPGTUB, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u(), i->Rc_u())


PRINT(VCMPGTUH, SIMDForm) {
    *result = format_nnn(i->Rc_u() ? "vcmpgtuh." : "vcmpgtuh", i->vD(), i->vA(), i->vB());
}

#define _VCMPGTUH(_va, _vb, _vd, _rc) { \
    auto val = _mm_set1_epi16(0x8000); \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    a = _mm_sub_epi16(a, val); \
    b = _mm_sub_epi16(b, val); \
    auto d = _mm_cmpgt_epi16(a, b); \
    TH->r(_vd).set_xmm(d); \
    updateCRF(d, _rc); \
}
EMU_REWRITE(VCMPGTUH, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u(), i->Rc_u())

PRINT(VCMPGTUW, SIMDForm) {
    *result = format_nnn(i->Rc_u() ? "vcmpgtuw." : "vcmpgtuw", i->vD(), i->vA(), i->vB());
}

#define _VCMPGTUW(_va, _vb, _vd, _rc) { \
    auto val = _mm_set1_epi32(0x80000000); \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    a = _mm_sub_epi32(a, val); \
    b = _mm_sub_epi32(b, val); \
    auto d = _mm_cmpgt_epi32(a, b); \
    TH->r(_vd).set_xmm(d); \
    updateCRF(d, _rc); \
}
EMU_REWRITE(VCMPGTUW, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u(), i->Rc_u())


PRINT(VCMPGTSW, SIMDForm) {
    *result = format_nnn(i->Rc_u() ? "vcmpgtsw." : "vcmpgtsw", i->vD(), i->vA(), i->vB());
}

#define _VCMPGTSW(_va, _vb, _vd, _rc) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_cmpgt_epi32(a, b); \
    TH->r(_vd).set_xmm(d); \
    updateCRF(d, _rc); \
}
EMU_REWRITE(VCMPGTSW, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u(), i->Rc_u())


PRINT(VCMPEQFP, SIMDForm) { // TODO: test
    *result = format_nnn(i->Rc_u() ? "vcmpeqfp." : "vcmpeqfp", i->vD(), i->vA(), i->vB());
}

#define _VCMPEQFP(_va, _vb, _vd, _rc) { \
    auto a = TH->r(_va).xmm_f(); \
    auto b = TH->r(_vb).xmm_f(); \
    auto d = _mm_cmpeq_ps(a, b); \
    TH->r(_vd).set_xmm_f(d); \
    updateCRF(_mm_castps_si128(d), _rc); \
}
EMU_REWRITE(VCMPEQFP, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u(), i->Rc_u())


PRINT(VCMPBFP, SIMDForm) {
    *result = format_nnn(i->Rc_u() ? "vcmpbfp." : "vcmpbfp", i->vD(), i->vA(), i->vB());
}

#define _VCMPBFP(_va, _vb, _vd, _rc) { \
    auto a = TH->r(_va).xmm_f(); \
    auto b = TH->r(_vb).xmm_f(); \
    auto le = _mm_cmple_ps(a, b); \
    auto ge = _mm_cmpge_ps(a, _mm_sub_ps(_mm_set1_ps(0), b)); \
    auto d = _mm_or_si128( \
        _mm_andnot_si128(_mm_castps_si128(le), _mm_set1_epi32(1 << 31)), \
        _mm_andnot_si128(_mm_castps_si128(ge), _mm_set1_epi32(1 << 30))); \
    TH->r(_vd).set_xmm(d); \
    updateCRF(d, _rc); \
}
EMU_REWRITE(VCMPBFP, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u(), i->Rc_u())


PRINT(VSRW, SIMDForm) {
    *result = format_nnn("vsrw", i->vD(), i->vA(), i->vB());
}

#define _VSRW(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    b = _mm_and_si128(b, _mm_set1_epi32(0b11111)); \
    auto d = _mm_srlv_epi32(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSRW, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VSLW, SIMDForm) {
    *result = format_nnn("vslw", i->vD(), i->vA(), i->vB());
}

#define _VSLW(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    b = _mm_and_si128(b, _mm_set1_epi32(0b11111)); \
    auto d = _mm_sllv_epi32(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSLW, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VMRGHW, SIMDForm) {
    *result = format_nnn("vmrghw", i->vD(), i->vA(), i->vB());
}

#define _VMRGHW(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    a = _mm_shuffle_epi8(a, VMRGHW1_SHUFFLE_CONTROL); \
    b = _mm_shuffle_epi8(b, VMRGHW2_SHUFFLE_CONTROL); \
    auto d = _mm_or_si128(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VMRGHW, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VMRGLW, SIMDForm) {
    *result = format_nnn("vmrglw", i->vD(), i->vA(), i->vB());
}

#define _VMRGLW(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    a = _mm_shuffle_epi8(a, VMRGLW1_SHUFFLE_CONTROL); \
    b = _mm_shuffle_epi8(b, VMRGLW2_SHUFFLE_CONTROL); \
    auto d = _mm_or_si128(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VMRGLW, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VPERM, SIMDForm) {
    *result = format_nnnn("vperm", i->vD(), i->vA(), i->vB(), i->vC());
}

#define _VPERM(_va, _vb, _vc, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto c = TH->r(_vc).xmm(); \
    c = _mm_sub_epi8(_mm_set1_epi8(31), c); \
    auto gt = _mm_cmpgt_epi8(c, _mm_set1_epi8(15)); \
    auto s1 = c; \
    auto s2 = _mm_sub_epi8(c, _mm_set1_epi8(16)); \
    auto d1 = _mm_shuffle_epi8(b, s1); \
    auto d2 = _mm_shuffle_epi8(a, s2); \
    auto d = _mm_blendv_epi8(d1, d2, gt); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VPERM, SIMDForm, i->vA_u(), i->vB_u(), i->vC_u(), i->vD_u())


PRINT(LVSR, SIMDForm) {
    *result = format_nnn("lvsr", i->vD(), i->rA(), i->rB());
}

#define _LVSR(_ra, _rb, _vd) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    auto sh = ea & 15; \
    auto d = LVSR_TABLE[sh]; \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(LVSR, SIMDForm, i->rA_u(), i->rB_u(), i->vD_u())


PRINT(LVSL, SIMDForm) {
    *result = format_nnn("lvsl", i->vD(), i->rA(), i->rB());
}

#define _LVSL(_ra, _rb, _vd) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    auto sh = ea & 15; \
    auto d = LVSL_TABLE[sh]; \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(LVSL, SIMDForm, i->rA_u(), i->rB_u(), i->vD_u())


PRINT(VANDC, SIMDForm) {
    *result = format_nnn("vandc", i->vD(), i->vA(), i->vB());
}

#define _VANDC(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_andnot_si128(b, a); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VANDC, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VREFP, SIMDForm) {
    *result = format_nn("vrefp", i->vD(), i->vB());
}

#define _VREFP(_vb, _vd) { \
    auto b = TH->r(_vb).xmm_f(); \
    auto d = _mm_rcp_ps(b); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VREFP, SIMDForm, i->vB_u(), i->vD_u())


PRINT(VSRAW, SIMDForm) {
    *result = format_nnn("vsraw", i->vD(), i->vA(), i->vB());
}

#define _VSRAW(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    b = _mm_and_si128(b, _mm_set1_epi32(0b11111)); \
    auto d = _mm_srav_epi32(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSRAW, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VRFIP, SIMDForm) {
    *result = format_nn("vrfip", i->vD(), i->vB());
}

#define _VRFIP(_vb, _vd) { \
    auto b = TH->r(_vb).xmm_f(); \
    auto d = _mm_ceil_ps(b); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VRFIP, SIMDForm, i->vB_u(), i->vD_u())


PRINT(VRFIM, SIMDForm) {
    *result = format_nn("vrfim", i->vD(), i->vB());
}

#define _VRFIM(_vb, _vd) { \
    auto b = TH->r(_vb).xmm_f(); \
    auto d = _mm_floor_ps(b); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VRFIM, SIMDForm, i->vB_u(), i->vD_u())


PRINT(VRFIZ, SIMDForm) {
    *result = format_nn("vrfiz", i->vD(), i->vB());
}

#define _VRFIZ(_vb, _vd) { \
    auto b = TH->r(_vb).xmm_f(); \
    auto d = _mm_round_ps(b, _MM_FROUND_TO_ZERO |_MM_FROUND_NO_EXC); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VRFIZ, SIMDForm, i->vB_u(), i->vD_u())


PRINT(VMAXFP, SIMDForm) {
    *result = format_nnn("vmaxfp", i->vD(), i->vA(), i->vB());
}

#define _VMAXFP(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm_f(); \
    auto b = TH->r(_vb).xmm_f(); \
    auto d = _mm_max_ps(a, b); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VMAXFP, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VMINFP, SIMDForm) {
    *result = format_nnn("vminfp", i->vD(), i->vA(), i->vB());
}

#define _VMINFP(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm_f(); \
    auto b = TH->r(_vb).xmm_f(); \
    auto d = _mm_min_ps(a, b); \
    TH->r(_vd).set_xmm_f(d); \
}
EMU_REWRITE(VMINFP, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())

PRINT(VMLADDUHM, SIMDForm) {
    *result = format_nnnn("vmladduhm", i->vD(), i->vA(), i->vB(), i->vC());
}

#define _VMLADDUHM(_va, _vb, _vc, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto c = TH->r(_vc).xmm(); \
    auto d = _mm_mullo_epi16(a, b); \
    d = _mm_add_epi16(d, c); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VMLADDUHM, SIMDForm, i->vA_u(), i->vB_u(), i->vC_u(), i->vD_u())

PRINT(VMHRADDSHS, SIMDForm) {
    *result = format_nnnn("vmhraddshs", i->vD(), i->vA(), i->vB(), i->vC());
}

#define _VMHRADDSHS(_va, _vb, _vc, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto c = TH->r(_vc).xmm(); \
    auto h = _mm_mulhi_epi16(a, b); \
    auto l = _mm_mullo_epi16(a, b); \
    auto hext1 = _mm_cvtepu16_epi32(_mm_bsrli_si128(h, 8)); \
    auto hext2 = _mm_cvtepu16_epi32(h); \
    auto lext1 = _mm_cvtepu16_epi32(_mm_bsrli_si128(l, 8)); \
    auto lext2 = _mm_cvtepu16_epi32(l); \
    auto bhl1 = _mm_or_si128(_mm_bslli_si128(hext1, 2), lext1); \
    auto bhl2 = _mm_or_si128(_mm_bslli_si128(hext2, 2), lext2); \
    bhl1 = _mm_add_epi32(bhl1, _mm_set1_epi32(0x00004000)); \
    bhl2 = _mm_add_epi32(bhl2, _mm_set1_epi32(0x00004000)); \
    bhl1 = _mm_srai_epi32(bhl1, 15); \
    bhl2 = _mm_srai_epi32(bhl2, 15); \
    auto c1 = _mm_cvtepi16_epi32(_mm_bsrli_si128(c, 8)); \
    auto c2 = _mm_cvtepi16_epi32(c); \
    auto s1 = _mm_add_epi32(bhl1, c1); \
    auto s2 = _mm_add_epi32(bhl2, c2); \
    s1 = _mm_max_epi32(s1, _mm_set1_epi32(INT16_MIN)); \
    s2 = _mm_max_epi32(s2, _mm_set1_epi32(INT16_MIN)); \
    s1 = _mm_min_epi32(s1, _mm_set1_epi32(INT16_MAX)); \
    s2 = _mm_min_epi32(s2, _mm_set1_epi32(INT16_MAX)); \
    auto left = _mm_shuffle_epi8(s1, \
        _mm_set_epi8(13, 12, 9, 8, 5, 4, 1, 0, -1, -1, -1, -1, -1, -1, -1, -1)); \
    auto right = _mm_shuffle_epi8(s2, \
        _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 13, 12, 9, 8, 5, 4, 1, 0)); \
    auto d = _mm_or_si128(left, right); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VMHRADDSHS, SIMDForm, i->vA_u(), i->vB_u(), i->vC_u(), i->vD_u())

PRINT(VCTUXS, SIMDForm) {
    *result = format_nnn("vctuxs", i->vD(), i->vB(), i->UIMM());
}

// TODO: test
#define _VCTUXS(_vb, _uimm, _vd) { \
    auto b = TH->r(_vb).xmm_f(); \
    auto m = _mm_set1_ps(1u << _uimm); \
    b = _mm_mul_ps(b, m); \
    b = _mm_max_ps(b, _mm_setzero_ps()); \
    auto d = _mm_cvtps_epi32(b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VCTUXS, SIMDForm, i->vB_u(), i->UIMM_u(), i->vD_u())


PRINT(STVEWX, SIMDForm) {
    *result = format_nnn("stvewx", i->vS(), i->rA(), i->rB());
}

#define _STVEWX(_ra, _rb, _vs) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    auto eb = ea & 3; \
    MM->store32(ea, TH->r(_vs).w(eb), TH->granule()); \
}
EMU_REWRITE(STVEWX, SIMDForm, i->rA_u(), i->rB_u(), i->vS_u())


PRINT(VADDSHS, SIMDForm) {
    *result = format_nnn("vaddshs", i->vD(), i->vA(), i->vB());
}

#define _VADDSHS(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_adds_epi16(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VADDSHS, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VSUBSHS, SIMDForm) {
    *result = format_nnn("vsubshs", i->vD(), i->vA(), i->vB());
}

#define _VSUBSHS(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_subs_epi16(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSUBSHS, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VADDUHM, SIMDForm) {
    *result = format_nnn("vadduhm", i->vD(), i->vA(), i->vB());
}

#define _VADDUHM(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_add_epi16(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VADDUHM, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VADDUBM, SIMDForm) {
    *result = format_nnn("vaddubm", i->vD(), i->vA(), i->vB());
}

#define _VADDUBM(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_add_epi8(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VADDUBM, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VSUBUBM, SIMDForm) {
    *result = format_nnn("vsububm", i->vD(), i->vA(), i->vB());
}

#define _VSUBUBM(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_sub_epi8(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSUBUBM, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VMINUB, SIMDForm) {
    *result = format_nnn("vminub", i->vD(), i->vA(), i->vB());
}

#define _VMINUB(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_min_epu8(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VMINUB, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())

PRINT(VMAXUB, SIMDForm) {
    *result = format_nnn("vmaxub", i->vD(), i->vA(), i->vB());
}

#define _VMAXUB(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_max_epu8(a, b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VMAXUB, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VPKSHUS, SIMDForm) {
    *result = format_nnn("vpkshus", i->vD(), i->vA(), i->vB());
}

#define _VPKSHUS(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_packus_epi16(b, a); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VPKSHUS, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VSLH, SIMDForm) {
    *result = format_nnn("vslh", i->vD(), i->vA(), i->vB());
}

#define _VSLH(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto aext = _mm256_cvtepu16_epi32(a); \
    auto sext = _mm256_cvtepu16_epi32(b); \
    sext = _mm256_and_si256(sext, _mm256_set1_epi32(0b1111)); \
    auto dext = _mm256_sllv_epi32(aext, sext); \
    auto sh = _mm256_set_epi8( \
        29, 28, 25, 24, 21, 20, 17, 16, \
        -1, -1, -1, -1, -1, -1, -1, -1, \
        -1, -1, -1, -1, -1, -1, -1, -1, \
        13, 12, 9, 8, 5, 4, 1, 0 \
    ); \
    dext = _mm256_shuffle_epi8(dext, sh); \
    auto d = _mm_or_si128( _mm256_extracti128_si256(dext, 1), \
                           _mm256_extracti128_si256(dext, 0) ); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSLH, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VSLB, SIMDForm) {
    *result = format_nnn("vslb", i->vD(), i->vA(), i->vB());
}

#define _VSLB(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    alignas(16) uint8_t abytes[16], bbytes[16]; \
    b = _mm_and_si128(_mm_set1_epi8(0b111), b); \
    _mm_store_si128((__m128i*)abytes, a); \
    _mm_store_si128((__m128i*)bbytes, b); \
    for (int i = 0; i < 16; ++i) { \
        abytes[i] <<= bbytes[i]; \
    } \
    auto d = _mm_load_si128((__m128i*)abytes); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSLB, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VSRAH, SIMDForm) {
    *result = format_nnn("vsrah", i->vD(), i->vA(), i->vB());
}

#define _VSRAH(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    auto aext = _mm256_cvtepi16_epi32(a); \
    auto sext = _mm256_cvtepu16_epi32(b); \
    sext = _mm256_and_si256(sext, _mm256_set1_epi32(0b1111)); \
    auto dext = _mm256_srav_epi32(aext, sext); \
    auto sh = _mm256_set_epi8( \
        29, 28, 25, 24, 21, 20, 17, 16, \
        -1, -1, -1, -1, -1, -1, -1, -1, \
        -1, -1, -1, -1, -1, -1, -1, -1, \
        13, 12, 9, 8, 5, 4, 1, 0 \
    ); \
    dext = _mm256_shuffle_epi8(dext, sh); \
    auto d = _mm_or_si128( _mm256_extracti128_si256(dext, 1), \
                           _mm256_extracti128_si256(dext, 0) ); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSRAH, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VSUM4UBS, SIMDForm) {
    *result = format_nnn("vsum4ubs", i->vD(), i->vA(), i->vB());
}

// TODO: SAT bit
#define _VSUM4UBS(_va, _vb, _vd) { \
    auto a = TH->r(_va).xmm(); \
    auto b = TH->r(_vb).xmm(); \
    alignas(16) uint8_t abytes[16]; \
    alignas(16) uint32_t bwords[4]; \
    _mm_store_si128((__m128i*)abytes, a); \
    _mm_store_si128((__m128i*)bwords, b); \
    alignas(16) uint32_t sums[4]; \
    for (int i = 0; i < 4; ++i) { \
        uint64_t sum = 0; \
        for (int j = 4 * i; j < 4 * i + 4; ++j) { \
            sum += abytes[j]; \
        } \
        sum += (uint64_t)bwords[i]; \
        if (sum > 0xffffffff) \
            sum = 0xffffffff; \
        sums[i] = sum; \
    } \
    auto d = _mm_load_si128((__m128i*)sums); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VSUM4UBS, SIMDForm, i->vA_u(), i->vB_u(), i->vD_u())


PRINT(VUPKHSB, SIMDForm) {
    *result = format_nn("vupkhsb", i->vD(), i->vB());
}

#define _VUPKHSB(_vb, _vd) { \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_cvtepi8_epi16(_mm_bsrli_si128(b, 8)); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VUPKHSB, SIMDForm, i->vB_u(), i->vD_u())


PRINT(VUPKLSB, SIMDForm) {
    *result = format_nn("vupklsb", i->vD(), i->vB());
}

#define _VUPKLSB(_vb, _vd) { \
    auto b = TH->r(_vb).xmm(); \
    auto d = _mm_cvtepi8_epi16(b); \
    TH->r(_vd).set_xmm(d); \
}
EMU_REWRITE(VUPKLSB, SIMDForm, i->vB_u(), i->vD_u())

PRINT(DCBZ, XForm_1) {
    *result = format_nn("dcbz", i->RA(), i->RB());
}

#define _DCBZ(_ra, _rb) { \
    auto b = getB(_ra, TH); \
    auto ea = b + TH->getGPR(_rb); \
    auto line = ea & ~127; \
    MM->setMemory(line, 0, 128); \
}
EMU_REWRITE(DCBZ, XForm_1, i->RA_u(), i->RB_u())


PRINT(DCBTST, XForm_1) {
    *result = format_nn("dcbtst", i->RA(), i->RB());
}

#define _DCBTST(_) { \
}
EMU_REWRITE(DCBTST, XForm_1, 0)


struct PPUDasmInstruction {
    const char* mnemonic;
    std::string operands;
};

#if !defined(EMU_REWRITER)
template <DasmMode M, typename S>
void ppu_dasm(const void* instr, uint64_t cia, S* state) {
    uint32_t x = big_to_native<uint32_t>(*reinterpret_cast<const uint32_t*>(instr));
    auto iform = reinterpret_cast<IForm*>(&x);
    switch (iform->OPCD_u()) {
        case NCALL_OPCODE: invoke(NCALL);
        case BB_CALL_OPCODE: invoke(BBCALL);
        case 4: {
            auto simd = reinterpret_cast<SIMDForm*>(&x);
            switch (simd->VA_XO_u()) {
                case 33: invoke(VMHRADDSHS);
                case 34: invoke(VMLADDUHM);
                case 46: invoke(VMADDFP);
                case 47: invoke(VNMSUBFP);
                case 42: invoke(VSEL);
                case 43: invoke(VPERM);
                case 44: invoke(VSLDOI);
                default:
                    switch (simd->VXR_XO_u()) {
                        case 198: invoke(VCMPEQFP);
                        case 646: invoke(VCMPGTUW);
                        case 582: invoke(VCMPGTUH);
                        case 518: invoke(VCMPGTUB);
                        case 710: invoke(VCMPGTFP);
                        case 902: invoke(VCMPGTSW);
                        case 134: invoke(VCMPEQUW);
                        case 6: invoke(VCMPEQUB);
                        default:
                            switch (simd->VX_XO_u()) {
                                case 836: invoke(VSRAH);
                                case 654: invoke(VUPKLSB);
                                case 526: invoke(VUPKHSB);
                                case 324: invoke(VSLH);
                                case 270: invoke(VPKSHUS);
                                case 64: invoke(VADDUHM);
                                case 0: invoke(VADDUBM);
                                case 1024: invoke(VSUBUBM);
                                case 514: invoke(VMINUB);
                                case 2: invoke(VMAXUB);
                                case 832: invoke(VADDSHS);
                                case 1856: invoke(VSUBSHS);
                                case 1220: invoke(VXOR);
                                case 652: invoke(VSPLTW);
                                case 588: invoke(VSPLTH);
                                case 908: invoke(VSPLTISW);
                                case 844: invoke(VSPLTISH);
                                case 780: invoke(VSPLTISB);
                                case 10: invoke(VADDFP);
                                case 74: invoke(VSUBFP);
                                case 970: invoke(VCTSXS);
                                case 1028: invoke(VAND);
                                case 1284: invoke(VNOR);
                                case 1156: invoke(VOR);
                                case 128: invoke(VADDUWM);
                                case 1152: invoke(VSUBUWM);
                                case 1088: invoke(VSUBUHM);
                                case 388: invoke(VSLW);
                                case 260: invoke(VSLB);
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
                                case 332: invoke(VMRGLH);
                                case 76: invoke(VMRGHH);
                                case 328: invoke(VMULOSH);
                                case 132: invoke(VRLW);
                                case 522: invoke(VRFIN);
                                case 1544: invoke(VSUM4UBS);
                                case 966: invoke(VCMPBFP);
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
            switch (xlform->XO_u()) {
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
            if (mdsform->XO_u() == 8) {
                invoke(RLDCL);
            } else if (mdsform->XO_u() == 9) {
                invoke(RLDCR);
            } else switch (mdform->XO_u()) {
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
            if (xoform->XO_u() == 40) {
                invoke(SUBF);
            } else if (xoform->XO_u() == 202) {
                invoke(ADDZE);
            } else if (xsform->XO_u() == 413) {
                invoke(SRADI);
            } else
            switch (xform->XO_u()) {
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
                case 551: invoke(LVRX);
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
                case 136: invoke(SUBFE);
                case 20: invoke(LWARX);
                case 84: invoke(LDARX);
                case 150: invoke(STWCX);
                case 214: invoke(STDCX);
                case 199: invoke(STVEWX);
                case 854: invoke(EIEIO);
                case 1014: invoke(DCBZ);
                case 246: invoke(DCBTST);
                case 792: invoke(SRAW);
                case 794: invoke(SRAD);
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
            switch (dsform->XO_u()) {
                case 2: invoke(LWA);
                case 0: invoke(LD);
                case 1: invoke(LDU);
                default: throw IllegalInstructionException();
            }
            break;
        }
        case 59: {
            auto aform = reinterpret_cast<AForm_1*>(&x);
            switch (aform->XO_u()) {
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
            switch (dsform->XO_u()) {
                case 0: invoke(STD);
                case 1: invoke(STDU);
                default: throw IllegalInstructionException();
            }
            break;
        }
        case 63: {
            auto xform = reinterpret_cast<XForm_1*>(&x);
            auto aform = reinterpret_cast<AForm_1*>(&x);
            if (aform->XO_u() == 21) {
                invoke(FADD);
            } else if (aform->XO_u() == 25) {
                invoke(FMUL);
            } else if (aform->XO_u() == 18) {
                invoke(FDIV);
            } else if (aform->XO_u() == 29) {
                invoke(FMADD);
            } else if (aform->XO_u() == 28) {
                invoke(FMSUB);
            } else if (aform->XO_u() == 20) {
                invoke(FSUB);
            } else if (aform->XO_u() == 31) {
                invoke(FNMADD);
            } else if (aform->XO_u() == 30) {
                invoke(FNMSUB);
            } else if (aform->XO_u() == 22) {
                invoke(FSQRT);
            } else if (aform->XO_u() == 24) {
                invoke(FRE);
            } else if (aform->XO_u() == 26) {
                invoke(FRSQRTE);
            } else if (aform->XO_u() == 23) {
                invoke(FSEL);
            } else
            switch (xform->XO_u()) {
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
        default: throw IllegalInstructionException();
    }
}

BranchMnemonicType getExtBranchMnemonic(bool lr,
                                        bool abs,
                                        bool tolr,
                                        bool toctr,
                                        BitField<6, 11> btbo,
                                        BitField<11, 16> bi,
                                        std::string& mnemonic) {
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

        m = sformat("b{}{}{}{}{}",
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
            m = sformat("bd{}{}{}{}",
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

std::string formatCRbit(BitField<11, 16> bi) {
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
    return sformat("4*cr{}+{}", bi.u() / 4, crbit);
}

bool isAbsoluteBranch(uint32_t instr) {
    auto iform = reinterpret_cast<IForm*>(&instr);
    return iform->OPCD_u() == 18 || iform->OPCD_u() == 16;
}

bool isTaken(uint32_t branchInstr, uint32_t cia, PPUThread* thread) {
    auto iform = reinterpret_cast<IForm*>(&branchInstr);
    if (iform->OPCD_u() == 18) {
        return true;
    } else if (iform->OPCD_u() == 16) {
        auto b = reinterpret_cast<BForm*>(&branchInstr);
        return isTaken(b->BO0_u(), b->BO1_u(), b->BO2_u(), b->BO3_u(), thread, b->BI_u());
    }
    throw std::runtime_error("not absolute branch");
}

uint64_t getTargetAddress(uint32_t branchInstr, uint32_t cia) {
    auto iform = reinterpret_cast<IForm*>(&branchInstr);
    if (iform->OPCD_u() == 18) {
        return getNIA(iform, cia);
    } else if (iform->OPCD_u() == 16) {
        return getNIA(reinterpret_cast<BForm*>(&branchInstr), cia);
    }
    throw std::runtime_error("not absolute branch");
}

InstructionInfo analyze(uint32_t instr, uint32_t cia) {
    InstructionInfo info;
    auto iform = reinterpret_cast<IForm*>(&instr);
    auto bform = reinterpret_cast<BForm*>(&instr);
    auto xlform1 = reinterpret_cast<XLForm_1*>(&instr);
    auto xlform2 = reinterpret_cast<XLForm_2*>(&instr);
    if (iform->OPCD_u() == 1) { // ncall
        info.flow = true;
        info.ncall = true;
    }
    if (iform->OPCD_u() == 18) { // b
        info.flow = true;
        info.target = getNIA(iform, cia);
        info.passthrough = iform->LK_u();
    }
    if (iform->OPCD_u() == 16) { // bc
        info.flow = true;
        info.passthrough = !(bform->BO2_u() && bform->BO0_u()) || bform->LK_u();
        info.target = getNIA(bform, cia);
    }
    if (iform->OPCD_u() == 19) {
        if (xlform1->XO_u() == 16) {
            info.flow = true;
            info.passthrough = !(xlform2->BO2_u() && xlform2->BO0_u()) || xlform2->LK_u();
        }
        if (xlform1->XO_u() == 528) {
            info.flow = true;
            info.passthrough = !xlform2->BO0_u() || xlform2->LK_u();
        }
    }
    return info;
}

template void ppu_dasm<DasmMode::Print, std::string>(
    const void* instr, uint64_t cia, std::string* state);

template void ppu_dasm<DasmMode::Name, std::string>(
    const void* instr, uint64_t cia, std::string* name);

template void ppu_dasm<DasmMode::Rewrite, std::string>(
    const void* instr, uint64_t cia, std::string* name);

template void ppu_dasm<DasmMode::Emulate, PPUThread>(
    const void* instr, uint64_t cia, PPUThread* th);
#endif
