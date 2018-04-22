#include <catch.hpp>

#include "ps3emu/libs/audio/AudioAttributes.h"
#include <vector>
#include <tuple>

TEST_CASE("audioattributes_test") {
    std::vector<std::tuple<uint64_t, uint64_t>> ring;
    std::vector<uint8_t> mem(0x400);

    auto tag_shift = (big_uint32_t*)&mem[0x1b0];
    auto sizePtr = (big_uint64_t*)&mem[0x158];
    auto lastOffset = (big_uint64_t*)&mem[0x1b8];
    auto read = [&](uint64_t tag) {
        uint64_t offset = (uint64_t)*lastOffset & 0xff;
        uint64_t last = (uint64_t)*lastOffset >> 8;
        auto size = (uint64_t)*sizePtr;
        uint64_t x = ((last - (size + tag)) >> (uint64_t)*tag_shift) + offset;
        if (x > 0xf) {
            x -= 0xf;
        }
        uint64_t res = *(big_uint64_t*)&mem[0x1b8 + (x << 3)];
        return res;
    };

    AudioAttributes attrs(&mem[0]);

    auto add = [&](uint64_t tag, uint64_t timestamp) {
        if (ring.size() == 0xe)
            ring.erase(begin(ring));
        ring.push_back({tag, timestamp});
    };

    auto compare = [&] {
        for (auto [t, ts] : ring) {
            REQUIRE(read(t) == ts);
        }
    };

    uint64_t tag = 0x21, ts = 0x10000;
    attrs.setTag(tag);
    for (auto i = 0u; i < 100; ++i) {
        compare();
        attrs.insertTimestamp(ts);
        add(tag, ts);
        tag += 2;
        ts += 0x100;
    }
}
