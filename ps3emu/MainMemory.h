#pragma once

#include "int.h"
#include "BitField.h"
#include "constants.h"
#include "utils.h"
#include "ReservationMap.h"
#include "ModificationMap.h"
#include "state.h"
#include "ps3emu/utils/debug.h"
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
    std::string name;
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
    inline MemoryPage() : ptr(0) { }
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
    void dealloc();

    template<int Len>
    inline typename IntTraits<Len>::Type load(ps3_uintptr_t va, bool validate);
    template<int Len>
    inline void store(ps3_uintptr_t va, typename IntTraits<Len>::Type value, ReservationGranule* granule);

    inline bool writeSpecialMemory(ps3_uintptr_t va, const void* buf, uint len) {
        if (unlikely((va & SpuThreadBaseAddr) == SpuThreadBaseAddr)) {
            writeSpuThreadVa(va, buf, len);
            return true;
        }
        if (unlikely((va & RawSpuBaseAddr) == RawSpuBaseAddr)) {
            writeRawSpuVa(va, buf, len);
            return true;
        }
        return false;
    }

    inline bool readSpecialMemory(ps3_uintptr_t va, void* buf) {
        if (unlikely((va & RawSpuBaseAddr) == RawSpuBaseAddr)) {
            *(big_uint32_t*)buf = readRawSpuVa(va);
            return true;
        }
        return false;
    }

public:
    MainMemory();
    ~MainMemory();
    void writeMemory(ps3_uintptr_t va,
                     const void* buf,
                     uint len);
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
    void store8(ps3_uintptr_t va, uint8_t value, ReservationGranule* granule);
    void store16(ps3_uintptr_t va, uint16_t value, ReservationGranule* granule);
    void store32(ps3_uintptr_t va, uint32_t value, ReservationGranule* granule);
    void store64(ps3_uintptr_t va, uint64_t value, ReservationGranule* granule);
    void store128(ps3_uintptr_t va, uint128_t value, ReservationGranule* granule);
    
    inline void storef(ps3_uintptr_t va, float value, ReservationGranule* granule) {
        store32(va, bit_cast<uint32_t>(value), granule);
    }
    
    inline void stored(ps3_uintptr_t va, double value, ReservationGranule* granule) {
        store64(va, bit_cast<uint64_t>(value), granule);
    }
    
    inline float loadf(ps3_uintptr_t va) {
        auto f = load32(va);
        return bit_cast<float>(f);
    }
    
    inline double loadd(ps3_uintptr_t va) {
         auto f = load64(va);
         return bit_cast<double>(f);
    }
    
    inline auto modificationMap() {
        return &_mmap;
    }

    template <auto Len>
    inline void loadReserve(ps3_uintptr_t va,
                            void* buf,
                            lost_notify_t notify = 0,
                            uintptr_t arg1 = 0,
                            uintptr_t arg2 = 0) {
        static_assert(Len == 4 || Len == 8 || Len == 128);
        if constexpr(Len == 4)
            EMU_ASSERT((va & 0b11) == 0);
        if constexpr(Len == 8)
            EMU_ASSERT((va & 0b111) == 0);
        if constexpr(Len == 128) {
            EMU_ASSERT((va & 0x7f) == 0);
        }
        
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
        
        ReservationLine *line, *nextLine;
        _rmap.lock<Len>(va, &line, &nextLine);
        assert(!nextLine && "only a single line can be reserved");
        assert(((va & 0x7f) + Len) <= 128);
        auto ptr = getMemoryPointer(va, Len);
        memcpy(buf, ptr, Len);
        granule->line = line;
        granule->notify = notify;
        granule->arg1 = arg1;
        granule->arg2 = arg2;
        line->granules.insert(granule);
        _rmap.unlock(line, nextLine);
    }

    template <auto Len, bool Unconditional = false>
    bool writeCond(ps3_uintptr_t va, void* buf) {
        static_assert(Len == 4 || Len == 8 || Len == 128);
        ReservationLine *line, *nextLine;
        _rmap.lock<Len>(va, &line, &nextLine);
        assert(!nextLine && "only a single line can be reserved");
        if constexpr(!Unconditional)
            assert(g_state.granule);
        if (Unconditional || line->granules.exists(g_state.granule)) {
            auto ptr = getMemoryPointer(va, Len);
            memcpy(ptr, buf, Len);
            _rmap.destroySingleLine(line);
            _rmap.unlock(line, nextLine);
            _mmap.mark<Len>(va);
            return true;
        }
        _rmap.unlock(line, nextLine);
        return false;
    }
    
#ifdef MEMORY_PROTECTION
    struct MemoryBreakpointInfo {
        uint32_t ea;
        uint32_t len;
        bool write;
    };
    std::vector<MemoryBreakpointInfo> _memoryBreakpoints;
    void dbgMemoryBreakpoint(uint32_t ea, int32_t len, bool write);
    void reportViolation(uint32_t ea, uint32_t len, bool write);
    void mark(uint32_t ea, uint32_t len, bool readonly, std::string comment);
    void unmark(uint32_t ea, uint32_t len);
    void validate(uint32_t ea, uint32_t len, bool write);
    ProtectionRange addressRange(uint32_t ea);
#else
    inline void dbgMemoryBreakpoint(uint32_t ea, int32_t len, bool write) { }
    inline void mark(uint32_t ea, uint32_t len, bool readonly, std::string comment) { }
    inline void unmark(uint32_t ea, uint32_t len) { }
    inline void validate(uint32_t ea, uint32_t len, bool write) { }
    inline ProtectionRange addressRange(uint32_t ea) { return {}; }
#endif
};

uint32_t calcFnid(const char* name);
uint32_t calcEid(const char* name);
uint32_t encodeNCall(MainMemory* mm, ps3_uintptr_t va, uint32_t index);
const NCallEntry* findNCallEntry(uint32_t fnid, uint32_t& index, bool assertFound = false);
const NCallEntry* findNCallEntryByIndex(uint32_t index);
uint32_t addNCallEntry(NCallEntry entry);
void readString(MainMemory* mm, uint32_t va, std::string& str);
