#pragma once

#include <stdint.h>
#include <x86intrin.h>
#include <boost/endian/arithmetic.hpp>

static const __m128i ENDIAN_SWAP_MASK128 =
    _mm_set_epi8(
        0, 1, 2, 3,
        4, 5, 6, 7,
        8, 9, 10, 11,
        12, 13, 14, 15
    );

using uint128_t = __m128i;
using int128_t = __m128i;

using namespace boost::endian;

template<int Len>
struct IntTraits;

template<>
struct IntTraits<1> {
    using BigType = big_uint8_t;
    using Type = uint8_t;
};

template<>
struct IntTraits<2> {
    using BigType = big_uint16_t;
    using Type = uint16_t;
};

template<>
struct IntTraits<4> {
    using BigType = big_uint32_t;
    using Type = uint32_t;
};

template<>
struct IntTraits<8> {
    using BigType = big_uint64_t;
    using Type = uint64_t;
};

template<>
struct IntTraits<16> {
    using BigType = uint128_t;
    using Type = uint128_t;
};

inline uint128_t fast_endian_reverse(uint128_t val) {
    return _mm_shuffle_epi8(val, ENDIAN_SWAP_MASK128);
}

inline uint64_t fast_endian_reverse(uint64_t val) {
    return __builtin_bswap64(val);
}

inline uint32_t fast_endian_reverse(uint32_t val) {
    return __builtin_bswap32(val);
}

inline uint16_t fast_endian_reverse(uint16_t val) {
    return __builtin_bswap16(val);
}

inline uint8_t fast_endian_reverse(uint8_t val) {
    return val;
}

template <int Size>
void fast_endian_reverse(void* dest, const void* src, unsigned count) {
    static_assert(Size == 2 || Size == 4);
    auto mask16 = _mm256_set_epi8(
                30, 31, 28, 29, 26, 27, 24, 25,
                22, 23, 20, 21, 18, 19, 16, 17,
                14, 15, 12, 13, 10, 11, 8, 9,
                6, 7, 4, 5, 2, 3, 0, 1);
    auto mask32 = _mm256_set_epi8(
                28, 29, 30, 31, 24, 25, 26, 27,
                20, 21, 22, 23, 16, 17, 18, 19,
                12, 13, 14, 15, 8, 9, 10, 11,
                4, 5, 6, 7, 0, 1, 2, 3);
    auto mask = Size == 2 ? mask16 : mask32;
    auto lineCount = (count * Size) / 32;
    for (auto i = 0u; i < lineCount; ++i) {
        auto val = _mm256_lddqu_si256((__m256i*)src + i);
        auto res = _mm256_shuffle_epi8(val, mask);
        _mm256_storeu_si256((__m256i*)dest + i, res);
    }
    for (auto i = lineCount * 32; i < count * Size; i += Size) {
        if (Size == 2) {
            *(uint16_t*)((uint8_t*)dest + i) =
                fast_endian_reverse(*(uint16_t*)((uint8_t*)src + i));
        } else {
            *(uint32_t*)((uint8_t*)dest + i) =
                fast_endian_reverse(*(uint32_t*)((uint8_t*)src + i));
        }
    }
}

constexpr uint32_t constexpr_log2(uint32_t n, uint32_t p = 0) {
    return n <= 1 ? p : constexpr_log2(n / 2, p + 1);
}

#define likely(x) __builtin_expect ((x), 1)
#define unlikely(x) __builtin_expect ((x), 0)
