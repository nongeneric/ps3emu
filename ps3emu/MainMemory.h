#pragma once

#include "int.h"
#include "BitField.h"
#include "constants.h"
#include "utils.h"
#include "ReservationMap.h"
#include "ModificationMap.h"
#include "state.h"
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
    std::atomic<uintptr_t> ptr = 0;
    void alloc();
    void dealloc();
    inline MemoryPage() { ptr = 0; }
};

struct ProtectionRange {
    uint32_t start;
    uint32_t len;
    bool readonly;
    std::string comment;
};

class MainMemory {
    std::unique_ptr<MemoryPage[]> _pages;
    std::bitset<DefaultMainMemoryPageCount> _providedMemoryPages;
    boost::mutex _pageMutex;
    ReservationMap _rmap;
    ModificationMap<16u << 10u, 0xffffffffu> _mmap;
    
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
    
    void writeRawSpuVa(ps3_uintptr_t va, const void* val, uint32_t len);
    void writeSpuThreadVa(ps3_uintptr_t va, const void* val, uint32_t len);
    uint32_t readRawSpuVa(ps3_uintptr_t va);
    bool writeSpecialMemory(ps3_uintptr_t va, const void* buf, uint len);
    bool readSpecialMemory(ps3_uintptr_t va, void* buf, uint len);
    void dealloc();

public:
    MainMemory();
    ~MainMemory();
    void writeMemory(ps3_uintptr_t va, const void* buf, uint len);
    void readMemory(ps3_uintptr_t va,
                    void* buf,
                    uint len,
                    bool validate = true);
    void setMemory(ps3_uintptr_t va, uint8_t value, uint len);
    void reset();
    int allocatedPages();
    bool isAllocated(ps3_uintptr_t va);
    void provideMemory(ps3_uintptr_t src, uint32_t size, void* memory);
    uint8_t* getMemoryPointer(ps3_uintptr_t va, uint32_t len);
    
    uint8_t load8(ps3_uintptr_t va, bool validate = true);
    uint16_t load16(ps3_uintptr_t va, bool validate = true);
    uint32_t load32(ps3_uintptr_t va, bool validate = true);
    uint64_t load64(ps3_uintptr_t va, bool validate = true);
    uint128_t load128(ps3_uintptr_t va, bool validate = true);
    
    void store8(ps3_uintptr_t va, uint8_t val);
    void store16(ps3_uintptr_t va, uint16_t val);
    void store32(ps3_uintptr_t va, uint32_t val);
    void store64(ps3_uintptr_t va, uint64_t val);
    void store128(ps3_uintptr_t va, uint128_t val);
    
    template <auto Len>
    typename IntTraits<Len>::Type load(ps3_uintptr_t va) {
        if constexpr(Len == 1)
            return load8(va);
        if constexpr(Len == 2)
            return load16(va);
        if constexpr(Len == 4)
            return load32(va);
        if constexpr(Len == 8)
            return load64(va);
        if constexpr(Len == 16)
            return load128(va);
    }
    
    template <auto Len>
    void store(ps3_uintptr_t va, typename IntTraits<Len>::Type val) {
        if constexpr(Len == 1)
            store8(va, val);
        if constexpr(Len == 2)
            store16(va, val);
        if constexpr(Len == 4)
            store32(va, val);
        if constexpr(Len == 8)
            store64(va, val);
        if constexpr(Len == 16)
            store128(va, val);
    }
    
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
    
    inline auto modificationMap() {
        return &_mmap;
    }

    template <auto Len>
    inline void loadReserve(ps3_uintptr_t va,
                            void* buf,
                            lost_notify_t notify = {}) {
        static_assert(Len == 4 || Len == 8 || Len == 128);
        if constexpr(Len == 4)
            assert((va & 0b11) == 0);
        if constexpr(Len == 8)
            assert((va & 0b111) == 0);
        if constexpr(Len == 128)
            assert((va & 0b1111111) == 0);
        
        auto granule = g_state.granule;
        assert(granule);
        auto granuleLine = granule->line;
        if (granuleLine) {
            granuleLine->lock.lock();
            if (granule->line) {
                _rmap.destroySingleReservation(granule);
            }
            granuleLine->lock.unlock();
        }
        
        auto line = _rmap.lock<Len>(va);
        assert(!line.nextLine && "only a single line can be reserved");
        assert(((va & 0x7f) + Len) <= 128);
        auto ptr = getMemoryPointer(va, Len);
        memcpy(buf, ptr, Len);
        granule->line = line.line;
        granule->notify = notify;
        line.line->granules.push_back(granule);
        _rmap.unlock(line);
    }

    template <auto Len, bool Unconditional = false>
    bool writeCond(ps3_uintptr_t va, void* buf) {
        static_assert(Len == 4 || Len == 8 || Len == 128);
        auto line = _rmap.lock<Len>(va);
        assert(!line.nextLine && "only a single line can be reserved");
        if constexpr(!Unconditional)
            assert(g_state.granule);
        if (Unconditional ||
            std::find(begin(line.line->granules), end(line.line->granules), g_state.granule) !=
                end(line.line->granules)) {
            auto ptr = getMemoryPointer(va, Len);
            memcpy(ptr, buf, Len);
            _rmap.destroySingleLine(line.line);
            _rmap.unlock(line);
            _mmap.mark<Len>(va);
            return true;
        }
        _rmap.unlock(line);
        return false;
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
