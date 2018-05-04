#pragma once

#include "ps3emu/int.h"

class AudioAttributes {
    uint64_t _tag;
    unsigned _offset;

    static constexpr unsigned maxPorts = 8;
    static constexpr unsigned timestampCount = 0xf;
    static constexpr unsigned tagDelta = 0x23;

    #pragma pack(1)
    struct Layout {
        struct {
            big_uint64_t val;
            big_uint64_t unk;
        } readIndex[maxPorts + 1]; // libAudio index + 1
        big_uint64_t pad_90[6];
        big_uint64_t unk_c0;
        big_uint64_t pad_c8[5];
        big_uint64_t tags[maxPorts]; // the tag corresponding to readIndex
        big_uint64_t pad_130[4];
        big_uint64_t deltas[maxPorts + 1];
        big_uint64_t pad_198[3];
        big_uint32_t tagShift;
        char pad_1b4[3];
        big_uint64_t lastTag; // last tag in the timestamp table
        uint8_t offset;       // offset of the first tag in the timestamp table
        big_uint64_t timestamps[timestampCount]; // timestamp table, tags and their timestamps
                                                 // in descending order
    } * _layout;
    #pragma pack()

    static_assert(offsetof(Layout, pad_90) == 0x90);
    static_assert(offsetof(Layout, unk_c0) == 0xc0);
    static_assert(offsetof(Layout, pad_c8) == 0xc8);
    static_assert(offsetof(Layout, tags) == 0xf0);
    static_assert(offsetof(Layout, pad_130) == 0x130);
    static_assert(offsetof(Layout, deltas) == 0x150);
    static_assert(offsetof(Layout, pad_198) == 0x198);
    static_assert(offsetof(Layout, pad_1b4) == 0x1b4);
    static_assert(offsetof(Layout, offset) == 0x1bf);
    static_assert(offsetof(Layout, timestamps) == 0x1c0);

public:
    static constexpr unsigned portHwBase = 5;
    static constexpr unsigned fixedLatency = 3;

    AudioAttributes() = default;
    AudioAttributes(uint8_t* mem);
    void setReadIndex(unsigned port, unsigned index);
    unsigned getReadIndex(unsigned port);
    void setTag(uint64_t tag);
    void insertTimestamp(uint64_t timestamp);
};
