#pragma once

#include "BitField.h"
#include "constants.h"
#include "utils.h"
#include <boost/endian/arithmetic.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <functional>
#include <bitset>
#include <memory>
#include <atomic>
#include <vector>
#include <map>

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
    std::function<void(PPUThread*)> stub;
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

struct Reservation {
    uint32_t va;
    uint32_t size;
    boost::thread::id thread;
    std::function<void(boost::thread::id)> notify;
};

class SpinLock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) ;
    }
    
    void unlock() {
        flag.clear(std::memory_order_release);
    }
};

struct ProtectionRange {
    uint32_t start;
    uint32_t len;
    bool readonly;
    std::string comment;
};

class MainMemory {
    std::function<void(uint32_t, uint32_t)> _memoryWriteHandler;    
    std::unique_ptr<MemoryPage[]> _pages;
    std::bitset<DefaultMainMemoryPageCount> _providedMemoryPages;
    Process* _proc;
    boost::mutex _pageMutex;
    SpinLock _storeLock;
    std::vector<Reservation> _reservations;
    
#ifdef MEMORY_PROTECTION
    struct ProtectionInfo {
        std::bitset<1u << 10> subrange;
    };
    
    std::map<uint32_t, ProtectionInfo> readInfos;
    std::map<uint32_t, ProtectionInfo> writeInfos;
    std::vector<ProtectionRange> protectionRanges;
    std::bitset<0xffffffffu / (1u << 10)> readMap;
    std::bitset<0xffffffffu / (1u << 10)> writeMap;
#endif
    
    template <bool Read>
    void copy(ps3_uintptr_t va, 
        const void* buf, 
        uint len, 
        bool allocate,
        bool locked);
    bool storeMemoryWithReservation(void* dest,
                                    const void* buf, 
                                    uint len,
                                    uint32_t va,
                                    bool cond);
    void writeRawSpuVa(ps3_uintptr_t va, const void* val, uint32_t len);
    void writeSpuThreadVa(ps3_uintptr_t va, const void* val, uint32_t len);
    uint32_t readRawSpuVa(ps3_uintptr_t va);
    void destroyReservationWithoutLocking(uint32_t va, uint32_t size);
    void destroyReservationOfCurrentThread();
    bool isReservedByCurrentThread(uint32_t va, uint32_t size);
    bool writeSpecialMemory(ps3_uintptr_t va, const void* buf, uint len);
    bool readSpecialMemory(ps3_uintptr_t va, void* buf, uint len);
    void dealloc();

public:
    MainMemory();
    ~MainMemory();
    void writeMemory(ps3_uintptr_t va, const void* buf, uint len, bool allocate = false);
    void readMemory(ps3_uintptr_t va,
                    void* buf,
                    uint len,
                    bool allocate = false,
                    bool locked = true,
                    bool validate = true);
    void setMemory(ps3_uintptr_t va, uint8_t value, uint len, bool allocate = false);
    void reset();
    int allocatedPages();
    bool isAllocated(ps3_uintptr_t va);
    void provideMemory(ps3_uintptr_t src, uint32_t size, void* memory);
    uint8_t* getMemoryPointer(ps3_uintptr_t va, uint32_t len);
    void memoryBreakHandler(std::function<void(uint32_t, uint32_t)> handler);
    void memoryBreak(uint32_t va, uint32_t size);
    
    uint8_t load8(ps3_uintptr_t va);
    uint16_t load16(ps3_uintptr_t va);
    uint32_t load32(ps3_uintptr_t va);
    uint64_t load64(ps3_uintptr_t va);
    
    void store8(ps3_uintptr_t va, uint8_t val);
    void store16(ps3_uintptr_t va, uint16_t val);
    void store32(ps3_uintptr_t va, uint32_t val);
    void store64(ps3_uintptr_t va, uint64_t val);
    
    inline void storef(ps3_uintptr_t va, float value) {
        store32(va, union_cast<float, uint32_t>(value));
    }
    
    inline void stored(ps3_uintptr_t va, double value) {
        store64(va, union_cast<double, uint64_t>(value));
    }
    
    inline float loadf(ps3_uintptr_t va) {
        auto f = load32(va);
        return union_cast<uint32_t, float>(f);
    }
    
    inline double loadd(ps3_uintptr_t va) {
         auto f = load64(va);
         return union_cast<uint64_t, double>(f);
    }
    
    inline unsigned __int128 load128(uint64_t va) {
        unsigned __int128 i = load64(va);
        i <<= 64;
        i |= load64(va + 8);
        return i;
    }
    
    inline void store128(uint64_t va, unsigned __int128 value) {
        uint8_t *bytes = (uint8_t*)&value;
        std::reverse(bytes, bytes + 16);
        writeMemory(va, bytes, 16);
    }

    inline void loadReserve(ps3_uintptr_t va,
                     void* buf,
                     uint len,
                     std::function<void(boost::thread::id)> notify = {}) {
        boost::unique_lock<SpinLock> lock(_storeLock);
        destroyReservationOfCurrentThread();
        _reservations.push_back({va, len, boost::this_thread::get_id(), notify});
        readMemory(va, buf, len, false, false);
    }

    inline bool writeCond(ps3_uintptr_t va, const void* buf, uint len) {
        auto dest = getMemoryPointer(va, len);
        return storeMemoryWithReservation(dest, buf, len, va, true);
    }
    
#ifdef MEMORY_PROTECTION
    void reportViolation(uint32_t ea, uint32_t len, bool write);
    void mark(uint32_t ea, uint32_t len, bool readonly, std::string comment);
    void unmark(uint32_t ea, uint32_t len);
    void validate(uint32_t ea, uint32_t len, bool write);
    ProtectionRange addressRange(uint32_t ea);
#else
    inline void mark(uint32_t ea, uint32_t len, bool readonly, std::string comment) { }
    inline void unmark(uint32_t ea, uint32_t len) { }
    inline void validate(uint32_t ea, uint32_t len, bool write) { }
    inline ProtectionRange addressRange(uint32_t ea) { return {}; }
#endif
};

uint32_t calcFnid(const char* name);
uint32_t calcEid(const char* name);
void encodeNCall(MainMemory* mm, ps3_uintptr_t va, uint32_t index);
const NCallEntry* findNCallEntry(uint32_t fnid, uint32_t& index, bool assertFound = false);
uint32_t addNCallEntry(NCallEntry entry);
void readString(MainMemory* mm, uint32_t va, std::string& str);
