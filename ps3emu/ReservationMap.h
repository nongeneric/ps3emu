#pragma once

#include "ps3emu/utils/SpinLock.h"

#include <stdint.h>
#include <memory>
#include <atomic>
#include <functional>
#include <assert.h>
#include <vector>
#include <string>

struct ReservationGranule;

using lost_notify_t = void(*)(uintptr_t arg1, uintptr_t arg2);

template<typename ReservationGranule>
class ReservationGranuleArray {
    std::array<ReservationGranule*, 64> _arr;
    size_t _size = 0;

    void notify(ReservationGranule* granule) {
        if (granule->notify) {
            granule->notify(granule->arg1, granule->arg2);
        }
        granule->notify = 0;
    }

public:
    inline ReservationGranuleArray() {
        std::fill(begin(_arr), end(_arr), nullptr);
    }

    inline void insert(ReservationGranule* granule) {
        for (auto i = 0u; i < _size; ++i) {
            auto& g = _arr[i];
            if (!g) {
                g = granule;
                return;
            }
        }
        _size++;
        _arr[_size - 1] = granule;
    }

    inline void clear() {
        for (auto i = 0u; i < _size; ++i) {
            auto granule = _arr[i];
            if (granule) {
                notify(granule);
                granule->line = nullptr;
            }
        }
        _size = 0;
    }

    inline void clearExcept(ReservationGranule* exceptGranule) {
        auto newSize = 0u;
        for (auto i = 0u; i < _size; ++i) {
            auto& granule = _arr[i];
            if (granule) {
                if (granule == exceptGranule) {
                    newSize = i + 1;
                } else {
                    notify(granule);
                    granule->line = nullptr;
                    granule = nullptr;
                }
            }
        }
        _size = newSize;
    }

    inline void clearOne(ReservationGranule* targetGranule) {
        for (auto i = 0u; i < _size; ++i) {
            auto& granule = _arr[i];
            if (granule == targetGranule) {
                notify(granule);
                granule->line = nullptr;
                granule = nullptr;
            }
        }
    }

    inline bool exists(ReservationGranule* granule) {
        for (auto i = 0u; i < _size; ++i) {
            if (_arr[i] == granule)
                return true;
        }
        return false;
    }
};

struct ReservationLine {
    SpinLock lock; 
    ReservationGranuleArray<ReservationGranule> granules;
};

struct ReservationGranule {
    ReservationLine *line = nullptr;
    lost_notify_t notify = nullptr;
    uintptr_t arg1 = 0;
    uintptr_t arg2 = 0;
    std::string dbgName;

    inline ~ReservationGranule() {
        auto local = line;
        if (local) {
            local->lock.lock();
            local->granules.clear();
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
        granule->line->granules.clearOne(granule);
    }
    
    inline void destroySingleLine(ReservationLine* line) {
        line->granules.clear();
    }
    
    inline void destroyExcept(LockedLine lockedLine, ReservationGranule* granule) {
        lockedLine.line->granules.clearExcept(granule);
        if (lockedLine.nextLine) {
            lockedLine.nextLine->granules.clearExcept(granule);
        }
    }
};
