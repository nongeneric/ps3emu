#pragma once

#include <stdint.h>
#include <memory>
#include <atomic>
#include <functional>
#include <assert.h>
#include <vector>
#include <string>

class SpinLock {
    std::atomic_flag _flag = ATOMIC_FLAG_INIT;

public:
    inline void lock() {
        while (_flag.test_and_set(std::memory_order_acquire)) ;
    }
    
    inline void unlock() {
        _flag.clear(std::memory_order_release);
    }
};

struct ReservationGranule;

using lost_notify_t = std::function<void()>;

struct ReservationLine {
    SpinLock lock; 
    std::vector<ReservationGranule*> granules;
};

struct ReservationGranule {
    ReservationLine *line = nullptr;
    lost_notify_t notify = {};
    std::string dbgName;
    
    inline ~ReservationGranule() {
        auto local = line;
        if (local) {
            local->lock.lock();
            auto it = std::find(begin(local->granules), end(local->granules), this);
            if (it != end(local->granules))
                local->granules.erase(it);
            local->lock.unlock();
        }
    }
};

struct LockedLine {
    ReservationLine* line;
    ReservationLine* nextLine;
};

// Assume all reservations are naturally aligned. That is, data of length 4 must be aligned to 4,
// data of length 8 must be aligned to 8, and so on.
// In other words, no reservation might cross a cache line.
// This doesn't prevent a lock to be placed across cache line boundaries. Unaligned writes are
// legal and supported.

class ReservationMap {
    static constexpr unsigned _cacheLine = 128;
    static constexpr unsigned _slotNumber = 0xffffffff / _cacheLine;
    std::unique_ptr<ReservationLine[]> _lines;
    
    inline void destroySingleLineExcept(ReservationLine* line, ReservationGranule* exceptGranule) {
        if (line->granules.empty())
            return;
        auto e = end(line->granules);
        auto it = std::find(begin(line->granules), e, exceptGranule);
        for (auto granule : line->granules) {
            if (granule == exceptGranule)
                continue;
            if (granule->notify)
                granule->notify();
            granule->notify = {};
            granule->line = nullptr;
        }
        line->granules.clear();
        if (it != e)
            line->granules.push_back(*it);
    }
    
public:
    inline ReservationMap() {
        _lines.reset(new ReservationLine[_slotNumber]);
    }
    
    template <auto Len>
    LockedLine lock(uint32_t va) {
        static_assert(Len == 1 || Len == 2 || Len == 4 || Len == 8 || Len == 16 ||
                      Len == 128);
        auto index = va >> 7;
        auto line = &_lines[index];
        line->lock.lock();
        if ((va & 0x7f) + Len > 128) {
            //INFO(spu) << ssnprintf("locking second line %x", va);
            auto next = &_lines[index + 1];
            next->lock.lock();
            return {line, next};
        }
        return {line, nullptr};
    }
    
    inline void unlock(LockedLine lockedLine) {
        if (lockedLine.nextLine) {
            lockedLine.nextLine->lock.unlock();
        }
        lockedLine.line->lock.unlock();
    }
    
    inline void destroySingleReservation(ReservationGranule* granule) {
        auto line = granule->line;
        auto it = std::find(begin(line->granules), end(line->granules), granule);
        assert(it != end(line->granules));
        if (granule->notify)
            granule->notify();
        granule->notify = {};
        granule->line = nullptr;
        line->granules.erase(it);
    }
    
    inline void destroySingleLine(ReservationLine* line) {
        for (auto granule : line->granules) {
            if (granule->notify)
                granule->notify();
            granule->notify = {};
            granule->line = nullptr;
        }
        line->granules.clear();
    }
    
    inline void destroyExcept(LockedLine lockedLine, ReservationGranule* granule) {
        destroySingleLineExcept(lockedLine.line, granule);
        if (lockedLine.nextLine)
            destroySingleLineExcept(lockedLine.nextLine, granule);
    }
};
