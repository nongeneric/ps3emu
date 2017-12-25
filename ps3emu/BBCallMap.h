#pragma once

#include "ps3emu/int.h"
#include "ps3emu/dasm_utils.h"
#include <array>

class BBCallMap {
    // need space for both 0x00000000 (normal ppu code) and 0x10000000 (emu code)
    std::array<uint32_t, 512u << 20u> _map;
    
public:
    inline void set(uint32_t va, uint32_t bbcall) {
        _map[va / 4] = bbcall;
    }
    
    inline uint32_t get(uint32_t va) {
        return _map[va / 4];
    }

    inline uint32_t safeGet(uint32_t va) {
        return va > 0x20000000 ? 0 : get(va);
    }
    
    inline uint32_t* base() {
        return &_map[0];
    }
};

inline void bbcallmap_dasm(uint32_t val, uint32_t& segment, uint32_t& label) {
    BBCallForm form { val };
    segment = form.Segment.u();
    label = form.Label.u();
}
