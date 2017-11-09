#pragma once

#include "ps3emu/int.h"

namespace {

alignas(16) static const __m128i BYTE_SHIFT_LEFT_SHUFFLE_CONTROL[32] {
    _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0),
    _mm_set_epi8(14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1),
    _mm_set_epi8(13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -1),
    _mm_set_epi8(12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -1, -1),
    _mm_set_epi8(11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -1, -1, -1),
    _mm_set_epi8(10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -1, -1, -1, -1),
    _mm_set_epi8(9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(7, 6, 5, 4, 3, 2, 1, 0, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(6, 5, 4, 3, 2, 1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(5, 4, 3, 2, 1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(4, 3, 2, 1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(3, 2, 1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(2, 1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
};
    
}

struct alignas(16) R128 {
    __m128i _xmm;
public:
    R128() = default;
    inline R128(R128 const& r) {
        _xmm = r._xmm;
    }
    
    inline __m128i xmm() const {
        return _xmm;
    }
    
    inline void set_xmm(__m128i xmm) {
        _xmm = xmm;
    }
    
    inline __m128 xmm_f() const {
        return _mm_castsi128_ps(_xmm);
    }
    
    inline void set_xmm_f(__m128 xmm) {
        _xmm = _mm_castps_si128(xmm);
    }
    
    inline __m128d xmm_d() const {
        return _mm_castsi128_pd(_xmm);
    }
    
    inline void set_xmm_d(__m128d xmm) {
        _xmm = _mm_castpd_si128(xmm);
    }
    
    inline uint8_t b(int n) const {
        uint8_t vec[16];
        memcpy(vec, &_xmm, 16);
        return vec[15 - n];
    }
    
    inline void set_b(int n, uint8_t val) {
        uint8_t vec[16];
        memcpy(vec, &_xmm, 16);
        vec[15 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    inline int16_t hw(int n) const {
        int16_t vec[8];
        memcpy(vec, &_xmm, 16);
        return vec[7 - n];
    }
    
    inline void set_hw(int n, uint16_t val) {
        int16_t vec[8];
        memcpy(vec, &_xmm, 16);
        vec[7 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    template <int N>
    int32_t w() {
        return (int32_t)_mm_extract_epi32(_xmm, 3 - N);
    }
    
    inline int32_t w(int n) const {
        int32_t vec[4];
        memcpy(vec, &_xmm, 16);
        return vec[3 - n];
    }
    
    inline void set_w(int n, uint32_t val) {
        int32_t vec[4];
        memcpy(vec, &_xmm, 16);
        vec[3 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    template <int N>
    int64_t dw() const {
        return (int64_t)_mm_extract_epi64(_xmm, 1 - N);
    }
    
    inline int64_t dw(int n) const {
        int64_t vec[2];
        memcpy(vec, &_xmm, 16);
        return vec[1 - n];
    }
    
    inline void set_dw(int n, int64_t val) {
        int64_t vec[2];
        memcpy(vec, &_xmm, 16);
        vec[1 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    inline float fs(int n) const {
        float vec[4];
        memcpy(vec, &_xmm, 16);
        return vec[3 - n];
    }
    
    inline void set_fs(int n, float val) {
        float vec[4];
        memcpy(vec, &_xmm, 16);
        vec[3 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    inline double fd(int n) const {
        double vec[2];
        memcpy(vec, &_xmm, 16);
        return vec[1 - n];
    }
    
    inline void set_fd(int n, double val) {
        double vec[2];
        memcpy(vec, &_xmm, 16);
        vec[1 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    inline int16_t hw_pref() const {
        return hw(1);
    }
};
