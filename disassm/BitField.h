#pragma once

#include <stdint.h>

inline int64_t getSValue(int64_t u) { return u; }
template <typename BF, int = BF::P>
inline int64_t getSValue(BF bf) { return bf.s(); }

inline uint64_t getUValue(uint64_t u) { return u; }
template <typename BF, int = BF::P>
inline uint64_t getUValue(BF bf) { return bf.u(); }

template <int Pos, int Next>
class BitField {
    static_assert(Next <= 32, "out of bounds");
    static_assert(Next - Pos != 0, "zero length");
    static_assert(Next > Pos, "bad range");
    uint32_t _v;
    uint64_t v() {
        return (_v >> (32 - Next)) & ~(~0ull << (Next - Pos));
    }
public:
    static constexpr int P = Pos;
    
    inline uint64_t u() {
        return v();
    }
    
    inline int64_t s() {
        auto r = v();
        if (r & (1 << (Next - Pos - 1))) {
            return r | (~0ull << (Next - Pos));
        }
        return r;
    }
    
    inline int64_t operator<<(unsigned shift) {
        return static_cast<uint64_t>(s()) << shift;
    }
};

template <typename BF, int = BF::P>
inline int64_t operator+(BF bf, int64_t x) {
    return bf.s() + x;
}

template <typename BF, int = BF::P>
inline int64_t operator+(int64_t x, BF bf) {
    return bf.s() + x;
}

template <typename BF, int = BF::P>
inline uint64_t operator+(BF bf, uint64_t x) {
    return bf.u() + x;
}

template <typename BF, int = BF::P>
inline uint64_t operator+(uint64_t x, BF bf) {
    return bf.u() + x;
}