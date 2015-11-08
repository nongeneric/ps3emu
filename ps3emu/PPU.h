#pragma once

#include "Rsx.h"
#include "BitField.h"
#include "constants.h"
#include "utils.h"
#include <stdint.h>
#include <memory>
#include <type_traits>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <bitset>
#include <boost/endian/arithmetic.hpp>
#include <boost/chrono.hpp>

class ProcessFinishedException : public std::exception { };

union VirtualAddress {
    uint32_t val;
    BitField<0, DefaultMainMemoryPageBits> page;
    BitField<DefaultMainMemoryPageBits, 32> offset;
};

struct MemoryPage {
    uint8_t* ptr = nullptr;
    void alloc();
    void dealloc();
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
    uint32_t v = 0;
};
static_assert(sizeof(CR_t) == sizeof(uint32_t), "");

union FPSCR_t {
    struct {
        uint32_t RN : 2;
        uint32_t NI : 1;
        uint32_t XE : 1;
        uint32_t ZE : 1;
        uint32_t UE : 1;
        uint32_t OE : 1;
        uint32_t VE : 1;
        uint32_t VXCVI : 1;
        uint32_t VXSQRT : 1;
        uint32_t VXSOFT : 1;
        uint32_t _ : 1;
        uint32_t FUorNAN : 1;
        uint32_t FEorEq : 1;
        uint32_t FGorGt : 1;
        uint32_t FLofLt : 1;
        uint32_t C : 1;
        uint32_t FI : 1;
        uint32_t FR : 1;
        uint32_t VXVC : 1;
        uint32_t VXIMZ : 1;
        uint32_t VXZDZ : 1;
        uint32_t VXIDI : 1;
        uint32_t VXISI : 1;
        uint32_t VXSNAN : 1;
        uint32_t XX : 1;
        uint32_t ZX : 1;
        uint32_t UX : 1;
        uint32_t OX : 1;
        uint32_t VX : 1;
        uint32_t FEX : 1;
        uint32_t FX : 1;
    } f;
    struct {
        uint32_t _ : 12;
        uint32_t v : 5;
    } fprf;
    struct {
        uint32_t _ : 13;
        uint32_t v : 4;
    } fpcc;
    uint32_t v = 0;
};
static_assert(sizeof(FPSCR_t) == sizeof(uint32_t), "");

union XER_t {
    struct {
        uint64_t Bytes : 7;
        uint64_t __ : 22;
        uint64_t CA : 1;
        uint64_t OV : 1;
        uint64_t SO : 1;
        uint64_t _ : 32;
    } f;
    uint64_t v = 0;
};
static_assert(sizeof(XER_t) == sizeof(uint64_t), "");

class ELFLoader;
class PPU {
    uint64_t _LR = 0;
    uint64_t _CTR = 0;
    uint64_t _NIP = 0;
    std::array<uint64_t, 32> _GPR;
    std::array<double, 32> _FPR;
    std::array<unsigned __int128, 32> _V;
    FPSCR_t _FPSCR;
    CR_t _CR;
    XER_t _XER;
    
    std::unique_ptr<MemoryPage[]> _pages;
    std::bitset<DefaultMainMemoryPageCount> _providedMemoryPages;
    Rsx* _rsx = nullptr;
    ELFLoader* _elfLoader = nullptr;
    boost::chrono::high_resolution_clock::time_point _systemStart;
    
    inline uint8_t get4bitField(uint32_t r, uint8_t n) {
        auto fpos = 4 * n;
        auto fmask = (uint32_t)mask<32>(fpos, fpos + 3);
        return (r & fmask) >> (32 - fpos - 4);
    }
    
    inline uint32_t set4bitField(uint32_t r, uint8_t n, uint8_t value) {
        auto fpos = 4 * n;
        auto fmask = ~(uint32_t)mask<32>(fpos, fpos + 3);
        auto f = value << (32 - fpos - 4);
        return (r & fmask) | f;
    }
    
public:
    PPU();
    void shutdown();
    void writeMemory(ps3_uintptr_t va, const void* buf, uint len, bool allocate = false);
    void readMemory(ps3_uintptr_t va, void* buf, uint len, bool allocate = false);
    void setMemory(ps3_uintptr_t va, uint8_t value, uint len, bool allocate = false);
    ps3_uintptr_t malloc(ps3_uintptr_t size);
    void allocPage(void** ptr, ps3_uintptr_t* va);
    void reset();
    void ncall(uint32_t index);
    void scall();
    uint32_t findNCallEntryIndex(std::string name);
    int allocatedPages();
    bool isAllocated(ps3_uintptr_t va);
    void setRsx(Rsx* rsx);
    void setELFLoader(ELFLoader* elfLoader);
    ELFLoader* getELFLoader();
    void map(ps3_uintptr_t src, ps3_uintptr_t dest, uint32_t size);
    void provideMemory(ps3_uintptr_t src, uint32_t size, void* memory);
    
    uint8_t* getMemoryPointer(ps3_uintptr_t va, uint32_t len);
    
    uint64_t getFrequency();
    uint64_t getTimeBase();
    
    template <int Bytes>
    typename BytesToBEType<Bytes>::type load(ps3_uintptr_t va) {
        typename BytesToBEType<Bytes>::beType res;
        readMemory(va, &res, Bytes);
        return res;
    }
    
    template <int Bytes>
    typename BytesToBEType<Bytes>::stype loads(ps3_uintptr_t va) {
        return load<Bytes>(va);
    }
    
    template <int Bytes, typename V>
    void store(uint64_t va, V value) {
        typename BytesToBEType<Bytes>::beType x = getUValue(value);
        writeMemory(va, &x, Bytes);
    }
    
    void store16(uint64_t va, unsigned __int128 value) {
        uint8_t *bytes = (uint8_t*)&value;
        std::reverse(bytes, bytes + 16);
        writeMemory(va, bytes, 16);
    }
    
    void storef(ps3_uintptr_t va, float value) {
        store<sizeof(float)>(va, union_cast<float, uint32_t>(value));
    }
    
    void stored(ps3_uintptr_t va, double value) {
        store<sizeof(double)>(va, union_cast<double, uint64_t>(value));
    }
    
    float loadf(ps3_uintptr_t va) {
        auto f = (uint32_t)load<sizeof(float)>(va);
        return union_cast<uint32_t, float>(f);
    }
    
    unsigned __int128 load16(uint64_t va) {
        unsigned __int128 i = load<8>(va);
        i <<= 64;
        i |= load<8>(va + 8);
        return i;
    }
    
    double loadd(ps3_uintptr_t va) {
        auto f = (uint64_t)load<sizeof(double)>(va);
        return union_cast<uint64_t, double>(f);
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
    
    template <typename V>
    inline void setV(V i, unsigned __int128 value) {
        _V[getUValue(i)] = value;
    }
    
    template <typename V>
    inline void setV(V i, uint8_t* be) {
        assert(getUValue(i) < _V.size());
        auto v = (uint8_t*)&_V[getUValue(i)];
        std::reverse_copy(be, be + 16, v);
    }
    
    template <typename V>
    inline void getV(V i, uint8_t* be) {
        assert(getUValue(i) < _V.size());
        auto v = (uint8_t*)&_V[getUValue(i)];
        std::reverse_copy(v, v + 16, be);
    }
    
    template <typename V>
    inline unsigned __int128 getV(V i) {
        return _V[getUValue(i)];
    }
    
    template <typename V>
    inline void setFPRd(V i, double value) {
        _FPR[getUValue(i)] = value;
    }
    
    template <typename V>
    inline double getFPRd(V i) {
        return _FPR[getUValue(i)];
    }
    
    template <typename V>
    inline void setFPR(V i, uint64_t value) {
        auto idx = getUValue(i);
        *reinterpret_cast<uint64_t*>(&_FPR[idx]) = value;
    }
    
    template <typename V>
    inline uint64_t getFPR(V i) {
        auto idx = getUValue(i);
        return *reinterpret_cast<uint64_t*>(&_FPR[idx]);
    }
    
    inline void setFPSCR(uint32_t value) {
        _FPSCR.v = value;
    }
    
    inline FPSCR_t getFPSCR() {
        return _FPSCR;
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
    
    inline uint8_t getCRF_sign(uint8_t n) {
        return getCRF(n) >> 1;
    }
    
    inline void setCRF_sign(uint8_t n, uint8_t sign) {
        auto so = getCRF(n) & 1;
        setCRF(n, (sign << 1) | so);
    }
    
    inline uint8_t getCRF(uint8_t n) {
        return get4bitField(getCR(), n);
    }
    
    inline void setCRF(uint8_t n, uint8_t value) {
        setCR(set4bitField(getCR(), n, value));
    }
    
    inline uint8_t getFPSCRF(uint8_t n) {
        return get4bitField(getFPSCR().v, n);
    }
    
    inline void setFPSCRF(uint8_t n, uint8_t value) {
        setFPSCR(set4bitField(getFPSCR().v, n, value));
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
    
    inline Rsx* getRsx() {
        return _rsx;
    }
};
