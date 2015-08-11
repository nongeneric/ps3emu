#pragma once

#include <stdint.h>
#include <map>
#include <memory>
#include <boost/endian/arithmetic.hpp>

struct MemoryPage {
    MemoryPage();
    std::unique_ptr<uint8_t> ptr;
    static constexpr uint pageSize = 4 * 1024;
};

template <int Bytes>
struct BytesToBEType { };
template <>
struct BytesToBEType<1> { 
    typedef boost::endian::big_uint8_t beType;
    typedef uint8_t type;
};
template <>
struct BytesToBEType<2> { 
    typedef boost::endian::big_uint16_t beType;
    typedef uint16_t type;
};
template <>
struct BytesToBEType<4> { 
    typedef boost::endian::big_uint32_t beType;
    typedef uint32_t type;
};
template <>
struct BytesToBEType<8> { 
    typedef boost::endian::big_uint64_t beType;
    typedef uint64_t type;
};

union CR_t {
    struct {
        uint32_t _7 : 4;
        uint32_t _6 : 4;
        uint32_t _5 : 4;
        uint32_t _4 : 4;
        uint32_t _3 : 4;
        uint32_t _2 : 4;
        uint32_t _1 : 4;
        uint32_t _0 : 4;
    } f;
    struct {
        uint32_t _ : 28;
        uint32_t SO : 1;
        uint32_t sign : 3;
    } af;
    uint32_t v;
};
static_assert(sizeof(CR_t) == sizeof(uint32_t), "");

struct XER_t {
    uint64_t Bytes : 7;
    uint64_t __ : 22;
    uint64_t CA : 1;
    uint64_t OV : 1;
    uint64_t SO : 1;
    uint64_t _ : 32;
};
static_assert(sizeof(XER_t) == sizeof(uint64_t), "");

class PPU {
    uint64_t _LR;
    uint64_t _CTR;
    uint64_t _NIP;
    uint64_t _GPR[64];
    double _FPR[64];
    double _FPSCR;
    CR_t _CR;
    XER_t _XER;
    
    std::map<uint64_t, MemoryPage> _pages;
    
public:
    void writeMemory(uint64_t va, void* buf, uint len);
    void readMemory(uint64_t va, void* buf, uint len);
    void setMemory(uint64_t va, uint8_t value, uint len);
    
    template <int Bytes>
    typename BytesToBEType<Bytes>::type load(uint64_t va) {
        typename BytesToBEType<Bytes>::beType res;
        readMemory(va, &res, Bytes);
        return res;
    }
    
    template <int Bytes>
    void store(uint64_t va, uint64_t value) {
        typename BytesToBEType<Bytes>::beType x = value;
        writeMemory(va, &x, Bytes);
    }
    
    void run();
    
    inline void setGPR(int i, uint64_t value) {
        _GPR[i] = value;
    }
    
    inline uint64_t getGPR(int i) {
        return _GPR[i];
    }
    
    inline void setFPSCR(double value) {
        _FPSCR = value;
    }
    
    inline void setLR(uint64_t value) {
        _LR = value;
    }
    
    inline uint64_t getLR() {
        return _LR;
    }
    
    inline uint64_t getCTR() {
        return _CTR;   
    }
    
    inline void setCTR(uint64_t value) {
        _CTR = value;
    }
    
    inline uint32_t getCR() {
        return _CR.v;   
    }
    
    inline void setCR(uint32_t value) {
        _CR.v = value;
    }
    
    inline void setCR0_sign(uint8_t bits) {
        _CR.af.sign = bits;
    }
    
    inline void setOV() {
        _XER.OV = 1;
        _XER.SO = 1;
        _CR.af.SO = 1;
    }
    
    inline void setCA(uint8_t bit) {
        _XER.CA = bit;
    }
    
    inline void setNIP(uint32_t value) {
        _NIP = value;
    }
};