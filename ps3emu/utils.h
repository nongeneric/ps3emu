#pragma once

#include "constants.h"
#include <fmt/printf.h>
#include <fmt/format.h>
#include <boost/align.hpp>
#include <boost/endian/arithmetic.hpp>
#include <inttypes.h>
#include <string>

template <typename... Args>
std::string sformat(const char* f, Args... args) {
    return fmt::format(f, args...);
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
