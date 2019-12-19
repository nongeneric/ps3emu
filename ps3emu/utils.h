#pragma once

#include "constants.h"
#include <boost/align.hpp>
#include <boost/endian/arithmetic.hpp>
#include <inttypes.h>
#include <stdio.h>
#include <string>

namespace {
    thread_local std::string ssnprintf_buf;
}

template <typename T>
struct AdaptType {
    T const& adapt(T const& t) { return t; }
};

template <>
struct AdaptType<std::string> {
    const char* adapt(std::string const& str) {
        return str.c_str();
    }
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
std::string ssnprintf(const char* f, Args... args) {
    if (!f) {
        f = "";
    }
    auto len = snprintf(0, 0, f, AdaptType<decltype(args)>().adapt(args)...);
    ssnprintf_buf.resize(len);
    len = snprintf(&ssnprintf_buf[0],
                   len + 1,
                   f,
                   AdaptType<decltype(args)>().adapt(args)...);
    return ssnprintf_buf;
}

template <class D, class S>
D bit_cast(const S& from) {
    static_assert(sizeof(D) == sizeof(S));
    D to;
    memcpy(&to, &from, sizeof(D));
    return to;
}

inline slow_uint128_t make128(uint64_t low, uint64_t high) {
    slow_uint128_t i = low;
    i <<= 64;
    return i | high;
}

inline slow_uint128_t make128(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3) {
    slow_uint128_t i = w0;
    i <<= 32;
    i |= w1;
    i <<= 32;
    i |= w2;
    i <<= 32;
    i |= w3;
    return i;
}

void ums_sleep(uint64_t microseconds);
std::string print_hex(const void* buf, int len, bool cArray = false);

template <typename T>
bool intersects(T a, T alen, T b, T blen) {
    return !(a + alen <= b || b + blen <= a);
}

template <typename T>
bool subset(T sub, T sublen, T a, T alen) {
    return sub >= a && sub + sublen <= a + alen;
}

template <typename Container, typename Pred>
void erase_if(Container& container, Pred pred) {
    auto it = std::find_if(begin(container), end(container), pred);
    if (it != end(container)) {
        container.erase(it);
    }
}

template <typename Iter, typename IsEmptyPred>
Iter findGap(Iter begin, Iter end, unsigned width, unsigned alignment, IsEmptyPred isEmpty) {
    auto lower = begin;
    auto current = begin;
    auto count = 0u;
    while (lower <= end) {
        if (count == 0) {
            auto index = std::distance(begin, current);
            index = boost::alignment::align_up(index, alignment);
            current = begin + index;
        }
        if (isEmpty(current)) {
            count++;
        } else {
            current = lower;
            count = 0;
        }
        if (count == width)
            break;
        ++lower;
    }
    if (count == width)
        return current;
    return end;
}
