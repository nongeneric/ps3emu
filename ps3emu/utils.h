#pragma once

#include <boost/endian/arithmetic.hpp>
#include "constants.h"
#include <string>
#include <stdio.h>
#include <inttypes.h>

template <typename T>
struct AdaptType {
    T const& adapt(T const& t) { return t; }
};

template <>
struct AdaptType<std::string> { 
    const char* adapt(std::string const& str) { return str.c_str(); }
};

template <>
struct AdaptType<boost::endian::big_uint64_t> { 
    uint64_t adapt(boost::endian::big_uint64_t str) { return (uint64_t)str; }
};

template <>
struct AdaptType<boost::endian::big_uint32_t> { 
    uint32_t adapt(boost::endian::big_uint32_t str) { return (uint32_t)str; }
};

template <>
struct AdaptType<boost::endian::big_uint16_t> { 
    uint16_t adapt(boost::endian::big_uint16_t str) { return (uint16_t)str; }
};

template <>
struct AdaptType<boost::endian::big_int64_t> { 
    int64_t adapt(boost::endian::big_int64_t str) { return (int64_t)str; }
};

template <>
struct AdaptType<boost::endian::big_int32_t> { 
    int32_t adapt(boost::endian::big_int32_t str) { return (int32_t)str; }
};

template <>
struct AdaptType<boost::endian::big_int16_t> { 
    int16_t adapt(boost::endian::big_int16_t str) { return (int16_t)str; }
};

template <typename... Args>
std::string& ssnprintf(const char* f, Args... args) {
    thread_local std::string buf;
    auto len = snprintf(0, 0, f, AdaptType<decltype(args)>().adapt(args)...);
    buf.resize(len);
    len = snprintf(&buf[0], buf.size() + 1, f, AdaptType<decltype(args)>().adapt(args)...);
    return buf;
}

template <typename S, typename D>
union union_cast {
    static_assert(sizeof(S) == sizeof(D), "invalid cast");
    union_cast(S s) : _s(s) { }
    operator D() const {
        return _d;
    }
private:
    S _s;
    D _d;
};

inline uint128_t make128(uint64_t low, uint64_t high) {
    uint128_t i = low;
    i <<= 64;
    return i | high;
}

inline uint128_t make128(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3) {
    uint128_t i = w0;
    i <<= 32;
    i |= w1;
    i <<= 32;
    i |= w2;
    i <<= 32;
    i |= w3;
    return i;
}

inline void split128(uint128_t i, uint32_t* us) {
    us[3] = i & 0xffffffff;
    i >>= 32;
    us[2] = i & 0xffffffff;
    i >>= 32;
    us[1] = i & 0xffffffff;
    i >>= 32;
    us[0] = i & 0xffffffff;
}

inline void split128(uint128_t i, float* fs) {
    split128(i, (uint32_t*)fs);
}

void ums_sleep(uint64_t microseconds);
std::string print_hex(const void* buf, int len, bool cArray = false);

template <typename T, typename Iter, typename IsEmptyPred>
Iter findGap(Iter begin, Iter end, unsigned width, IsEmptyPred isEmpty) {
    auto current = begin;
    auto count = 0u;
    while (begin != end) {
        if (isEmpty(*current)) {
            count++;
        } else {
            current = begin;
            count = 0;
        }
        if (count == width)
            break;
        ++begin;
    }
    assert(count == width);
    return current;
}

template <typename T>
bool intersects(T a, T alen, T b, T blen) {
    return !(a + alen < b || b + blen < a);
}

template <typename T>
T align(T n, unsigned alignment) {
    assert(alignment > 0 && (alignment & (alignment - 1)) == 0);
    return (n + alignment - 1) & (~(alignment - 1));
}
