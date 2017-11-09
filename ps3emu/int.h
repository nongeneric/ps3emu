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
    using Type = big_uint8_t;
};

template<>
struct IntTraits<2> {
    using BigType = big_uint16_t;
    using Type = big_uint16_t;
};

template<>
struct IntTraits<4> {
    using BigType = big_uint32_t;
    using Type = big_uint32_t;
};

template<>
struct IntTraits<8> {
    using BigType = big_uint64_t;
    using Type = big_uint64_t;
};

template<>
struct IntTraits<16> {
    using BigType = uint128_t;
    using Type = uint128_t;
};

constexpr uint32_t constexpr_log2(uint32_t n, uint32_t p = 0) {
    return n <= 1 ? p : constexpr_log2(n / 2, p + 1);
}
