#pragma once

#include "BitField.h"
#include <stdint.h>
#include <map>
#include <memory>
#include <type_traits>
#include <boost/endian/arithmetic.hpp>

struct MemoryPage {
    MemoryPage();
    std::unique_ptr<uint8_t> ptr;
    static constexpr uint pageSize = 64 * 1024;
};

template <int Bytes>
struct BytesToBEType { };
template <>
struct BytesToBEType<1> { 
    typedef boost::endian::big_uint8_t beType;
    typedef uint8_t type;
    typedef int8_t stype;
};
template <>
struct BytesToBEType<2> { 
    typedef boost::endian::big_uint16_t beType;
    typedef uint16_t type;
    typedef int16_t stype;
};
template <>
struct BytesToBEType<4> { 
    typedef boost::endian::big_uint32_t beType;
    typedef uint32_t type;
    typedef int32_t stype;
};
template <>
struct BytesToBEType<8> { 
    typedef boost::endian::big_uint64_t beType;
    typedef uint64_t type;
    typedef int64_t stype;
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

union XER_t {
    struct {
        uint64_t Bytes : 7;
        uint64_t __ : 22;
        uint64_t CA : 1;
        uint64_t OV : 1;
        uint64_t SO : 1;
        uint64_t _ : 32;
    } f;
    uint64_t v;
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
    void writeMemory(uint64_t va, void* buf, uint len, bool allocate = false);
    void readMemory(uint64_t va, void* buf, uint len, bool allocate = false);
    void setMemory(uint64_t va, uint8_t value, uint len, bool allocate = false);
    void ncall(uint32_t index);
    uint32_t findNCallEntryIndex(std::string name);
    
    template <int Bytes>
    typename BytesToBEType<Bytes>::type load(uint64_t va) {
        typename BytesToBEType<Bytes>::beType res;
        readMemory(va, &res, Bytes);
        return res;
    }
    
    template <int Bytes>
    typename BytesToBEType<Bytes>::stype loads(uint64_t va) {
        return load<Bytes>(va);
    }
    
    template <int Bytes, typename V>
    void store(uint64_t va, V value) {
        typename BytesToBEType<Bytes>::beType x = getUValue(value);
        writeMemory(va, &x, Bytes);
    }
    
    void run();
    
    template <typename V>
    inline void setGPR(V i, uint64_t value) {
        _GPR[getUValue(i)] = value;
    }
    
    template <typename V>
    inline uint64_t getGPR(V i) {
        return _GPR[getUValue(i)];
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
    
    inline uint64_t getXER() {
        return _XER.v;
    }
    
    inline void setXER(uint64_t value) {
        _XER.v = value;
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
    
    inline void setCRF_sign(uint8_t n, uint8_t sign) {
        auto fpos = 4 * n;
        auto fmask = ~(uint32_t)mask<32>(fpos, fpos + 2);
        auto f = ((sign << 1) | getSO()) << (31 - fpos - 3);
        setCR((getCR() & fmask) | f);
    }
    
    inline void setCR0_sign(uint8_t bits) {
        _CR.af.sign = bits;
    }
    
    inline uint8_t getCR0_sign() {
        return _CR.af.sign;
    }
    
    inline void setOV() {
        _XER.f.OV = 1;
        _XER.f.SO = 1;
        _CR.af.SO = 1;
    }
    
    inline void setCA(uint8_t bit) {
        _XER.f.CA = bit;
    }
    
    inline uint8_t getCA() {
        return _XER.f.CA;
    }
    
    inline uint8_t getOV() {
        return _XER.f.OV;
    }
    
    inline uint8_t getSO() {
        return _XER.f.SO;
    }
    
    inline void setNIP(uint64_t value) {
        _NIP = value;
    }
    
    inline uint64_t getNIP() {
        return _NIP;
    }
    
    int allocatedPages();
    bool isAllocated(uint64_t va);
};
