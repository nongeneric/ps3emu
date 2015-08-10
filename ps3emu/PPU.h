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

class PPU {
    uint64_t _LR;
    uint64_t _CTR;
    uint64_t _XER;
    uint64_t _NIP;
    uint64_t _GPR[64];
    double _FPR[64];
    double _FPSCR;
    uint32_t _CR;
    
    std::map<uint64_t, MemoryPage> _pages;
    
public:
    void writeMemory(uint64_t va, void* buf, uint len);
    void readMemory(uint64_t va, void* buf, uint len);
    void setMemory(uint64_t va, uint8_t value, uint len);
    
    template <int Bytes>
    inline typename BytesToBEType<Bytes>::type load(uint64_t va) {
        typename BytesToBEType<Bytes>::beType res;
        readMemory(va, &res, Bytes);
        return res;
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
        return _CR;   
    }
    
    inline void setCR(uint32_t value) {
        _CR = value;
    }
    
    inline void setNIP(uint32_t value) {
        _NIP = value;
    }
};