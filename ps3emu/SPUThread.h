#pragma once

#include "constants.h"
#include "BitField.h"
#include <assert.h>
#include <algorithm>
#include <stdexcept>

static constexpr uint32_t LSLR = 0x3ffff;

class StopSignalException : public virtual std::runtime_error {
public:
    StopSignalException() : std::runtime_error("stop signal") { }
};


class R128 {
    uint8_t _bs[16];
public:
    template <int N>
    uint8_t& b() {
        static_assert(0 <= N && N < 16, "");
        return _bs[15 - N];
    }
    
    uint8_t& b(int n) {
        assert(0 <= n && n < 16);
        return _bs[15 - n];
    }
    
    template <int N>
    int16_t& hw() {
        static_assert(0 <= N && N < 8, "");
        return ((int16_t*)_bs)[7 - N];
    }
    
    int16_t& hw(int n) {
        assert(0 <= n && n < 8);
        return ((int16_t*)_bs)[7 - n];
    }
    
    template <int N>
    int32_t& w() {
        static_assert(0 <= N && N < 4, "");
        return ((int32_t*)_bs)[3 - N];
    }
    
    int32_t& w(int n) {
        assert(0 <= n && n < 4);
        return ((int32_t*)_bs)[3 - n];
    }
    
    template <int N>
    int64_t& dw() {
        static_assert(0 <= N && N < 2, "");
        return ((int64_t*)_bs)[1 - N];
    }
    
    int64_t& dw(int n) {
        assert(0 <= n && n < 2);
        return ((int64_t*)_bs)[1 - n];
    }
    
    template <int N>
    float& fs() {
        static_assert(0 <= N && N < 4, "");
        return ((float*)_bs)[3 - N];
    }
    
    inline float& fs(int n) {
        assert(0 <= n && n < 4);
        return ((float*)_bs)[3 - n];
    }
    
    template <int N>
    double& fd() {
        static_assert(0 <= N && N < 2, "");
        return ((double*)_bs)[1 - N];
    }
    
    double& fd(int n) {
        assert(0 <= n && n < 2);
        return ((double*)_bs)[1 - n];
    }
    
    inline void load(const uint8_t* ptr) {
        std::reverse_copy(ptr, ptr + 16, _bs);
    }
    
    inline void store(uint8_t* ptr) {
        std::reverse_copy(_bs, _bs + 16, ptr);
    }
};

class SPUThread {
    uint32_t _nip;
    R128 _rs[128];
    uint8_t _ls[256 * 1024];
    uint32_t _srr0;
public:
    template <typename V>
    inline R128& r(V i) {
        return _rs[getUValue(i)];
    }
    
    inline uint8_t* ptr(uint32_t lsa) {
        assert(lsa < sizeof(_ls));
        return &_ls[lsa];
    }
    
    inline void setNip(uint32_t nip) {
        assert(nip < LSLR);
        _nip = nip;
    }
    
    inline uint32_t getSrr0() {
        return _srr0;
    }
    
    inline void setSrr0(uint32_t val) {
        assert(val < LSLR);
        _srr0 = val;
    }
};