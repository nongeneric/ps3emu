#pragma once

#include <stdint.h>
#include <assert.h>

inline int64_t getSValue(int64_t u) { return u; }
template <typename BF, int = BF::P>
inline int64_t getSValue(BF bf) { return bf.s(); }

inline uint64_t getUValue(uint64_t u) { return u; }
template <typename BF, int = BF::P>
inline uint64_t getUValue(BF bf) { return bf.u(); }

template <int Width, typename T>
inline T mask(int x, int y) {
    static_assert(Width <= sizeof(T) * 8, "");
    assert(x < Width && y < Width);
    assert(x >= 0 && y >= 0);
    #pragma GCC diagnostic ignored "-Wstrict-overflow"
    if (x > y)
    #pragma GCC diagnostic pop
        return mask<Width, T>(0, y) | mask<Width, T>(x, Width - 1);
    T a = 0;
    a = ~a;
    a >>= x;
    T b = 0;
    b = ~b;
    b <<= sizeof(T) * 8 - 1 - y;
    b &= a;
    b >>= sizeof(T) * 8 - Width;
    return b;
}

template <int Width>
inline uint64_t mask(int x, int y) {
    return mask<Width, uint64_t>(x, y);
}

enum class BitFieldType {
    Signed, Unsigned, GPR, Channel, CR, FPR, Vector, None
};

template <int Pos, int Next, BitFieldType T = BitFieldType::None, int Shift = 0>
class BitField {
    static_assert(Next <= 32, "out of bounds");
    static_assert(Next - Pos != 0, "zero length");
    static_assert(Next > Pos, "bad range");
    uint32_t _v;
    uint32_t v() const {
        return (_v >> (32 - Next)) & ~(~0ull << (Next - Pos));
    }
public:
    static constexpr int P = Pos;
    static constexpr int W = Next - Pos;
    
    inline uint32_t u() const {
        return v();
    }
    
    inline int32_t s() const {
        auto r = v();
        if (r & (1 << (Next - Pos - 1))) {
            return r | (~0ull << (Next - Pos));
        }
        return r;
    }
    
    inline int32_t native() const {
        return T == BitFieldType::Signed ?
            (*this << Shift) : (u() << Shift);
    }
    
    inline const char* prefix() const {
        return T == BitFieldType::CR ? "cr"
             : T == BitFieldType::GPR ? "r"
             : T == BitFieldType::Channel ? "ch"
             : T == BitFieldType::FPR ? "f"
             : T == BitFieldType::Vector ? "v"
             : "";
    }
    
    inline int32_t operator<<(unsigned shift) const {
        return static_cast<uint32_t>(s()) << shift;
    }
    
    inline void set(uint32_t u) {
        uint32_t field = (u & ((1 << W) - 1)) << (32 - W - P);
        _v = (_v & ~mask<32>(P, P + W - 1)) | field;
    }
};

template <typename N>
inline uint8_t bit_test(uint64_t number, int width, N pos) {
    auto n = getUValue(pos);
    auto sh = width - n - 1;
    return (number & (1u << sh)) >> sh;
}

template <typename T, int Pos, int Next>
T bit_test(T number, BitField<Pos, Next> bf) {
    return bit_test(number, sizeof(T) * 8, bf);
}

template <typename N>
uint64_t bit_set(uint64_t number, int width, N pos, int val) {
    auto n = getUValue(pos);
    auto sh = width - n - 1;
    auto mask = ~(1u << sh);
    return (number & mask) | (val << sh);
}

template <typename T>
T bit_set(T number, int pos, int val) {
    return bit_set(number, sizeof(T) * 8, pos, val);
}

// left shift on a negative value is undefined
inline int32_t signed_lshift32(int32_t v, int n) {
    return (uint32_t)v << n;
}

inline int32_t signed_rshift32(int32_t v, int n) {
    static_assert(-1 >> 1 == -1,
        "implementation defined right shift on a negative "
        "value should replicate the left most bit");
    return v >> n;
}

inline unsigned count_ones32(uint32_t x) {
    x -= ((x >> 1) & 0x55555555);
    x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
    x = (((x >> 4) + x) & 0x0f0f0f0f);
    x += (x >> 8);
    x += (x >> 16);
    return x & 0x0000003f;
}

template <int Size, typename T>
inline T rol(T n, unsigned sh) {
    assert(sh < Size);
    if (sh == 0)
        return n;
    auto right = n & mask<Size, T>(sh, Size - 1);
    auto left = n & mask<Size, T>(0, sh);
    left >>= Size - sh;
    right <<= sh;
    return left | right;
}

template <int Size>
inline uint64_t rol(uint64_t n, unsigned sh) {
    return rol<Size, uint64_t>(n, sh);
}

template <int Size>
inline uint64_t ror(uint64_t n, unsigned sh) {
    return rol<Size, uint64_t>(Size - n, sh);
}