#pragma once

#include <stdint.h>

inline int64_t getSValue(int64_t u) { return u; }
template <typename BF, int = BF::P>
inline int64_t getSValue(BF bf) { return bf.s(); }

inline uint64_t getUValue(uint64_t u) { return u; }
template <typename BF, int = BF::P>
inline uint64_t getUValue(BF bf) { return bf.u(); }

enum class BitFieldType {
    Signed, Unsigned, GPR, CR, None
};

template <int Pos, int Next, BitFieldType T = BitFieldType::None, int Shift = 0>
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
    
    inline int64_t native() {
        return T == BitFieldType::Signed ?
            (*this << Shift) : (u() << Shift);
    }
    
    inline const char* prefix() {
        return T == BitFieldType::CR ? "cr"
             : T == BitFieldType::GPR ? "r"
             : "";
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

inline uint64_t mask(uint8_t x, uint8_t y) {
    if (x > y)
        return ~mask(y + 1, x - 1);
    return (~0ull << x) >> (64 - (y - x));
}