#pragma once

#include "BitField.h"
#include "constants.h"
#include "spinlock.h"
#include "utils.h"
#include <boost/endian/arithmetic.hpp>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <functional>
#include <bitset>
#include <memory>

class PPUThread;
class Process;
class Rsx;

class MemoryAccessException : public virtual std::runtime_error {
public:
    MemoryAccessException() : std::runtime_error("memory access error") { }
};

struct NCallEntry {
    const char* name;
    uint32_t fnid;
    void (*stub)(PPUThread*);
};

union VirtualAddress {
    uint32_t val;
    BitField<0, DefaultMainMemoryPageBits> page;
    BitField<DefaultMainMemoryPageBits, 32> offset;
};

struct MemoryPage {
    std::atomic<uintptr_t> ptr;
    void alloc();
    void dealloc();
    inline MemoryPage() { ptr = 0; }
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

class MainMemory {
    std::function<void(uint32_t, uint32_t)> _memoryWriteHandler;    
    std::unique_ptr<MemoryPage[]> _pages;
    std::bitset<DefaultMainMemoryPageCount> _providedMemoryPages;
    boost::chrono::high_resolution_clock::time_point _systemStart;
    Rsx* _rsx;
    Process* _proc;
    boost::mutex _pageMutex;
    spinlock _storeLock;
    boost::thread::id _reservationThread;
    ps3_uintptr_t _reservationInstr;
    
    template <bool Read>
    void copy(ps3_uintptr_t va, 
        const void* buf, 
        uint len, 
        bool allocate);
    bool storeMemoryWithReservation(void* dest, 
                                    const void* buf, 
                                    uint len,
                                    bool cond);
    void writeSpuAddress(ps3_uintptr_t va, uint64_t val, uint32_t len);
    uint32_t readSpuAddress(ps3_uintptr_t va);
        
public:
    MainMemory();
    void writeMemory(ps3_uintptr_t va, const void* buf, uint len, bool allocate = false);
    void readMemory(ps3_uintptr_t va, void* buf, uint len, bool allocate = false);
    void setMemory(ps3_uintptr_t va, uint8_t value, uint len, bool allocate = false);
    ps3_uintptr_t malloc(ps3_uintptr_t size);
    void free(ps3_uintptr_t addr);
    void reset();
    int allocatedPages();
    bool isAllocated(ps3_uintptr_t va);
    void map(ps3_uintptr_t src, ps3_uintptr_t dest, uint32_t size);
    void provideMemory(ps3_uintptr_t src, uint32_t size, void* memory);
    uint8_t* getMemoryPointer(ps3_uintptr_t va, uint32_t len);
    uint64_t getFrequency();
    uint64_t getTimeBase();
    void memoryBreakHandler(std::function<void(uint32_t, uint32_t)> handler);
    void memoryBreak(uint32_t va, uint32_t size);
    void setRsx(Rsx* rsx);
    void setProc(Process* proc);
    uint32_t loadReserve4(ps3_uintptr_t va);
    bool storeCond4(ps3_uintptr_t va, uint32_t val);
    
    void store16(uint64_t va, unsigned __int128 value) {
        uint8_t *bytes = (uint8_t*)&value;
        std::reverse(bytes, bytes + 16);
        writeMemory(va, bytes, 16);
    }
    
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
    
    template <int N>
    typename BytesToBEType<N>::type loadReserve(ps3_uintptr_t va, ps3_uintptr_t cia) {
        boost::unique_lock<spinlock> lock(_storeLock);
        _reservationThread = boost::this_thread::get_id();
        _reservationInstr = cia;
        return load<N>(va);
    }

    template <int N>
    bool storeCond(ps3_uintptr_t va, typename BytesToBEType<N>::type val) {
        auto dest = getMemoryPointer(va, N);
        boost::endian::native_to_big_inplace(val);
        return storeMemoryWithReservation(dest, &val, N, true);
    }
};

uint32_t calcFnid(const char* name);
void encodeNCall(MainMemory* mm, ps3_uintptr_t va, uint32_t index);
const NCallEntry* findNCallEntry(uint32_t fnid, uint32_t& index);