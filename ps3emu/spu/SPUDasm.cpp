#include "SPUDasm.h"

#include "SPUThread.h"
#include "../BitField.h"
#include "../dasm_utils.h"
#include "../utils.h"
#include <boost/endian/conversion.hpp>
#include <bitset>
#include "../log.h"

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

using I16_t = BitField<9, 25, BitFieldType::Signed>;

union SPUForm {
    BitField<0, 4> OP4;
    BitField<0, 7> OP7;
    BitField<0, 6> OP6;
    BitField<0, 8> OP8;
    BitField<0, 9> OP9;
    BitField<0, 10> OP10;
    BitField<0, 11> OP11;
    BitField<11, 18, BitFieldType::GPR> RB;
    BitField<18, 25, BitFieldType::GPR> RA;
    BitField<25, 32, BitFieldType::GPR> RC;
    BitField<25, 32, BitFieldType::GPR> RT;
    BitField<4, 11, BitFieldType::GPR> RT_ABC;
    BitField<18, 25, BitFieldType::Channel> CA;
    BitField<11, 18, BitFieldType::Signed> I7;
    BitField<10, 18, BitFieldType::Signed> I8;
    BitField<8, 18, BitFieldType::Signed> I10;
    I16_t I16;
    BitField<7, 25, BitFieldType::Signed> I18;
    BitField<18, 32> StopAndSignalType;
};

#define EMU(name) inline void emulate##name(SPUForm* i, uint32_t cia, SPUThread* thread)
#define INVOKE(name) invoke_impl<M>(#name, print##name, emulate##name, rewrite##name, &x, cia, state); return
#define PRINT(name) inline void print##name(SPUForm* i, uint32_t cia, std::string* result)

#ifdef EMU_REWRITER
#define EMU_REWRITE(...)
#else
#define EMU_REWRITE(...) \
    inline void BOOST_PP_CAT(rewrite, BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)) \
        (SPUForm* i, uint64_t cia, std::string* result) { \
            *result = rewrite_print( \
                BOOST_PP_STRINGIZE(BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)), \
                BOOST_PP_LIST_ENUM( \
                    BOOST_PP_LIST_REST_N(1, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__))) \
            ); \
        } \
    inline void BOOST_PP_CAT(emulate, BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)) \
        (SPUForm* i, uint64_t cia, SPUThread* thread) { \
            BOOST_PP_EXPAND(BOOST_PP_CAT(_, BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)) BOOST_PP_LPAREN() \
                BOOST_PP_LIST_ENUM( \
                    BOOST_PP_LIST_REST_N(1, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__)))) \
            ); \
        }
#endif

#ifdef EMU_REWRITER
    #define SPU_SET_NIP_INDIRECT_PIC(x) { thread->setNip(((x) & ~3ul) + pic_offset); return; }
    #define SPU_SET_NIP_INDIRECT(x) { thread->setNip((x) & ~3ul); return; }
    #define SPU_SET_NIP_INITIAL(x) goto _##x
    #define SPU_SET_NIP SPU_SET_NIP_INITIAL
    #define SPU_RESTORE_NIP(cia) thread->setNip(cia + 4 + pic_offset)
    #define SPU_ADJUST_LINK(x) ((x) + pic_offset)
#else
    #define SPU_SET_NIP(x) thread->setNip(x)
    #define SPU_SET_NIP_INDIRECT(x) SPU_SET_NIP(x)
    #define SPU_ADJUST_LINK(x) x
    #define SPU_RESTORE_NIP(cia)
#endif

#define th thread

PRINT(bbcall) {
    auto bbform = (BBCallForm*)i;
    *result = format_nn("bbcall", bbform->Segment, bbform->Label);
}

#define _bbcall(_) { \
    assert(false); \
}
EMU_REWRITE(bbcall, 0);

alignas(16) static const __m128i BYTE_ROTATE_LEFT_SHUFFLE_CONTROL[16] {
    _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0),
    _mm_set_epi8(14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15),
    _mm_set_epi8(13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14),
    _mm_set_epi8(12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13),
    _mm_set_epi8(11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12),
    _mm_set_epi8(10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11),
    _mm_set_epi8(9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10),
    _mm_set_epi8(8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9),
    _mm_set_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8),
    _mm_set_epi8(6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7),
    _mm_set_epi8(5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6),
    _mm_set_epi8(4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5),
    _mm_set_epi8(3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4),
    _mm_set_epi8(2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3),
    _mm_set_epi8(1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2),
    _mm_set_epi8(0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1),
};

alignas(16) static const __m128i GENERATE_CONTROL_BYTE[32] {
    _mm_set_epi8(0x3, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x3, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x3, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x3, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x3, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x3, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x3, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x3, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x3, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x3, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x3, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x3, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x3, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x3, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x3, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x3),
};

alignas(16) static const __m128i GENERATE_CONTROL_HW[32] {
    _mm_set_epi8(0x2, 0x3, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x2, 0x3, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x2, 0x3, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x2, 0x3, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x2, 0x3, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x2, 0x3, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x2, 0x3, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x2, 0x3),
};

alignas(16) static const __m128i GENERATE_CONTROL_W[32] {
    _mm_set_epi8(0x0, 0x1, 0x2, 0x3, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x0, 0x1, 0x2, 0x3, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x0, 0x1, 0x2, 0x3, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x0, 0x1, 0x2, 0x3),
};

alignas(16) static const __m128i GENERATE_CONTROL_DW[32] {
    _mm_set_epi8(0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f),
    _mm_set_epi8(0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7),
};

PRINT(lqd) {
    *result = format_br_nnn("lqd", i->RT, i->I10, i->RA);
}

#define _lqd(_i10_4, _ra, _rt) { \
    auto lsa = (th->r(_ra).w<0>() + _i10_4) & LSLR & 0xfffffff0; \
    auto val = _mm_lddqu_si128((__m128i*)th->ptr(lsa)); \
    val = _mm_shuffle_epi8(val, ENDIAN_SWAP_MASK128); \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(lqd, (i->I10 << 4), i->RA.u(), i->RT.u())


PRINT(lqx) {
    *result = format_nnn("lqx", i->RT, i->RA, i->RB);
}

#define _lqx(_rb, _ra, _rt) { \
    auto lsa = (th->r(_ra).w<0>() + th->r(_rb).w<0>()) & LSLR & 0xfffffff0; \
    auto val = _mm_lddqu_si128((__m128i*)th->ptr(lsa)); \
    val = _mm_shuffle_epi8(val, ENDIAN_SWAP_MASK128); \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(lqx, i->RB.u(), i->RA.u(), i->RT.u())


inline uint32_t abs_lsa(I16_t i16) {
    return (i16 << 2) & LSLR & 0xfffffff0;
}

inline uint32_t cia_lsa(I16_t i16, uint32_t cia) {
    return ((i16 << 2) + cia) & LSLR & 0xfffffff0;
}

PRINT(lqa) {
    *result = format_nu("lqa", i->RT, abs_lsa(i->I16));
}

#define _lqa(_abs_lsa, _rt) { \
    auto lsa = _abs_lsa; \
    auto val = _mm_lddqu_si128((__m128i*)th->ptr(lsa)); \
    val = _mm_shuffle_epi8(val, ENDIAN_SWAP_MASK128); \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(lqa, abs_lsa(i->I16), i->RT.u())


PRINT(lqr) {
    *result = format_nu("lqr", i->RT, cia_lsa(i->I16, cia));
}

#define _lqr(_cia_lsa, _rt) { \
    auto lsa = SPU_ADJUST_LINK(_cia_lsa); \
    auto val = _mm_lddqu_si128((__m128i*)th->ptr(lsa)); \
    val = _mm_shuffle_epi8(val, ENDIAN_SWAP_MASK128); \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(lqr, cia_lsa(i->I16, cia), i->RT.u())


PRINT(stqd) {
    *result = format_br_nnn("stqd", i->RT, i->I10, i->RA);
}

#define _stqd(_i10_4, _ra, _rt) { \
    auto lsa = (th->r(_ra).w<0>() + _i10_4) & LSLR & 0xfffffff0; \
    auto val = th->r(_rt).xmm(); \
    val = _mm_shuffle_epi8(val, ENDIAN_SWAP_MASK128); \
    _mm_store_si128((__m128i*)th->ptr(lsa), val); \
}
EMU_REWRITE(stqd, (i->I10 << 4), i->RA.u(), i->RT.u())


PRINT(stqx) {
    *result = format_nnn("stqx", i->RT, i->RA, i->RB);
}

#define _stqx(_rb, _ra, _rt) { \
    auto lsa = (th->r(_ra).w<0>() + th->r(_rb).w<0>()) & LSLR & 0xfffffff0; \
    auto val = th->r(_rt).xmm(); \
    val = _mm_shuffle_epi8(val, ENDIAN_SWAP_MASK128); \
    _mm_store_si128((__m128i*)th->ptr(lsa), val); \
}
EMU_REWRITE(stqx, i->RB.u(), i->RA.u(), i->RT.u())


PRINT(stqa) {
    *result = format_nu("stqa", i->RT, abs_lsa(i->I16));
}

#define _stqa(_abs_lsa, _rt) { \
    auto lsa = _abs_lsa; \
    auto val = th->r(_rt).xmm(); \
    val = _mm_shuffle_epi8(val, ENDIAN_SWAP_MASK128); \
    _mm_store_si128((__m128i*)th->ptr(lsa), val); \
}
EMU_REWRITE(stqa, abs_lsa(i->I16), i->RT.u())


PRINT(stqr) {
    *result = format_nu("stqr", i->RT, cia_lsa(i->I16, cia));
}

#define _stqr(_cia_lsa, _rt) { \
    auto lsa = SPU_ADJUST_LINK(_cia_lsa); \
    auto val = th->r(_rt).xmm(); \
    val = _mm_shuffle_epi8(val, ENDIAN_SWAP_MASK128); \
    _mm_store_si128((__m128i*)th->ptr(lsa), val); \
}
EMU_REWRITE(stqr, cia_lsa(i->I16, cia), i->RT.u())


PRINT(cbd) {
    *result = format_nnn("cbd", i->RT, i->I7, i->RA);
}

#define _cbd(_i7, _ra, _rt) { \
    auto t = _i7 + th->r(_ra).w<0>(); \
    auto val = GENERATE_CONTROL_BYTE[t & 0xf]; \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(cbd, i->I7.s(), i->RA.u(), i->RT.u())


PRINT(cbx) {
    *result = format_nnn("cbx", i->RT, i->RA, i->RB);
}

#define _cbx(_rb, _ra, _rt) { \
    auto t = (uint32_t)th->r(_ra).w<0>() + (uint32_t)th->r(_rb).w<0>(); \
    auto val = GENERATE_CONTROL_BYTE[t & 0xf]; \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(cbx, i->RB.u(), i->RA.u(), i->RT.u())


PRINT(chd) {
    *result = format_nnn("chd", i->RT, i->I7, i->RA);
}

#define _chd(_i7, _ra, _rt) { \
    auto t = _i7 + th->r(_ra).w<0>(); \
    auto val = GENERATE_CONTROL_HW[(t & 0xe) >> 1]; \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(chd, i->I7.s(), i->RA.u(), i->RT.u())


PRINT(chx) {
    *result = format_nnn("chx", i->RT, i->RA, i->RB);
}

#define _chx(_rb, _ra, _rt) { \
    auto t = (uint32_t)th->r(_ra).w<0>() + (uint32_t)th->r(_rb).w<0>(); \
    auto val = GENERATE_CONTROL_HW[(t & 0xe) >> 1]; \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(chx, i->RB.u(), i->RA.u(), i->RT.u())


PRINT(cwd) {
    *result = format_br_nnn("cwd", i->RT, i->I7, i->RA);
}

#define _cwd(_i7, _ra, _rt) { \
    auto t = _i7 + th->r(_ra).w<0>(); \
    auto val = GENERATE_CONTROL_W[(t & 0xc) >> 2]; \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(cwd, i->I7.s(), i->RA.u(), i->RT.u())


PRINT(cwx) {
    *result = format_nnn("cwx", i->RT, i->RA, i->RB);
}

#define _cwx(_rb, _ra, _rt) { \
    auto t = th->r(_ra).w<0>() + th->r(_rb).w<0>(); \
    auto val = GENERATE_CONTROL_W[(t & 0xc) >> 2]; \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(cwx, i->RB.u(), i->RA.u(), i->RT.u())


PRINT(cdd) {
    *result = format_br_nnn("cdd", i->RT, i->I7, i->RA);
}

#define _cdd(_i7, _ra, _rt) { \
    auto t = _i7 + th->r(_ra).w<0>(); \
    auto val = GENERATE_CONTROL_DW[(t & 0x8) >> 3]; \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(cdd, i->I7.s(), i->RA.u(), i->RT.u())


PRINT(cdx) {
    *result = format_nnn("cdx", i->RT, i->RA, i->RB);
}

#define _cdx(_rb, _ra, _rt) { \
    auto t = th->r(_ra).w<0>() + th->r(_rb).w<0>(); \
    auto val = GENERATE_CONTROL_DW[(t & 0x8) >> 3]; \
    th->r(_rt).set_xmm(val); \
}
EMU_REWRITE(cdx, i->RB.u(), i->RA.u(), i->RT.u())


PRINT(ilh) {
    *result = format_nn("ilh", i->RT, i->I16);
}

#define _ilh(_rt, _i16) { \
    auto t = _mm_set1_epi16(_i16); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(ilh, i->RT.u(), i->I16.s())


PRINT(ilhu) {
    *result = format_nn("ilhu", i->RT, i->I16);
}

#define _ilhu(_rt, _i16_16) { \
    auto t = _mm_set1_epi32(_i16_16); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(ilhu, i->RT.u(), (i->I16.u() << 16))


PRINT(il) {
    *result = format_nn("il", i->RT, i->I16);
}

#define _il(_rt, _i16) { \
    auto t = _mm_set1_epi32(_i16); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(il, i->RT.u(), i->I16.s())


PRINT(ila) {
    *result = format_nu("ila", i->RT, i->I18.u());
}

#define _ila(_rt, _i18) { \
    auto t = _mm_set1_epi32(_i18); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(ila, i->RT.u(), i->I18.u())


PRINT(iohl) {
    *result = format_nu("iohl", i->RT, i->I16.u());
}

#define _iohl(_rt, _i16) { \
    auto t = _mm_set1_epi32(_i16); \
    auto rt = th->r(_rt).xmm(); \
    th->r(_rt).set_xmm(rt | t); \
}
EMU_REWRITE(iohl, i->RT.u(), i->I16.u())


PRINT(fsmbi) {
    *result = format_nu("fsmbi", i->RT, i->I16.u());
}

#ifdef EMU_REWRITER
#define _fsmbi(_rt, _i16) { \
    auto t = _mm_set_epi8( \
        _i16 & 0b1000000000000000 ? 0xff : 0, \
        _i16 & 0b0100000000000000 ? 0xff : 0, \
        _i16 & 0b0010000000000000 ? 0xff : 0, \
        _i16 & 0b0001000000000000 ? 0xff : 0, \
        _i16 & 0b0000100000000000 ? 0xff : 0, \
        _i16 & 0b0000010000000000 ? 0xff : 0, \
        _i16 & 0b0000001000000000 ? 0xff : 0, \
        _i16 & 0b0000000100000000 ? 0xff : 0, \
        _i16 & 0b0000000010000000 ? 0xff : 0, \
        _i16 & 0b0000000001000000 ? 0xff : 0, \
        _i16 & 0b0000000000100000 ? 0xff : 0, \
        _i16 & 0b0000000000010000 ? 0xff : 0, \
        _i16 & 0b0000000000001000 ? 0xff : 0, \
        _i16 & 0b0000000000000100 ? 0xff : 0, \
        _i16 & 0b0000000000000010 ? 0xff : 0, \
        _i16 & 0b0000000000000001 ? 0xff : 0 \
    ); \
    th->r(_rt).set_xmm(t); \
}
#else
#define _fsmbi(_rt, _i16) { \
    auto right = _pdep_u64(_i16, 0x8080808080808080ull); \
    auto left = _pdep_u64(_i16 >> 8, 0x8080808080808080ull); \
    __m128i lr = _mm_set_epi64x(left, right); \
    auto t = _mm_cmplt_epi8(lr, _mm_setzero_si128()); \
    th->r(_rt).set_xmm(t); \
}
#endif
EMU_REWRITE(fsmbi, i->RT.u(), i->I16.u())


PRINT(ah) {
    *result = format_nnn("ah", i->RT, i->RA, i->RB);
}

#define _ah(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = _mm_add_epi16(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(ah, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(ahi) {
    *result = format_nnn("ahi", i->RT, i->RA, i->I10);
}

#define _ahi(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto s = _mm_set1_epi16(_i10); \
    auto t = _mm_add_epi16(a, s); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(ahi, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(a) {
    *result = format_nnn("a", i->RT, i->RA, i->RB);
}

#define _a(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = _mm_add_epi32(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(a, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(ai) {
    *result = format_nnn("ai", i->RT, i->RA, i->I10);
}

#define _ai(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto s = _mm_set1_epi32(_i10); \
    auto t = _mm_add_epi32(a, s); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(ai, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(sfh) {
    *result = format_nnn("sfh", i->RT, i->RA, i->RB);
}

#define _sfh(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = _mm_sub_epi16(b, a); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(sfh, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(sfhi) {
    *result = format_nnn("sfhi", i->RT, i->RA, i->I10);
}

#define _sfhi(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi16(_i10); \
    auto t = _mm_sub_epi16(b, a); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(sfhi, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(sf) {
    *result = format_nnn("sf", i->RT, i->RA, i->RB);
}

#define _sf(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = _mm_sub_epi32(b, a); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(sf, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(sfi) {
    *result = format_nnn("sfi", i->RT, i->RA, i->I10);
}

#define _sfi(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi32(_i10); \
    auto t = _mm_sub_epi32(b, a); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(sfi, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(addx) {
    *result = format_nnn("addx", i->RT, i->RA, i->RB);
}

#define _addx(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = th->r(_rt).xmm(); \
    auto mask = _mm_set1_epi32(1); \
    auto ab = _mm_add_epi32(a, b); \
    t = _mm_and_si128(t, mask); \
    t = _mm_add_epi32(ab, t); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(addx, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(cg) {
    *result = format_nnn("cg", i->RT, i->RA, i->RB);
}

#define _cg(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto ab = _mm_add_epi32(a, b); \
    auto val = _mm_set1_epi32(0x80000000); \
    a = _mm_sub_epi32(a, val); \
    ab = _mm_sub_epi32(ab, val); \
    auto t = _mm_cmplt_epi32(ab, a); \
    t = _mm_srli_epi32(t, 31); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(cg, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(cgx) {
    *result = format_nnn("cg", i->RT, i->RA, i->RB);
}

#define _cgx(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        uint64_t t = (uint64_t)(uint32_t)rb.w(i) \
                   + (uint64_t)(uint32_t)ra.w(i) \
                   + (rt.w(i) & 1); \
        rt.set_w(i, (t >> 32) & 1); \
    } \
}
EMU_REWRITE(cgx, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(sfx) {
    *result = format_nnn("sfx", i->RT, i->RA, i->RB);
}

#define _sfx(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, (uint64_t)rb.w(i) + ~ra.w(i) + (rt.w(i) & 1)); \
    } \
}
EMU_REWRITE(sfx, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(bg) {
    *result = format_nnn("bg", i->RT, i->RA, i->RB);
}

#define _bg(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto val = _mm_set1_epi32(0x80000000); \
    a = _mm_sub_epi32(a, val); \
    b = _mm_sub_epi32(b, val); \
    auto t = ~_mm_cmpgt_epi32(a, b); \
    t = _mm_srli_epi32(t, 31); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(bg, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(bgx) {
    *result = format_nnn("bg", i->RT, i->RA, i->RB);
}

#define _bgx(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        if (rt.w(i) & 1) { \
            rt.set_w(i, (uint32_t)rb.w(i) >= (uint32_t)ra.w(i)); \
        } else { \
            rt.set_w(i, (uint32_t)rb.w(i) > (uint32_t)ra.w(i)); \
        } \
    } \
}
EMU_REWRITE(bgx, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(mpy) {
    *result = format_nnn("mpy", i->RT, i->RA, i->RB);
}

#define _mpy(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    a = _mm_slli_epi32(a, 16); \
    a = _mm_srai_epi32(a, 16); \
    b = _mm_slli_epi32(b, 16); \
    b = _mm_srai_epi32(b, 16); \
    auto t = _mm_mullo_epi32(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(mpy, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(mpyu) {
    *result = format_nnn("mpyu", i->RT, i->RA, i->RB);
}

#define _mpyu(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto mask = _mm_set1_epi32(0x0000ffff); \
    a = _mm_and_si128(a, mask); \
    b = _mm_and_si128(b, mask); \
    auto t = _mm_mullo_epi32(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(mpyu, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(mpyi) {
    *result = format_nnn("mpyi", i->RT, i->RA, i->I10);
}

#define _mpyi(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi32(_i10); \
    a = _mm_slli_epi32(a, 16); \
    a = _mm_srai_epi32(a, 16); \
    auto t = _mm_mullo_epi32(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(mpyi, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(mpyui) {
    *result = format_nnn("mpyui", i->RT, i->RA, i->I10);
}

#define _mpyui(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi32(_i10 & 0xffff); \
    auto mask = _mm_set1_epi32(0xffff); \
    a = _mm_and_si128(a, mask); \
    auto t = _mm_mullo_epi32(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(mpyui, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(mpya) {
    *result = format_nnnn("mpya", i->RT_ABC, i->RA, i->RB, i->RC);
}

#define _mpya(_ra, _rb, _rc, _rt_abc) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rc = th->r(_rc); \
    auto& rt = th->r(_rt_abc); \
    for (int i = 0; i < 4; ++i) { \
        uint32_t t = (int16_t)ra.w(i); \
        t *= (int16_t)rb.w(i); \
        rt.set_w(i, t + rc.w(i)); \
    } \
}
EMU_REWRITE(mpya, i->RA.u(), i->RB.u(), i->RC.u(), i->RT_ABC.u())


PRINT(mpyh) {
    *result = format_nnn("mpyh", i->RT, i->RA, i->RB);
}

#define _mpyh(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    a = _mm_srai_epi32(a, 16); \
    b = _mm_slli_epi32(b, 16); \
    b = _mm_srai_epi32(b, 16); \
    auto t = _mm_mullo_epi32(a, b); \
    t = _mm_slli_epi32(t, 16); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(mpyh, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(mpys) {
    *result = format_nnn("mpys", i->RT, i->RA, i->RB);
}

#define _mpys(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        int32_t t = (int16_t)ra.w(i); \
        t *= (int16_t)rb.w(i); \
        rt.set_w(i, signed_rshift32(t, 16)); \
    } \
}
EMU_REWRITE(mpys, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(mpyhh) {
    *result = format_nnn("mpyhh", i->RT, i->RA, i->RB);
}

#define _mpyhh(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        int32_t t = (int16_t)signed_rshift32(ra.w(i), 16); \
        t *= (int16_t)signed_rshift32(rb.w(i), 16); \
        rt.set_w(i, t); \
    } \
}
EMU_REWRITE(mpyhh, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(mpyhha) {
    *result = format_nnn("mpyhha", i->RT, i->RA, i->RB);
}

#define _mpyhha(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        int32_t t = (int16_t)signed_rshift32(ra.w(i), 16); \
        t *= (int16_t)signed_rshift32(rb.w(i), 16); \
        rt.set_w(i, rt.w(i) + t); \
    } \
}
EMU_REWRITE(mpyhha, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(mpyhhu) {
    *result = format_nnn("mpyhhu", i->RT, i->RA, i->RB);
}

#define _mpyhhu(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, ((uint32_t)ra.w(i) >> 16) * ((uint32_t)rb.w(i) >> 16)); \
    } \
}
EMU_REWRITE(mpyhhu, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(mpyhhau) {
    *result = format_nnn("mpyhhau", i->RT, i->RA, i->RB);
}

#define _mpyhhau(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, rt.w(i) + ((uint32_t)ra.w(i) >> 16) * ((uint32_t)rb.w(i) >> 16)); \
    } \
}
EMU_REWRITE(mpyhhau, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(clz) {
    *result = format_nn("clz", i->RT, i->RA);
}

#define _clz(_ra, _rt) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, ra.w(i) == 0 ? 32 : __builtin_clz(ra.w(i))); \
    } \
}
EMU_REWRITE(clz, i->RA.u(), i->RT.u())


PRINT(cntb) {
    *result = format_nn("cntb", i->RT, i->RA);
}

#define _cntb(_ra, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto mask = _mm_set1_epi8(15); \
    auto lookup = _mm_setr_epi8(0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4); \
    auto low = _mm_and_si128(mask, a); \
    auto sh = _mm_srli_epi16(a, 4); \
    auto high = _mm_and_si128(mask, sh); \
    auto shfl = _mm_shuffle_epi8(lookup, low); \
    auto shfh = _mm_shuffle_epi8(lookup, high); \
    auto t = _mm_add_epi8(shfl, shfh); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(cntb, i->RA.u(), i->RT.u())


PRINT(fsmb) {
    *result = format_nn("fsmb", i->RT, i->RA);
}

#define _fsmb(_ra, _rt) { \
    auto a = th->r(_ra).w<0>(); \
    uint64_t low = a & 0xff; \
    uint64_t high = (a >> 8) & 0xff; \
    uint64_t mask = 0x0101010101010101ull; \
    uint64_t tlow = _pdep_u64(low, mask); \
    uint64_t thigh = _pdep_u64(high, mask); \
    auto t = _mm_set_epi64x(thigh, tlow); \
    auto zero = _mm_set1_epi8(0); \
    t = _mm_cmpgt_epi8(t, zero); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(fsmb, i->RA.u(), i->RT.u())


PRINT(fsmh) {
    *result = format_nn("fsmh", i->RT, i->RA);
}

#define _fsmh(_ra, _rt) { \
    auto a = th->r(_ra).w<0>(); \
    uint64_t low = a & 0xf; \
    uint64_t high = (a >> 4) & 0xf; \
    uint64_t mask = 0x0001000100010001ull; \
    uint64_t tlow = _pdep_u64(low, mask); \
    uint64_t thigh = _pdep_u64(high, mask); \
    auto t = _mm_set_epi64x(thigh, tlow); \
    auto zero = _mm_set1_epi16(0); \
    t = _mm_cmpgt_epi16(t, zero); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(fsmh, i->RA.u(), i->RT.u())


PRINT(fsm) {
    *result = format_nn("fsm", i->RT, i->RA);
}

#define _fsm(_ra, _rt) { \
    auto a = th->r(_ra).w<0>(); \
    uint64_t low = a & 0b11; \
    uint64_t high = (a >> 2) & 0b11; \
    uint64_t mask = 0x0000000100000001ull; \
    uint64_t tlow = _pdep_u64(low, mask); \
    uint64_t thigh = _pdep_u64(high, mask); \
    auto t = _mm_set_epi64x(thigh, tlow); \
    auto zero = _mm_set1_epi32(0); \
    t = _mm_cmpgt_epi32(t, zero); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(fsm, i->RA.u(), i->RT.u())


PRINT(gbb) {
    *result = format_nn("gbb", i->RT, i->RA);
}

#define _gbb(_ra, _rt) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    std::bitset<16> bits; \
    for (int i = 0; i < 16; ++i) { \
        bits[15 - i] = ra.b(i) & 1; \
    } \
    rt.set_w(0, bits.to_ulong()); \
    rt.set_w(1, 0); \
    rt.set_w(2, 0); \
    rt.set_w(3, 0); \
}
EMU_REWRITE(gbb, i->RA.u(), i->RT.u())


PRINT(gbh) {
    *result = format_nn("gbh", i->RT, i->RA);
}

#define _gbh(_ra, _rt) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    std::bitset<8> bits; \
    for (int i = 0; i < 8; ++i) { \
        bits[7 - i] = ra.hw(i) & 1; \
    } \
    rt.set_w(0, bits.to_ulong()); \
    rt.set_w(1, 0); \
    rt.set_w(2, 0); \
    rt.set_w(3, 0); \
}
EMU_REWRITE(gbh, i->RA.u(), i->RT.u())


PRINT(gb) {
    *result = format_nn("gb", i->RT, i->RA);
}

#define _gb(_ra, _rt) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    std::bitset<4> bits; \
    for (int i = 0; i < 4; ++i) { \
        bits[3 - i] = ra.w(i) & 1; \
    } \
    rt.set_w(0, bits.to_ulong()); \
    rt.set_w(1, 0); \
    rt.set_w(2, 0); \
    rt.set_w(3, 0); \
}
EMU_REWRITE(gb, i->RA.u(), i->RT.u())


PRINT(avgb) {
    *result = format_nnn("avgb", i->RT, i->RA, i->RB);
}

#define _avgb(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 16; ++i) { \
        auto t = (uint16_t)ra.b(i) + rb.b(i) + 1; \
        rt.set_b(i, signed_rshift32(t, 1)); \
    } \
}
EMU_REWRITE(avgb, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(absdb) {
    *result = format_nnn("absdb", i->RT, i->RA, i->RB);
}

#define _absdb(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 16; ++i) { \
        rt.set_b(i, std::abs(rb.b(i) - ra.b(i))); \
    } \
}
EMU_REWRITE(absdb, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(sumb) {
    *result = format_nnn("sumb", i->RT, i->RA, i->RB);
}

#define _sumb(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        auto bsum = (uint16_t) \
                    rb.b(4 * i + 0) \
                  + rb.b(4 * i + 1) \
                  + rb.b(4 * i + 2) \
                  + rb.b(4 * i + 3); \
        auto asum = (uint16_t) \
                    ra.b(4 * i + 0) \
                  + ra.b(4 * i + 1) \
                  + ra.b(4 * i + 2) \
                  + ra.b(4 * i + 3); \
        rt.set_w(i, (bsum << 16) | asum); \
    } \
}
EMU_REWRITE(sumb, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(xsbh) {
    *result = format_nn("xsbh", i->RT, i->RA);
}

#define _xsbh(_ra, _rt) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 8; ++i) { \
        rt.set_hw(i, (int8_t)ra.b(2 * i + 1)); \
    } \
}
EMU_REWRITE(xsbh, i->RA.u(), i->RT.u())


PRINT(xshw) {
    *result = format_nn("xshw", i->RT, i->RA);
}

#define _xshw(_ra, _rt) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, ra.hw(2 * i + 1)); \
    } \
}
EMU_REWRITE(xshw, i->RA.u(), i->RT.u())


PRINT(xswd) {
    *result = format_nn("xswd", i->RT, i->RA);
}

#define _xswd(_ra, _rt) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 2; ++i) { \
        rt.set_dw(i, ra.w(2 * i + 1)); \
    } \
}
EMU_REWRITE(xswd, i->RA.u(), i->RT.u())


PRINT(and_) {
    *result = format_nnn("and", i->RT, i->RA, i->RB);
}

#define _and_(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = a & b; \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(and_, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(andc) {
    *result = format_nnn("andc", i->RT, i->RA, i->RB);
}

#define _andc(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = _mm_andnot_si128(b, a); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(andc, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(andbi) {
    *result = format_nnn("andbi", i->RT, i->RA, i->I10);
}

#define _andbi(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi8((int8_t)_i10); \
    auto t = a & b; \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(andbi, i->RA.u(), i->RT.u(), i->I10.u())


PRINT(andhi) {
    *result = format_nnn("andhi", i->RT, i->RA, i->I10);
}

#define _andhi(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi16((int16_t)_i10); \
    auto t = a & b; \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(andhi, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(andi) {
    *result = format_nnn("andi", i->RT, i->RA, i->I10);
}

#define _andi(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi32((int32_t)_i10); \
    auto t = a & b; \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(andi, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(or_) {
    *result = format_nnn("or", i->RT, i->RA, i->RB);
}

#define _or_(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = a | b; \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(or_, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(orc) {
    *result = format_nnn("orc", i->RT, i->RA, i->RB);
}

#define _orc(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = a | ~b; \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(orc, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(orbi) {
    *result = format_nnn("orbi", i->RT, i->RA, i->I10);
}

#define _orbi(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi8((int8_t)_i10); \
    auto t = a | b; \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(orbi, i->RA.u(), i->RT.u(), i->I10.u())


PRINT(orhi) {
    *result = format_nnn("orhi", i->RT, i->RA, i->I10);
}

#define _orhi(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi16((int16_t)_i10); \
    auto t = a | b; \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(orhi, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(ori) {
    *result = format_nnn("ori", i->RT, i->RA, i->I10);
}

#define _ori(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto s = _mm_set1_epi32(_i10); \
    auto t = _mm_or_si128(a, s); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(ori, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(orx) {
    *result = format_nn("orx", i->RT, i->RA);
}

#define _orx(_ra, _rt) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    rt.set_w(0, ra.w<0>() | ra.w<1>() | ra.w<2>() | ra.w<3>()); \
    rt.set_w(1, 0); \
    rt.set_dw(1,  0); \
}
EMU_REWRITE(orx, i->RA.u(), i->RT.u())


PRINT(xor_) {
    *result = format_nnn("xor", i->RT, i->RA, i->RB);
}

#define _xor_(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    th->r(_rt).set_xmm(a ^ b); \
}
EMU_REWRITE(xor_, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(xorbi) {
    *result = format_nnn("xorbi", i->RT, i->RA, i->I10);
}

#define _xorbi(_ra, _rt, _i10) { \
    auto mask = _mm_set1_epi8(_i10); \
    auto a = th->r(_ra).xmm(); \
    th->r(_rt).set_xmm(a ^ mask); \
}
EMU_REWRITE(xorbi, i->RA.u(), i->RT.u(), i->I10.u())


PRINT(xorhi) {
    *result = format_nnn("xorhi", i->RT, i->RA, i->I10);
}

#define _xorhi(_ra, _rt, _i10) { \
    auto mask = _mm_set1_epi16(_i10); \
    auto a = th->r(_ra).xmm(); \
    th->r(_rt).set_xmm(a ^ mask); \
}
EMU_REWRITE(xorhi, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(xori) {
    *result = format_nnn("xori", i->RT, i->RA, i->I10);
}

#define _xori(_ra, _rt, _i10) { \
    auto mask = _mm_set1_epi32(_i10); \
    auto a = th->r(_ra).xmm(); \
    th->r(_rt).set_xmm(a ^ mask); \
}
EMU_REWRITE(xori, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(nand) {
    *result = format_nnn("nand", i->RT, i->RA, i->RB);
}

#define _nand(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = ~(a & b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(nand, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(nor) {
    *result = format_nnn("nor", i->RT, i->RA, i->RB);
}

#define _nor(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = ~(a | b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(nor, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(eqv) {
    *result = format_nnn("eqv", i->RT, i->RA, i->RB);
}

#define _eqv(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = b ^ (~a); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(eqv, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(selb) {
    *result = format_nnnn("selb", i->RT_ABC, i->RA, i->RB, i->RC);
}

#define _selb(_ra, _rb, _rc, _rt_abc) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto c = th->r(_rc).xmm(); \
    auto t = (c & b) | (~c & a); \
    th->r(_rt_abc).set_xmm(t); \
}
EMU_REWRITE(selb, i->RA.u(), i->RB.u(), i->RC.u(), i->RT_ABC.u())


PRINT(shufb) {
    *result = format_nnnn("shufb", i->RT_ABC, i->RA, i->RB, i->RC);
}

/*
#define _shufb(_ra, _rb, _rc, _rt_abc) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rc = th->r(_rc); \
    auto& rt = th->r(_rt_abc); \
    for (int i = 0; i < 16; ++i) { \
        auto c = rc.b(i); \
        auto idx = c & 0b11111; \
        rt.b(i) = (c & 0b11000000) == 0b10000000 ? 0 \
                : (c & 0b11100000) == 0b11000000 ? 0xff \
                : (c & 0b11100000) == 0b11100000 ? 0x80 \
                : (idx < 16 ? ra.b(idx) : rb.b(idx % 16)); \
    } \
}
*/

#define _shufb(_ra, _rb, _rc, _rt_abc) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto c = th->r(_rc).xmm(); \
    auto sx = _mm_and_si128(_mm_srli_epi64(c, 5), _mm_set1_epi8(0b111)); \
    auto snx = _mm_and_si128(c, _mm_set1_epi8(0b11111)); \
    snx = _mm_sub_epi8(_mm_set1_epi8(31), snx); \
    auto gt = _mm_cmpgt_epi8(snx, _mm_set1_epi8(15)); \
    auto s1 = snx; \
    auto s2 = _mm_sub_epi8(snx, _mm_set1_epi8(16)); \
    auto d1 = _mm_shuffle_epi8(b, s1); \
    auto d2 = _mm_shuffle_epi8(a, s2); \
    auto x = _mm_set1_epi32(0x80ff0000); \
    auto dx = _mm_shuffle_epi8(x, sx); \
    auto d = _mm_blendv_epi8(d1, d2, gt); \
    d = _mm_blendv_epi8(d, dx, _mm_cmplt_epi8(c, _mm_setzero_si128())); \
    th->r(_rt_abc).set_xmm(d); \
}

EMU_REWRITE(shufb, i->RA.u(), i->RB.u(), i->RC.u(), i->RT_ABC.u())


PRINT(shlh) {
    *result = format_nnn("shlh", i->RT, i->RA, i->RB);
}

#define _shlh(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 8; ++i) { \
        auto sh = (uint16_t)rb.hw(i) & 0b11111; \
        rt.set_hw(i, sh > 15 ? 0 : signed_lshift32(ra.hw(i), sh)); \
    } \
}
EMU_REWRITE(shlh, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(shlhi) {
    *result = format_nnn("shlhi", i->RT, i->RA, i->I7);
}

#define _shlhi(_ra, _rt, _i7) { \
    auto a = th->r(_ra).xmm(); \
    auto t = _mm_slli_epi16(a, _i7); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(shlhi, i->RA.u(), i->RT.u(), i->I7.u())


PRINT(shl) {
    *result = format_nnn("shl", i->RT, i->RA, i->RB);
}

#define _shl(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    b = _mm_and_si128(b, _mm_set1_epi32(0b111111)); \
    auto t = _mm_sllv_epi32(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(shl, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(shli) {
    *result = format_nnn("shli", i->RT, i->RA, i->I7);
}

#define _shli(_ra, _rt, _i7) { \
    auto a = th->r(_ra).xmm(); \
    auto sh = _i7 & 0b111111; \
    if (sh > 31) { \
        auto t = _mm_setzero_si128(); \
        th->r(_rt).set_xmm(t); \
    } else { \
        auto t = _mm_slli_epi32(a, _i7 & 0b111111); \
        th->r(_rt).set_xmm(t); \
    } \
}
EMU_REWRITE(shli, i->RA.u(), i->RT.u(), i->I7.u())


PRINT(shlqbi) {
    *result = format_nnn("shlqbi", i->RT, i->RA, i->RB);
}

#define _shlqbi(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto sh = th->r(_rb).w<0>() & 0b111; \
    auto tail = _mm_extract_epi8(a, 7) >> (8 - sh); \
    auto tail128 = _mm_set_epi8( \
        0, 0, 0, 0, 0, 0, 0, tail, \
        0, 0, 0, 0, 0, 0, 0, 0 \
    ); \
    auto sh128 = _mm_set_epi64x(0, sh); \
    auto t = _mm_sll_epi64(a, sh128); \
    t = _mm_or_si128(t, tail128); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(shlqbi, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(shlqbii) {
    *result = format_nnn("shlqbii", i->RT, i->RA, i->I7);
}

#define _shlqbii(_ra, _rt, _i7) { \
    auto a = th->r(_ra).xmm(); \
    auto sh = _i7 & 0b111; \
    auto tail = _mm_extract_epi8(a, 7) >> (8 - sh); \
    auto tail128 = _mm_set_epi8( \
        0, 0, 0, 0, 0, 0, 0, tail, \
        0, 0, 0, 0, 0, 0, 0, 0 \
    ); \
    auto sh128 = _mm_set_epi64x(0, sh); \
    auto t = _mm_sll_epi64(a, sh128); \
    t = _mm_or_si128(t, tail128); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(shlqbii, i->RA.u(), i->RT.u(), i->I7.u())


PRINT(shlqby) {
    *result = format_nnn("shlqby", i->RT, i->RA, i->RB);
}

#define _shlqby(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto sh = (uint32_t)th->r(_rb).w<0>() & 0b11111; \
    auto t = _mm_shuffle_epi8(a, BYTE_SHIFT_LEFT_SHUFFLE_CONTROL[sh]); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(shlqby, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(shlqbyi) {
    *result = format_nnn("shlqbyi", i->RT, i->RA, i->I7);
}

#ifdef EMU_REWRITER
#define _shlqbyi(_ra, _rt, _i7) { \
    auto a = th->r(_ra).xmm(); \
    constexpr auto sh = _i7 & 0b11111; \
    if constexpr(sh > 15) { \
        auto t = _mm_setzero_si128(); \
        th->r(_rt).set_xmm(t); \
    } else { \
        auto t = _mm_bslli_si128(a, (_i7 & 0b11111)); \
        th->r(_rt).set_xmm(t); \
    } \
}
#else
#define _shlqbyi(_ra, _rt, _i7) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto u128 = make128(ra.dw<0>(), ra.dw<1>()); \
    auto sh = _i7 & 0b11111; \
    u128 = sh > 15 ? 0 : u128 << sh * 8; \
    rt.set_dw(0, u128 >> 64); \
    rt.set_dw(1, u128); \
}
#endif
EMU_REWRITE(shlqbyi, i->RA.u(), i->RT.u(), i->I7.u())


PRINT(shlqbybi) {
    *result = format_nnn("shlqbybi", i->RT, i->RA, i->RB);
}

#define _shlqbybi(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto sh = (uint32_t)th->r(_rb).w<0>() >> 3 & 0b11111; \
    auto t = _mm_shuffle_epi8(a, BYTE_SHIFT_LEFT_SHUFFLE_CONTROL[sh]); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(shlqbybi, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(roth) {
    *result = format_nnn("roth", i->RT, i->RA, i->RB);
}

#define _roth(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 8; ++i) { \
        auto sh = rb.hw(i) & 0b1111; \
        rt.set_hw(i, rol<uint16_t>(ra.hw(i), sh)); \
    } \
}
EMU_REWRITE(roth, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(rothi) {
    *result = format_nnn("rothi", i->RT, i->RA, i->I7);
}

#ifdef EMU_REWRITER
#define _rothi(_ra, _rt, _i7) { \
    auto a = th->r(_ra).xmm(); \
    auto sh = _i7 & 0b1111; \
    auto left = _mm_slli_epi16(a, sh); \
    auto right = _mm_srli_epi16(a, -sh & 0b1111); \
    auto t = _mm_or_si128(left, right); \
    th->r(_rt).set_xmm(t); \
}
#else
#define _rothi(_ra, _rt, _i7) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto sh = _i7 & 0b1111; \
    for (int i = 0; i < 8; ++i) { \
        rt.set_hw(i, rol<uint16_t>(ra.hw(i), sh)); \
    } \
}
#endif
EMU_REWRITE(rothi, i->RA.u(), i->RT.u(), i->I7.u())


PRINT(rot) {
    *result = format_nnn("rot", i->RT, i->RA, i->RB);
}

#define _rot(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        auto sh = (uint32_t)rb.w(i) & 0b11111; \
        rt.set_w(i, rol<uint32_t>(ra.w(i), sh)); \
    } \
}
EMU_REWRITE(rot, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(roti) {
    *result = format_nnn("roti", i->RT, i->RA, i->I7);
}

#define _roti(_ra, _rt, _i7) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto sh = _i7 & 0b11111; \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, rol<uint32_t>(ra.w(i), sh)); \
    } \
}
EMU_REWRITE(roti, i->RA.u(), i->RT.u(), i->I7.u())


PRINT(rotqby) {
    *result = format_nnn("rotqby", i->RT, i->RA, i->RB);
}

#define _rotqby(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto sh = (uint32_t)th->r(_rb).w<0>() & 0b1111; \
    auto t = _mm_shuffle_epi8(a, BYTE_ROTATE_LEFT_SHUFFLE_CONTROL[sh]); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(rotqby, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(rotqbyi) {
    *result = format_nnn("rotqbyi", i->RT, i->RA, i->I7);
}

#define _rotqbyi(_ra, _rt, _i7) { \
    auto a = th->r(_ra).xmm(); \
    auto sh = _i7 & 0b1111; \
    auto t = _mm_shuffle_epi8(a, BYTE_ROTATE_LEFT_SHUFFLE_CONTROL[sh]); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(rotqbyi, i->RA.u(), i->RT.u(), i->I7.u())


PRINT(rotqbybi) {
    *result = format_nnn("rotqbybi", i->RT, i->RA, i->RB);
}

#define _rotqbybi(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto sh = ((uint32_t)th->r(_rb).w<0>() >> 3) & 0b1111; \
    auto t = _mm_shuffle_epi8(a, BYTE_ROTATE_LEFT_SHUFFLE_CONTROL[sh]); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(rotqbybi, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(rotqbi) {
    *result = format_nnn("rotqbi", i->RT, i->RA, i->RB);
}

#define _rotqbi(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    auto u128 = make128(ra.dw<0>(), ra.dw<1>()); \
    auto sh = (uint32_t)rb.w<0>() & 0b111; \
    u128 = rol<uint128_t>(u128, sh); \
    rt.set_dw(0,  u128 >> 64); \
    rt.set_dw(1,  u128); \
}
EMU_REWRITE(rotqbi, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(rotqbii) {
    *result = format_nnn("rotqbii", i->RT, i->RA, i->I7);
}

#define _rotqbii(_ra, _rt, _i7) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto u128 = make128(ra.dw<0>(), ra.dw<1>()); \
    auto sh = _i7 & 0b111; \
    u128 = rol<uint128_t>(u128, sh); \
    rt.set_dw(0,  u128 >> 64); \
    rt.set_dw(1,  u128); \
}
EMU_REWRITE(rotqbii, i->RA.u(), i->RT.u(), i->I7.u())


PRINT(rothm) {
    *result = format_nnn("rothm", i->RT, i->RA, i->RB);
}

#define _rothm(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 8; ++i) { \
        auto sh = -rb.hw(i) & 0x1f; \
        rt.set_hw(i, sh < 16 ? (uint16_t)ra.hw(i) >> sh : 0); \
    } \
}
EMU_REWRITE(rothm, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(rothmi) {
    *result = format_nnn("rothmi", i->RT, i->RA, i->I7);
}

#define _rothmi(_ra, _rt, _i7) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto sh = -_i7 & 0x1f; \
    for (int i = 0; i < 8; ++i) { \
        rt.set_hw(i, sh < 16 ? (uint16_t)ra.hw(i) >> sh : 0); \
    } \
}
EMU_REWRITE(rothmi, i->RA.u(), i->RT.u(), i->I7.s())


PRINT(rotm) {
    *result = format_nnn("rotm", i->RT, i->RA, i->RB);
}

#define _rotm(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        auto sh = -rb.w(i) & 0x3f; \
        rt.set_w(i, sh < 32 ? (uint32_t)ra.w(i) >> sh : 0); \
    } \
}
EMU_REWRITE(rotm, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(rotmi) {
    *result = format_nnn("rotmi", i->RT, i->RA, i->I7);
}

#define _rotmi(_ra, _rt, _i7) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto sh = -_i7 & 0x3f; \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, sh < 32 ? (uint32_t)ra.w(i) >> sh : 0); \
    } \
}
EMU_REWRITE(rotmi, i->RA.u(), i->RT.u(), i->I7.s())


PRINT(rotqmby) {
    *result = format_nnn("rotqmby", i->RT, i->RA, i->RB);
}

#define _rotqmby(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    auto u128 = make128(ra.dw<0>(), ra.dw<1>()); \
    auto sh = -rb.w<0>() & 0x1f; \
    u128 = sh > 15 ? 0 : u128 >> sh * 8; \
    rt.set_dw(0,  u128 >> 64); \
    rt.set_dw(1,  u128); \
}
EMU_REWRITE(rotqmby, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(rotqmbyi) {
    *result = format_nnn("rotqmbyi", i->RT, i->RA, i->I7);
}

#define _rotqmbyi(_ra, _rt, _i7) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto u128 = make128(ra.dw<0>(), ra.dw<1>()); \
    auto sh = -_i7 & 0x1f; \
    u128 = sh > 15 ? 0 : u128 >> sh * 8; \
    rt.set_dw(0,  u128 >> 64); \
    rt.set_dw(1,  u128); \
}
EMU_REWRITE(rotqmbyi, i->RA.u(), i->RT.u(), i->I7.s())


PRINT(rotqmbybi) {
    *result = format_nnn("rotqmbybi", i->RT, i->RA, i->RB);
}

#define _rotqmbybi(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    auto u128 = make128(ra.dw<0>(), ra.dw<1>()); \
    auto sh = -(rb.w<0>() >> 3) & 0b11111; \
    u128 = sh < 16 ? u128 >> sh * 8 : make128(0, 0); \
    rt.set_dw(0,  u128 >> 64); \
    rt.set_dw(1,  u128); \
}
EMU_REWRITE(rotqmbybi, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(rotqmbi) {
    *result = format_nnn("rotqmbi", i->RT, i->RA, i->RB);
}

#define _rotqmbi(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    auto u128 = make128(ra.dw<0>(), ra.dw<1>()); \
    u128 >>= -rb.w<0>() & 7; \
    rt.set_dw(0,  u128 >> 64); \
    rt.set_dw(1,  u128); \
}
EMU_REWRITE(rotqmbi, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(rotqmbii) {
    *result = format_nnn("rotqmbii", i->RT, i->RA, i->I7);
}

#define _rotqmbii(_ra, _rt, _i7) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto u128 = make128(ra.dw<0>(), ra.dw<1>()); \
    u128 >>= -_i7 & 7; \
    rt.set_dw(0,  u128 >> 64); \
    rt.set_dw(1,  u128); \
}
EMU_REWRITE(rotqmbii, i->RA.u(), i->RT.u(), i->I7.s())


PRINT(rotmah) {
    *result = format_nnn("rotmah", i->RT, i->RA, i->RB);
}

#define _rotmah(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 8; ++i) { \
        auto sh = -rb.hw(i) & 0x1f; \
        rt.set_hw(i, sh < 16 ? signed_rshift32(ra.hw(i), sh) \
                 : ((ra.hw(i) & (1 << 15)) ? ~0 : 0)); \
    } \
}
EMU_REWRITE(rotmah, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(rotmahi) {
    *result = format_nnn("rotmahi", i->RT, i->RA, i->I7);
}

#define _rotmahi(_ra, _rt, _i7) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto sh = -_i7 & 0x1f; \
    for (int i = 0; i < 8; ++i) { \
        rt.set_hw(i, sh < 16 ? signed_rshift32(ra.hw(i), sh) \
                 : ((ra.hw(i) & (1 << 15)) ? ~0 : 0)); \
    } \
}
EMU_REWRITE(rotmahi, i->RA.u(), i->RT.u(), i->I7.s())


PRINT(rotma) {
    *result = format_nnn("rotma", i->RT, i->RA, i->RB);
}

#define _rotma(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        auto sh = -rb.w(i) & 0x3f; \
        rt.set_w(i, sh < 32 ? signed_rshift32(ra.w(i), sh) \
                : ((ra.w(i) & (1 << 31)) ? ~0u : 0)); \
    } \
}
EMU_REWRITE(rotma, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(rotmai) {
    *result = format_nnn("rotmai", i->RT, i->RA, i->I7);
}

#define _rotmai(_ra, _rt, _i7) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto sh = -_i7 & 0x3f; \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, sh < 32 ? signed_rshift32(ra.w(i), sh) \
                : ((ra.w(i) & (1 << 31)) ? ~0u : 0)); \
    } \
}
EMU_REWRITE(rotmai, i->RA.u(), i->RT.u(), i->I7.s())


PRINT(heq) {
    *result = format_nnn("heq", i->RT, i->RA, i->RB);
}

#define _heq(_ra, _rb, _cia) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    if (ra.w<0>() == rb.w<0>()) {\
        SPU_RESTORE_NIP(_cia); \
        throw BreakpointException(); \
    } \
}
EMU_REWRITE(heq, i->RA.u(), i->RB.u(), cia)


PRINT(heqi) {
    *result = format_nnn("heqi", i->RT, i->RA, i->I10);
}

#define _heqi(_ra, _i10, _cia) { \
    auto ra = th->r(_ra); \
    if (ra.w<0>() == _i10) { \
        SPU_RESTORE_NIP(_cia); \
        throw BreakpointException(); \
    } \
}
EMU_REWRITE(heqi, i->RA.u(), i->I10.s(), cia)


PRINT(hgt) {
    *result = format_nnn("hgt", i->RT, i->RA, i->RB);
}

#define _hgt(_ra, _rb, _cia) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    if (ra.w<0>() > rb.w<0>()) { \
        SPU_RESTORE_NIP(_cia); \
        throw BreakpointException(); \
    } \
}
EMU_REWRITE(hgt, i->RA.u(), i->RB.u(), cia)


PRINT(hgti) {
    *result = format_nnn("hgti", i->RT, i->RA, i->I10);
}

#define _hgti(_ra, _i10, _cia) { \
    auto ra = th->r(_ra); \
    if (ra.w<0>() > _i10) { \
        SPU_RESTORE_NIP(_cia); \
        throw BreakpointException(); \
    } \
}
EMU_REWRITE(hgti, i->RA.u(), i->I10.s(), cia)


PRINT(hlgt) {
    *result = format_nnn("hlgt", i->RT, i->RA, i->RB);
}

#define _hlgt(_ra, _rb, _cia) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    if ((uint32_t)ra.w<0>() > (uint32_t)rb.w<0>()) { \
        SPU_RESTORE_NIP(_cia); \
        throw BreakpointException(); \
    } \
}
EMU_REWRITE(hlgt, i->RA.u(), i->RB.u(), cia)


PRINT(hlgti) {
    *result = format_nnn("hlgti", i->RT, i->RA, i->I10);
}

#define _hlgti(_ra, _i10, _cia) { \
    auto ra = th->r(_ra); \
    if ((uint32_t)ra.w<0>() > (uint32_t)_i10) { \
        SPU_RESTORE_NIP(_cia); \
        throw BreakpointException(); \
    } \
}
EMU_REWRITE(hlgti, i->RA.u(), i->I10.s(), cia)


PRINT(ceqb) {
    *result = format_nnn("ceqb", i->RT, i->RA, i->RB);
}

#define _ceqb(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 16; ++i) { \
        rt.set_b(i, ra.b(i) == rb.b(i) ? 0xff : 0); \
    } \
}
EMU_REWRITE(ceqb, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(ceqbi) {
    *result = format_nnn("ceqbi", i->RT, i->RA, i->I10);
}

#define _ceqbi(_ra, _rt, _i10) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto imm = (uint8_t)_i10; \
    for (int i = 0; i < 16; ++i) { \
        rt.set_b(i, ra.b(i) == imm ? 0xff : 0); \
    } \
}
EMU_REWRITE(ceqbi, i->RA.u(), i->RT.u(), i->I10.u())


PRINT(ceqh) {
    *result = format_nnn("ceqh", i->RT, i->RA, i->RB);
}

#define _ceqh(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 8; ++i) { \
        rt.set_hw(i, ra.hw(i) == rb.hw(i) ? 0xffff : 0); \
    } \
}
EMU_REWRITE(ceqh, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(ceqhi) {
    *result = format_nnn("ceqhi", i->RT, i->RA, i->I10);
}

#define _ceqhi(_ra, _rt, _i10) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    int16_t imm = _i10; \
    for (int i = 0; i < 8; ++i) { \
        rt.set_hw(i, ra.hw(i) == imm ? 0xffff : 0); \
    } \
}
EMU_REWRITE(ceqhi, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(ceq) {
    *result = format_nnn("ceq", i->RT, i->RA, i->RB);
}

#define _ceq(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    auto t = _mm_cmpeq_epi32(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(ceq, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(ceqi) {
    *result = format_nnn("ceqi", i->RT, i->RA, i->I10);
}

#define _ceqi(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    int32_t imm = _i10; \
    auto b = _mm_set1_epi32(imm); \
    auto t = _mm_cmpeq_epi32(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(ceqi, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(cgtb) {
    *result = format_nnn("cgtb", i->RT, i->RA, i->RB);
}

#define _cgtb(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 16; ++i) { \
        rt.set_b(i, (int8_t)ra.b(i) > (int8_t)rb.b(i) ? 0xff : 0); \
    } \
}
EMU_REWRITE(cgtb, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(cgtbi) {
    *result = format_nnn("cgtbi", i->RT, i->RA, i->I10);
}

#define _cgtbi(_ra, _rt, _i10) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    int8_t imm = _i10 & 0xff; \
    for (int i = 0; i < 16; ++i) { \
        rt.set_b(i, (int8_t)ra.b(i) > imm ? 0xff : 0); \
    } \
}
EMU_REWRITE(cgtbi, i->RA.u(), i->RT.u(), i->I10.u())


PRINT(cgth) {
    *result = format_nnn("cgth", i->RT, i->RA, i->RB);
}

#define _cgth(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 8; ++i) { \
        rt.set_hw(i, ra.hw(i) > rb.hw(i) ? 0xffff : 0); \
    } \
}
EMU_REWRITE(cgth, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(cgthi) {
    *result = format_nnn("cgthi", i->RT, i->RA, i->I10);
}

#define _cgthi(_ra, _rt, _i10) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    int16_t imm = _i10; \
    for (int i = 0; i < 8; ++i) { \
        rt.set_hw(i, ra.hw(i) > imm ? 0xffff : 0); \
    } \
}
EMU_REWRITE(cgthi, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(cgt) {
    *result = format_nnn("cgt", i->RT, i->RA, i->RB);
}

#define _cgt(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, ra.w(i) > rb.w(i) ? 0xffffffff : 0); \
    } \
}
EMU_REWRITE(cgt, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(cgti) {
    *result = format_nnn("cgti", i->RT, i->RA, i->I10);
}

#define _cgti(_ra, _rt, _i10) { \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi32(_i10); \
    auto t = _mm_cmpgt_epi32(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(cgti, i->RA.u(), i->RT.u(), i->I10.s())


PRINT(clgtb) {
    *result = format_nnn("clgtb", i->RT, i->RA, i->RB);
}

#define _clgtb(_ra, _rb, _rt) { \
    auto val = _mm_set1_epi8(0x80); \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    a = _mm_sub_epi8(a, val); \
    b = _mm_sub_epi8(b, val); \
    auto t = _mm_cmpgt_epi8(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(clgtb, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(clgtbi) {
    *result = format_nnn("cgtbi", i->RT, i->RA, i->I10);
}

#define _clgtbi(_ra, _rt, _i10) { \
    auto val = _mm_set1_epi8(0x80); \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi8(char(_i10 - 0x80u)); \
    a = _mm_sub_epi8(a, val); \
    auto t = _mm_cmpgt_epi8(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(clgtbi, i->RA.u(), i->RT.u(), i->I10.u())


PRINT(clgth) {
    *result = format_nnn("clgth", i->RT, i->RA, i->RB);
}

#define _clgth(_ra, _rb, _rt) { \
    auto val = _mm_set1_epi16(0x8000); \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    a = _mm_sub_epi16(a, val); \
    b = _mm_sub_epi16(b, val); \
    auto t = _mm_cmpgt_epi16(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(clgth, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(clgthi) {
    *result = format_nnn("clgthi", i->RT, i->RA, i->I10);
}

#define _clgthi(_ra, _rt, _i10) { \
    auto val = _mm_set1_epi16(0x8000); \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi16((uint16_t)_i10 - (uint16_t)0x8000); \
    a = _mm_sub_epi16(a, val); \
    auto t = _mm_cmpgt_epi16(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(clgthi, i->RA.u(), i->RT.u(), i->I10.u())


PRINT(clgt) {
    *result = format_nnn("clgt", i->RT, i->RA, i->RB);
}

#define _clgt(_ra, _rb, _rt) { \
    auto val = _mm_set1_epi32(0x80000000); \
    auto a = th->r(_ra).xmm(); \
    auto b = th->r(_rb).xmm(); \
    a = _mm_sub_epi32(a, val); \
    b = _mm_sub_epi32(b, val); \
    auto t = _mm_cmpgt_epi32(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(clgt, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(clgti) {
    *result = format_nnn("clgti", i->RT, i->RA, i->I10);
}

#define _clgti(_ra, _rt, _i10) { \
    auto val = _mm_set1_epi32(0x80000000); \
    auto a = th->r(_ra).xmm(); \
    auto b = _mm_set1_epi32(_i10 - 0x80000000); \
    a = _mm_sub_epi32(a, val); \
    auto t = _mm_cmpgt_epi32(a, b); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(clgti, i->RA.u(), i->RT.u(), i->I10.u())


PRINT(br) {
    int32_t offset = signed_lshift32(i->I16.s(), 2);
    *result = format_u("br", (cia + offset) & LSLR);
}

#define _br(_i16, _cia, _dest) { \
    int32_t offset = signed_lshift32(_i16, 2); \
    if (offset == 0) { \
        SPU_RESTORE_NIP(_cia); \
        throw InfiniteLoopException(); \
    } \
    SPU_SET_NIP(_dest); \
}
EMU_REWRITE(br, i->I16.s(), cia, (cia + signed_lshift32(i->I16.s(), 2)) & LSLR)


PRINT(bra) {
    int32_t address = signed_lshift32(i->I16.s(), 2);
    *result = format_u("bra", address & LSLR);
}

#define _bra(_dest) { \
    SPU_SET_NIP(_dest); \
}
EMU_REWRITE(bra, signed_lshift32(i->I16.s(), 2) & LSLR)


PRINT(brsl) {
    int32_t offset = signed_lshift32(i->I16.s(), 2);
    *result = format_nu("brsl", i->RT, (cia + offset) & LSLR);
}

#define _brsl(_i16, _rt, _cia, _dest) { \
    auto t = _mm_set_epi32(SPU_ADJUST_LINK((_cia + 4) & LSLR), 0, 0, 0); \
    th->r(_rt).set_xmm(t); \
    SPU_SET_NIP(_dest); \
}
EMU_REWRITE(brsl, i->I16.s(), i->RT.u(), cia, (signed_lshift32(i->I16.s(), 2) + cia) & LSLR)


PRINT(brasl) {
    int32_t address = signed_lshift32(i->I16.s(), 2);
    *result = format_u("brasl", address & LSLR);
}

#define _brasl(_i16, _rt, _cia, _dest) { \
    auto t = _mm_set_epi32(SPU_ADJUST_LINK((_cia + 4) & LSLR), 0, 0, 0); \
    th->r(_rt).set_xmm(t); \
    SPU_SET_NIP(_dest); \
}
EMU_REWRITE(brasl, i->I16.s(), i->RT.u(), cia, (signed_lshift32(i->I16.s(), 2) + cia) & LSLR)


PRINT(bi) {
    *result = format_n("bi", i->RA);
}

#define _bi(_ra) { \
    SPU_SET_NIP_INDIRECT(th->r(_ra).w<0>() & LSLR & 0xfffffffc); \
}
EMU_REWRITE(bi, i->RA.u())


PRINT(iret) {
    *result = format_n("iret", i->RA);
}

#define _iret(_) { \
    SPU_SET_NIP_INDIRECT(th->getSrr0()); \
    assert(false); \
}
EMU_REWRITE(iret, 0)


PRINT(bisl) {
    *result = format_nn("bisl", i->RT, i->RA);
}

#define _bisl(_ra, _rt, _cia) { \
    auto t = _mm_set_epi32(SPU_ADJUST_LINK((_cia + 4) & LSLR), 0, 0, 0); \
    th->r(_rt).set_xmm(t); \
    SPU_SET_NIP_INDIRECT(th->r(_ra).w<0>() & LSLR & 0xfffffffc); \
}
EMU_REWRITE(bisl, i->RA.u(), i->RT.u(), cia)


inline uint32_t br_cia_lsa(I16_t i16, uint32_t cia) {
    return ((i16 << 2) + cia) & LSLR & 0xfffffffc;
}

PRINT(brnz) {
    *result = format_nu("brnz", i->RT, br_cia_lsa(i->I16, cia));
}

#define _brnz(_rt, _br_cia_lsa) { \
    if (th->r(_rt).w<0>() != 0) { \
        SPU_SET_NIP(_br_cia_lsa); \
    } \
}
EMU_REWRITE(brnz, i->RT.u(), br_cia_lsa(i->I16, cia))


PRINT(brz) {
    *result = format_nu("brz", i->RT, br_cia_lsa(i->I16, cia));
}

#define _brz(_rt, _br_cia_lsa) { \
    if (th->r(_rt).w<0>() == 0) { \
        SPU_SET_NIP(_br_cia_lsa); \
    } \
}
EMU_REWRITE(brz, i->RT.u(), br_cia_lsa(i->I16, cia))


PRINT(brhnz) {
    *result = format_nu("brhnz", i->RT, br_cia_lsa(i->I16, cia));
}

#define _brhnz(_rt, _br_cia_lsa) { \
    if (th->r(_rt).hw_pref() != 0) { \
        SPU_SET_NIP(_br_cia_lsa); \
    } \
}
EMU_REWRITE(brhnz, i->RT.u(), br_cia_lsa(i->I16, cia))


PRINT(brhz) {
    *result = format_nu("brhz", i->RT, br_cia_lsa(i->I16, cia));
}

#define _brhz(_rt, _br_cia_lsa) { \
    if (th->r(_rt).hw_pref() == 0) { \
        SPU_SET_NIP(_br_cia_lsa); \
    } \
}
EMU_REWRITE(brhz, i->RT.u(), br_cia_lsa(i->I16, cia))


PRINT(biz) {
    *result = format_nn("biz", i->RT, i->RA);
}

#define _biz(_rt, _ra) { \
    if (th->r(_rt).w<0>() == 0) { \
        auto address = th->r(_ra).w<0>() & LSLR & 0xfffffffc; \
        SPU_SET_NIP_INDIRECT(address); \
    } \
}
EMU_REWRITE(biz, i->RT.u(), i->RA.u())


PRINT(binz) {
    *result = format_nn("binz", i->RT, i->RA);
}

#define _binz(_rt, _ra) { \
    if (th->r(_rt).w<0>() != 0) { \
        auto address = th->r(_ra).w<0>() & LSLR & 0xfffffffc; \
        SPU_SET_NIP_INDIRECT(address); \
    } \
}
EMU_REWRITE(binz, i->RT.u(), i->RA.u())


PRINT(bihz) {
    *result = format_nn("bihz", i->RT, i->RA);
}

#define _bihz(_rt, _ra) { \
    if (th->r(_rt).hw_pref() == 0) { \
        auto address = th->r(_ra).w<0>() & LSLR & 0xfffffffc; \
        SPU_SET_NIP_INDIRECT(address); \
    } \
}
EMU_REWRITE(bihz, i->RT.u(), i->RA.u())


PRINT(bihnz) {
    *result = format_nn("bihnz", i->RT, i->RA);
}

#define _bihnz(_rt, _ra) { \
    if (th->r(_rt).hw_pref() != 0) { \
        auto address = th->r(_ra).w<0>() & LSLR & 0xfffffffc; \
        SPU_SET_NIP_INDIRECT(address); \
    } \
}
EMU_REWRITE(bihnz, i->RT.u(), i->RA.u())


PRINT(hbr) {
    *result = "hbr";
}

#define _hbr(_) { \
}
EMU_REWRITE(hbr, 0)


PRINT(hbra) {
    *result = "hbra";
}

#define _hbra(_) { \
}
EMU_REWRITE(hbra, 0)


PRINT(hbrr) {
    *result = "hbrr";
}

#define _hbrr(_) { \
}
EMU_REWRITE(hbrr, 0)


PRINT(fa) {
    *result = format_nnn("fa", i->RT, i->RA, i->RB);
}

#define _fa(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm_f(); \
    auto b = th->r(_rb).xmm_f(); \
    auto t = _mm_add_ps(a, b); \
    th->r(_rt).set_xmm_f(t); \
}
EMU_REWRITE(fa, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(dfa) {
    *result = format_nnn("dfa", i->RT, i->RA, i->RB);
}

#define _dfa(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm_d(); \
    auto b = th->r(_rb).xmm_d(); \
    auto t = _mm_add_pd(a, b); \
    th->r(_rt).set_xmm_d(t); \
}
EMU_REWRITE(dfa, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(fs) {
    *result = format_nnn("fs", i->RT, i->RA, i->RB);
}

#define _fs(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm_f(); \
    auto b = th->r(_rb).xmm_f(); \
    auto t = _mm_sub_ps(a, b); \
    th->r(_rt).set_xmm_f(t); \
}
EMU_REWRITE(fs, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(dfs) {
    *result = format_nnn("dfs", i->RT, i->RA, i->RB);
}

#define _dfs(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm_d(); \
    auto b = th->r(_rb).xmm_d(); \
    auto t = _mm_sub_pd(a, b); \
    th->r(_rt).set_xmm_d(t); \
}
EMU_REWRITE(dfs, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(fm) {
    *result = format_nnn("fm", i->RT, i->RA, i->RB);
}

#define _fm(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm_f(); \
    auto b = th->r(_rb).xmm_f(); \
    auto t = _mm_mul_ps(a, b); \
    th->r(_rt).set_xmm_f(t); \
}
EMU_REWRITE(fm, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(dfm) {
    *result = format_nnn("dfm", i->RT, i->RA, i->RB);
}

#define _dfm(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm_d(); \
    auto b = th->r(_rb).xmm_d(); \
    auto t = _mm_mul_pd(a, b); \
    th->r(_rt).set_xmm_d(t); \
}
EMU_REWRITE(dfm, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(fma) {
    *result = format_nnnn("fma", i->RT_ABC, i->RA, i->RB, i->RC);
}

#define _fma(_ra, _rb, _rc, _rt_abc) { \
    auto a = th->r(_ra).xmm_f(); \
    auto b = th->r(_rb).xmm_f(); \
    auto c = th->r(_rc).xmm_f(); \
    auto t = _mm_fmadd_ps(a, b, c); \
    th->r(_rt_abc).set_xmm_f(t); \
}
EMU_REWRITE(fma, i->RA.u(), i->RB.u(), i->RC.u(), i->RT_ABC.u())


PRINT(dfma) {
    *result = format_nnnn("dfma", i->RT_ABC, i->RA, i->RB, i->RC);
}

#define _dfma(_ra, _rb, _rc, _rt_abc) { \
    auto a = th->r(_ra).xmm_d(); \
    auto b = th->r(_rb).xmm_d(); \
    auto c = th->r(_rc).xmm_d(); \
    auto t = _mm_fmadd_pd(a, b, c); \
    th->r(_rt_abc).set_xmm_d(t); \
}
EMU_REWRITE(dfma, i->RA.u(), i->RB.u(), i->RC.u(), i->RT_ABC.u())


PRINT(fnms) {
    *result = format_nnnn("fnms", i->RT_ABC, i->RA, i->RB, i->RC);
}

#define _fnms(_ra, _rb, _rc, _rt_abc) { \
    auto a = th->r(_ra).xmm_f(); \
    auto b = th->r(_rb).xmm_f(); \
    auto c = th->r(_rc).xmm_f(); \
    auto t = _mm_fmsub_ps(a, b, c); \
    t = _mm_xor_ps(t, _mm_set1_ps(-0.f)); \
    th->r(_rt_abc).set_xmm_f(t); \
}
EMU_REWRITE(fnms, i->RA.u(), i->RB.u(), i->RC.u(), i->RT_ABC.u())


PRINT(dfnms) {
    *result = format_nnnn("dfnms", i->RT_ABC, i->RA, i->RB, i->RC);
}

#define _dfnms(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm_d(); \
    auto b = th->r(_rb).xmm_d(); \
    auto t = th->r(_rt).xmm_d(); \
    t = _mm_fmsub_pd(a, b, t); \
    t = _mm_xor_pd(t, _mm_set1_pd(-0.f)); \
    th->r(_rt).set_xmm_d(t); \
}
EMU_REWRITE(dfnms, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(fms) {
    *result = format_nnnn("fms", i->RT_ABC, i->RA, i->RB, i->RC);
}

#define _fms(_ra, _rb, _rc, _rt_abc) { \
    auto a = th->r(_ra).xmm_f(); \
    auto b = th->r(_rb).xmm_f(); \
    auto c = th->r(_rc).xmm_f(); \
    auto t = _mm_fmsub_ps(a, b, c); \
    th->r(_rt_abc).set_xmm_f(t); \
}
EMU_REWRITE(fms, i->RA.u(), i->RB.u(), i->RC.u(), i->RT_ABC.u())


PRINT(dfms) {
    *result = format_nnnn("dfms", i->RT_ABC, i->RA, i->RB, i->RC);
}

#define _dfms(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm_d(); \
    auto b = th->r(_rb).xmm_d(); \
    auto t = th->r(_rt).xmm_d(); \
    t = _mm_fmsub_pd(a, b, t); \
    th->r(_rt).set_xmm_d(t); \
}
EMU_REWRITE(dfms, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(dfnma) {
    *result = format_nnn("dfnma", i->RT, i->RA, i->RB);
}

#define _dfnma(_ra, _rb, _rt) { \
    auto a = th->r(_ra).xmm_d(); \
    auto b = th->r(_rb).xmm_d(); \
    auto t = th->r(_rt).xmm_d(); \
    t = _mm_fmadd_pd(a, b, t); \
    t = _mm_xor_pd(t, _mm_set1_pd(-0.f)); \
    th->r(_rt).set_xmm_d(t); \
}
EMU_REWRITE(dfnma, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(frest) {
    *result = format_nn("frest", i->RT, i->RA);
}

#define _frest(_ra, _rt) { \
    auto a = th->r(_ra).xmm_f(); \
    auto t = _mm_rcp_ps(a); \
    th->r(_rt).set_xmm_f(t); \
}
EMU_REWRITE(frest, i->RA.u(), i->RT.u())


PRINT(frsqest) {
    *result = format_nn("frsqest", i->RT, i->RA);
}

#define _frsqest(_ra, _rt) { \
    auto a = th->r(_ra).xmm_f(); \
    auto abs_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)); \
    a = _mm_and_ps(a, abs_mask); \
    auto t = _mm_rsqrt_ps(a); \
    th->r(_rt).set_xmm_f(t); \
}
EMU_REWRITE(frsqest, i->RA.u(), i->RT.u())


PRINT(fi) {
    *result = format_nnn("fi", i->RT, i->RA, i->RB);
}

// skip Newton-Raphson's second step
#define _fi(_rb, _rt) { \
    auto b = th->r(_rb).xmm_f(); \
    th->r(_rt).set_xmm_f(b); \
}
EMU_REWRITE(fi, i->RB.u(), i->RT.u())


PRINT(csflt) {
    *result = format_nnu("csflt", i->RT, i->RA, 155 - i->I8.u());
}

#define _csflt(_ra, _rt, _i8) { \
    auto a = th->r(_ra).xmm(); \
    auto div = std::pow(2.f, float(155 - _i8)); \
    auto div128 = _mm_set1_ps(div); \
    auto t = _mm_cvtepi32_ps(a); \
    t = _mm_div_ps(t, div128); \
    th->r(_rt).set_xmm_f(t); \
}
EMU_REWRITE(csflt, i->RA.u(), i->RT.u(), i->I8.u())


PRINT(cflts) {
    *result = format_nnu("cflts", i->RT, i->RA, 173 - i->I8.u());
}

#define _cflts(_ra, _rt, _i8) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto mul = std::pow(2.f, float(173 - _i8)); \
    for (int i = 0; i < 4; ++i) { \
        auto f = ra.fs(i) * mul; \
        auto s = f <= float(INT32_MIN) ? INT32_MIN \
               : f >= float(INT32_MAX) ? INT32_MAX : (int32_t)f; \
        rt.set_w(i, s); \
    } \
}
EMU_REWRITE(cflts, i->RA.u(), i->RT.u(), i->I8.u())


PRINT(cuflt) {
    *result = format_nnu("cuflt", i->RT, i->RA, 155 - i->I8.u());
}

#define _cuflt(_ra, _rt, _i8) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    uint128_t scale = 1; \
    scale <<= 155 - _i8; \
    for (int i = 0; i < 4; ++i) { \
        rt.set_fs(i, (float)(uint32_t)ra.w(i) / scale); \
    } \
}
EMU_REWRITE(cuflt, i->RA.u(), i->RT.u(), i->I8.u())


PRINT(cfltu) {
    *result = format_nnu("cfltu", i->RT, i->RA, 173 - i->I8.u());
}

#define _cfltu(_ra, _rt, _i8) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto mul = std::pow(2.f, float(173 - _i8)); \
    for (int i = 0; i < 4; ++i) { \
        auto f = ra.fs(i) * mul; \
        auto u = f < 0.f ? 0u \
               : f >= float(UINT32_MAX) ? UINT32_MAX : (uint32_t)f; \
        rt.set_w(i, u); \
    } \
}
EMU_REWRITE(cfltu, i->RA.u(), i->RT.u(), i->I8.u())


PRINT(frds) {
    *result = format_nn("frds", i->RT, i->RA);
}

#define _frds(_ra, _rt) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 2; ++i) { \
        rt.set_fs(i * 2, ra.fd(i)); \
        rt.set_w(i * 2 + 1, 0); \
    } \
}
EMU_REWRITE(frds, i->RA.u(), i->RT.u())


PRINT(fesd) {
    *result = format_nn("fesd", i->RT, i->RA);
}

#define _fesd(_ra, _rt) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 2; ++i) { \
        rt.set_fd(i, ra.fs(2 * i)); \
    } \
}
EMU_REWRITE(fesd, i->RA.u(), i->RT.u())


PRINT(dfceq) {
    *result = format_nnn("dfceq", i->RT, i->RA, i->RB);
}

#define _dfceq(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 2; ++i) { \
        rt.set_dw(i, ra.fd(i) == rb.fd(i) ? ~0ull : 0); \
    } \
}
EMU_REWRITE(dfceq, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(dfcmeq) {
    *result = format_nnn("dfcmeq", i->RT, i->RA, i->RB);
}

#define _dfcmeq(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 2; ++i) { \
        rt.set_dw(i, std::abs(ra.fd(i)) == std::abs(rb.fd(i)) ? ~0ull : 0); \
    } \
}
EMU_REWRITE(dfcmeq, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(dfcgt) {
    *result = format_nnn("dfcgt", i->RT, i->RA, i->RB);
}

#define _dfcgt(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 2; ++i) { \
        rt.set_dw(i, ra.fd(i) > rb.fd(i) ? ~0ull : 0); \
    } \
}
EMU_REWRITE(dfcgt, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(dfcmgt) {
    *result = format_nnn("dfcmgt", i->RT, i->RA, i->RB);
}

#define _dfcmgt(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 2; ++i) { \
        rt.set_dw(i, std::abs(ra.fd(i)) > std::abs(rb.fd(i)) ? ~0ull : 0); \
    } \
}
EMU_REWRITE(dfcmgt, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(dftsv) {
    *result = format_nnn("dftsv", i->RT, i->RA, i->I7);
}

#define _dftsv(_ra, _rt, _i7) { \
    auto ra = th->r(_ra); \
    auto& rt = th->r(_rt); \
    auto i7 = _i7; \
    for (int i = 0; i < 2; ++i) { \
        auto fd = ra.fd(i); \
        auto c = std::fpclassify(fd); \
        auto set = ((i7 & 0b1000000) && (c == FP_NAN)) \
                || ((i7 & 0b0100000) && (c == FP_INFINITE && copysign(1.f, fd) > 0.f)) \
                || ((i7 & 0b0010000) && (c == FP_INFINITE && copysign(1.f, fd) < 0.f)) \
                || ((i7 & 0b0001000) && (fd == +0.f)) \
                || ((i7 & 0b0000100) && (fd == -0.f)) \
                || ((i7 & 0b0000010) && (c == FP_SUBNORMAL && copysign(1.f, fd) > 0.f)) \
                || ((i7 & 0b0000001) && (c == FP_SUBNORMAL && copysign(1.f, fd) > 0.f)); \
        rt.set_dw(i, set ? -0ull : 0); \
    } \
}
EMU_REWRITE(dftsv, i->RA.u(), i->RT.u(), i->I7.u())


PRINT(fceq) {
    *result = format_nnn("fceq", i->RT, i->RA, i->RB);
}

#define _fceq(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, ra.fs(i) == rb.fs(i) ? -1 : 0); \
    } \
}
EMU_REWRITE(fceq, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(fcmeq) {
    *result = format_nnn("fcmeq", i->RT, i->RA, i->RB);
}

#define _fcmeq(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, std::abs(ra.fs(i)) == std::abs(rb.fs(i)) ? -1 : 0); \
    } \
}
EMU_REWRITE(fcmeq, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(fcgt) {
    *result = format_nnn("fcgt", i->RT, i->RA, i->RB);
}

#define _fcgt(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, ra.fs(i) > rb.fs(i) ? -1 : 0); \
    } \
}
EMU_REWRITE(fcgt, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(fcmgt) {
    *result = format_nnn("fcmgt", i->RT, i->RA, i->RB);
}

#define _fcmgt(_ra, _rb, _rt) { \
    auto ra = th->r(_ra); \
    auto rb = th->r(_rb); \
    auto& rt = th->r(_rt); \
    for (int i = 0; i < 4; ++i) { \
        rt.set_w(i, std::abs(ra.fs(i)) > std::abs(rb.fs(i)) ? -1 : 0); \
    } \
}
EMU_REWRITE(fcmgt, i->RA.u(), i->RB.u(), i->RT.u())


PRINT(fscrwr) {
    *result = format_n("fscrwr", i->RA);
}

#define _fscrwr(_ra) { \
    th->fpscr() = th->r(_ra); \
}
EMU_REWRITE(fscrwr, i->RA.u())


PRINT(fscrrd) {
    *result = format_n("fscrrd", i->RT);
}

#define _fscrrd(_rt) { \
    th->r(_rt) = th->fpscr(); \
}
EMU_REWRITE(fscrrd, i->RT.u())


PRINT(stop) {
    *result = format_n("stop", i->StopAndSignalType);
}

#define _stop(_sast, _cia) { \
    SPU_RESTORE_NIP(_cia); \
    throw StopSignalException(_sast); \
}
EMU_REWRITE(stop, i->StopAndSignalType.u(), cia)


PRINT(stopd) {
    *result = "stopd";
}

#define _stopd(_cia) { \
    SPU_RESTORE_NIP(_cia); \
    throw BreakpointException(); \
}
EMU_REWRITE(stopd, cia)


PRINT(lnop) {
    *result = "lnop";
}

#define _lnop(_) { \
}
EMU_REWRITE(lnop, 0)


PRINT(nop) {
    *result = "nop";
}

#define _nop(_) { \
}
EMU_REWRITE(nop, 0)


PRINT(sync) {
    *result = "sync";
}

#define _sync(_) { \
    __sync_synchronize(); \
}
EMU_REWRITE(sync, 0)


PRINT(dsync) {
    *result = "dsync";
}

#define _dsync(_) { \
    __sync_synchronize(); \
}
EMU_REWRITE(dsync, 0)


PRINT(rdch) {
    *result = format_nn("rdch", i->CA, i->RT);
}

#define _rdch(_ca, _rt) { \
    auto t = _mm_set_epi32(th->channels()->read(_ca), 0, 0, 0); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(rdch, i->CA.u(), i->RT.u())


PRINT(rchcnt) {
    *result = format_nn("rchcnt", i->CA, i->RT);
}

#define _rchcnt(_ca, _rt) { \
    auto t = _mm_set_epi32(th->channels()->readCount(_ca), 0, 0, 0); \
    th->r(_rt).set_xmm(t); \
}
EMU_REWRITE(rchcnt, i->CA.u(), i->RT.u())


PRINT(wrch) {
    *result = format_nn("wrch", i->CA, i->RT);
}

#define _wrch(_ca, _rt, _cia) { \
    SPU_RESTORE_NIP(_cia); \
    th->channels()->write(_ca, th->r(_rt).w<0>()); \
}
EMU_REWRITE(wrch, i->CA.u(), i->RT.u(), cia)

#if !defined(EMU_REWRITER)
template <DasmMode M, typename S>
void SPUDasm(void* instr, uint32_t cia, S* state) {
    uint32_t x = big_to_native<uint32_t>(*reinterpret_cast<uint32_t*>(instr));
    auto i = reinterpret_cast<SPUForm*>(&x);
    switch (i->OP11.u()) {
        case 0b00111000100: INVOKE(lqx);
        case 0b00101000100: INVOKE(stqx);
        case 0b00111110100: INVOKE(cbd);
        case 0b00111010100: INVOKE(cbx);
        case 0b00111110101: INVOKE(chd);
        case 0b00111010101: INVOKE(chx);
        case 0b00111110110: INVOKE(cwd);
        case 0b00111010110: INVOKE(cwx);
        case 0b00111110111: INVOKE(cdd);
        case 0b00111010111: INVOKE(cdx);
        case 0b00011001000: INVOKE(ah);
        case 0b00011000000: INVOKE(a);
        case 0b00001000000: INVOKE(sf);
        case 0b01101000000: INVOKE(addx);
        case 0b00011000010: INVOKE(cg);
        case 0b01101000010: INVOKE(cgx);
        case 0b00001000010: INVOKE(bg);
        case 0b01101000011: INVOKE(bgx);
        case 0b01111000100: INVOKE(mpy);
        case 0b01111001100: INVOKE(mpyu);
        case 0b01111000101: INVOKE(mpyh);
        case 0b01111000111: INVOKE(mpys);
        case 0b01111000110: INVOKE(mpyhh);
        case 0b01101000110: INVOKE(mpyhha);
        case 0b01111001110: INVOKE(mpyhhu);
        case 0b01101001110: INVOKE(mpyhhau);
        case 0b01010100101: INVOKE(clz);
        case 0b01010110100: INVOKE(cntb);
        case 0b00110110110: INVOKE(fsmb);
        case 0b00110110101: INVOKE(fsmh);
        case 0b00110110100: INVOKE(fsm);
        case 0b00110110010: INVOKE(gbb);
        case 0b00110110001: INVOKE(gbh);
        case 0b00110110000: INVOKE(gb);
        case 0b00011010011: INVOKE(avgb);
        case 0b00001010011: INVOKE(absdb);
        case 0b01001010011: INVOKE(sumb);
        case 0b01010110110: INVOKE(xsbh);
        case 0b01010101110: INVOKE(xshw);
        case 0b01010100110: INVOKE(xswd);
        case 0b00011000001: INVOKE(and_);
        case 0b01011000001: INVOKE(andc);
        case 0b00001000001: INVOKE(or_);
        case 0b01011001001: INVOKE(orc);
        case 0b00111110000: INVOKE(orx);
        case 0b01001000001: INVOKE(xor_);
        case 0b00011001001: INVOKE(nand);
        case 0b00001001001: INVOKE(nor);
        case 0b01001001001: INVOKE(eqv);
        case 0b00001011111: INVOKE(shlh);
        case 0b00001111111: INVOKE(shlhi);
        case 0b00001011011: INVOKE(shl);
        case 0b00001111011: INVOKE(shli);
        case 0b00111011011: INVOKE(shlqbi);
        case 0b00111111011: INVOKE(shlqbii);
        case 0b00111011111: INVOKE(shlqby);
        case 0b00111111111: INVOKE(shlqbyi);
        case 0b00111001111: INVOKE(shlqbybi);
        case 0b00001011100: INVOKE(roth);
        case 0b00001111100: INVOKE(rothi);
        case 0b00001011000: INVOKE(rot);
        case 0b00001111000: INVOKE(roti);
        case 0b00111011100: INVOKE(rotqby);
        case 0b00111111100: INVOKE(rotqbyi);
        case 0b00111001100: INVOKE(rotqbybi);
        case 0b00111011000: INVOKE(rotqbi);
        case 0b00111111000: INVOKE(rotqbii);
        case 0b00001011101: INVOKE(rothm);
        case 0b00001111101: INVOKE(rothmi);
        case 0b00001011001: INVOKE(rotm);
        case 0b00001111001: INVOKE(rotmi);
        case 0b00111011101: INVOKE(rotqmby);
        case 0b00111111101: INVOKE(rotqmbyi);
        case 0b00111001101: INVOKE(rotqmbybi);
        case 0b00111011001: INVOKE(rotqmbi);
        case 0b00111111001: INVOKE(rotqmbii);
        case 0b00001011110: INVOKE(rotmah);
        case 0b00001111110: INVOKE(rotmahi);
        case 0b00001011010: INVOKE(rotma);
        case 0b00001111010: INVOKE(rotmai);
        case 0b01111011000: INVOKE(heq);
        case 0b01001011000: INVOKE(hgt);
        case 0b01011011000: INVOKE(hlgt);
        case 0b01111010000: INVOKE(ceqb);
        case 0b01111001000: INVOKE(ceqh);
        case 0b01111000000: INVOKE(ceq);
        case 0b01001010000: INVOKE(cgtb);
        case 0b01001001000: INVOKE(cgth);
        case 0b01001000000: INVOKE(cgt);
        case 0b01011010000: INVOKE(clgtb);
        case 0b01011001000: INVOKE(clgth);
        case 0b01011000000: INVOKE(clgt);
        case 0b00110101000: INVOKE(bi);
        case 0b00110101010: INVOKE(iret);
        case 0b00110101001: INVOKE(bisl);
        case 0b00100101000: INVOKE(biz);
        case 0b00100101001: INVOKE(binz);
        case 0b00100101010: INVOKE(bihz);
        case 0b00100101011: INVOKE(bihnz);
        case 0b00110101100: INVOKE(hbr);
        case 0b01011000100: INVOKE(fa);
        case 0b01011001100: INVOKE(dfa);
        case 0b01011000101: INVOKE(fs);
        case 0b01011001101: INVOKE(dfs);
        case 0b01011000110: INVOKE(fm);
        case 0b01011001110: INVOKE(dfm);
        case 0b01101011100: INVOKE(dfma);
        case 0b01101011110: INVOKE(dfnms);
        case 0b01101011101: INVOKE(dfms);
        case 0b01101011111: INVOKE(dfnma);
        case 0b00110111000: INVOKE(frest);
        case 0b00110111001: INVOKE(frsqest);
        case 0b01111010100: INVOKE(fi);
        case 0b01110111001: INVOKE(frds);
        case 0b01110111000: INVOKE(fesd);
        case 0b01111000011: INVOKE(dfceq);
        case 0b01111001011: INVOKE(dfcmeq);
        case 0b01011000011: INVOKE(dfcgt);
        case 0b01011001011: INVOKE(dfcmgt);
        case 0b01110111111: INVOKE(dftsv);
        case 0b01111000010: INVOKE(fceq);
        case 0b01111001010: INVOKE(fcmeq);
        case 0b01011000010: INVOKE(fcgt);
        case 0b01011001010: INVOKE(fcmgt);
        case 0b01110111010: INVOKE(fscrwr);
        case 0b01110011000: INVOKE(fscrrd);
        case 0b00000000000: INVOKE(stop);
        case 0b00101000000: INVOKE(stopd);
        case 0b00000000001: INVOKE(lnop);
        case 0b01000000001: INVOKE(nop);
        case 0b00000000010: INVOKE(sync);
        case 0b00000000011: INVOKE(dsync);
        case 0b00000001101: INVOKE(rdch);
        case 0b00000001111: INVOKE(rchcnt);
        case 0b00100001101: INVOKE(wrch);
        case 0b00001001000: INVOKE(sfh);
        case 0b01101000001: INVOKE(sfx);
    }
    switch (i->OP10.u()) {
        case 0b0111011010: INVOKE(csflt);
        case 0b0111011000: INVOKE(cflts);
        case 0b0111011011: INVOKE(cuflt);
        case 0b0111011001: INVOKE(cfltu);
    }
    switch (i->OP9.u()) {
        case 0b001100001: INVOKE(lqa);
        case 0b001100111: INVOKE(lqr);
        case 0b001000001: INVOKE(stqa);
        case 0b001000111: INVOKE(stqr);
        case 0b010000011: INVOKE(ilh);
        case 0b010000010: INVOKE(ilhu);
        case 0b010000001: INVOKE(il);
        case 0b011000001: INVOKE(iohl);
        case 0b001100101: INVOKE(fsmbi);
        case 0b001100100: INVOKE(br);
        case 0b001100000: INVOKE(bra);
        case 0b001100110: INVOKE(brsl);
        case 0b001100010: INVOKE(brasl);
        case 0b001000010: INVOKE(brnz);
        case 0b001000000: INVOKE(brz);
        case 0b001000110: INVOKE(brhnz);
        case 0b001000100: INVOKE(brhz);
    }
    switch (i->OP8.u()) {
        case 0b00110100: INVOKE(lqd);
        case 0b00100100: INVOKE(stqd);
        case 0b00011101: INVOKE(ahi);
        case 0b00011100: INVOKE(ai);
        case 0b00001101: INVOKE(sfhi);
        case 0b00001100: INVOKE(sfi);
        case 0b01110100: INVOKE(mpyi);
        case 0b01110101: INVOKE(mpyui);
        case 0b00010110: INVOKE(andbi);
        case 0b00010101: INVOKE(andhi);
        case 0b00010100: INVOKE(andi);
        case 0b00000110: INVOKE(orbi);
        case 0b00000101: INVOKE(orhi);
        case 0b00000100: INVOKE(ori);
        case 0b01000110: INVOKE(xorbi);
        case 0b01000101: INVOKE(xorhi);
        case 0b01000100: INVOKE(xori);
        case 0b01111111: INVOKE(heqi);
        case 0b01001111: INVOKE(hgti);
        case 0b01011111: INVOKE(hlgti);
        case 0b01111110: INVOKE(ceqbi);
        case 0b01111101: INVOKE(ceqhi);
        case 0b01111100: INVOKE(ceqi);
        case 0b01001110: INVOKE(cgtbi);
        case 0b01001101: INVOKE(cgthi);
        case 0b01001100: INVOKE(cgti);
        case 0b01011110: INVOKE(clgtbi);
        case 0b01011101: INVOKE(clgthi);
        case 0b01011100: INVOKE(clgti);
    }
    switch (i->OP7.u()) {
        case 0b0100001: INVOKE(ila);
        case 0b0001000: INVOKE(hbra);
        case 0b0001001: INVOKE(hbrr);
    }
    switch (i->OP6.u()) {
        case SPU_BB_CALL_OPCODE: INVOKE(bbcall);
    }
    switch (i->OP4.u()) {
        case 0b1100: INVOKE(mpya);
        case 0b1000: INVOKE(selb);
        case 0b1011: INVOKE(shufb);
        case 0b1110: INVOKE(fma);
        case 0b1101: INVOKE(fnms);
        case 0b1111: INVOKE(fms);
    }
    throw IllegalInstructionException();
}

template void SPUDasm<DasmMode::Print, std::string>(
    void* instr, uint32_t cia, std::string* state);

template void SPUDasm<DasmMode::Emulate, SPUThread>(
    void* instr, uint32_t cia, SPUThread* th);

template void SPUDasm<DasmMode::Rewrite, std::string>(
    void* instr, uint32_t cia, std::string* th);

template void SPUDasm<DasmMode::Name, std::string>(
    void* instr, uint32_t cia, std::string* name);

InstructionInfo analyzeSpu(uint32_t instr, uint32_t cia) {
    auto i = reinterpret_cast<SPUForm*>(&instr);
    InstructionInfo info;
    info.flow = false;
    info.passthrough = false;

    switch (i->OP11.u()) {
        case 0b00110101000: // bi
        case 0b00110101010: // iret
            info.flow = true;
            break;
        case 0b00110101001: // bisl
            info.flow = true;
            info.passthrough = true;
            break;
        case 0b00100101000: // biz
        case 0b00100101001: // binz
        case 0b00100101010: // bihz
        case 0b00100101011: // bihnz
            info.passthrough = true;
            info.flow = true;
            break;
    }

    switch (i->OP9.u()) {
        case 0b001100100:   // br
        case 0b001100110: { // brsl
            int32_t offset = signed_lshift32(i->I16.s(), 2);
            info.target = (cia + offset) & LSLR;
            info.flow = true;
            info.passthrough = i->OP9.u() == 0b001100110;
        } break;
        case 0b001100000:   // bra
        case 0b001100010: { // brasl
            int32_t address = signed_lshift32(i->I16.s(), 2);
            info.target = address & LSLR;
            info.flow = true;
            info.passthrough = i->OP9.u() == 0b001100010;
        } break;
        case 0b001000010:   // brnz
        case 0b001000000:   // brz
        case 0b001000110:   // brhnz
        case 0b001000100: { // brhz
            info.target = br_cia_lsa(i->I16, cia);
            info.passthrough = true;
            info.flow = true;
        } break;
    }

    return info;
}
#endif
