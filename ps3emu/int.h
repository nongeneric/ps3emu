#pragma once

#include <stdint.h>
#include <boost/endian/arithmetic.hpp>

using uint128_t = __uint128_t;
using int128_t = __int128_t;

namespace boost::endian {
    typedef endian_arithmetic<order::big, __int128_t, 128> big_int128_t;
    typedef endian_arithmetic<order::big, __uint128_t, 128> big_uint128_t;
    inline uint128_t endian_reverse(uint128_t x) BOOST_NOEXCEPT {
        return *(big_uint128_t*)&x;
    }
}

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
    using BigType = big_uint128_t;
    using Type = big_uint128_t;
};
