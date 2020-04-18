#include <catch2/catch.hpp>

#include "ps3emu/libs/audio/AudioAttributes.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/utils/ranges.h"
#include "TestUtils.h"
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

TEST_CASE("playaudio", TAG_SERIAL) {
    test_interpreter_and_rewriter({testPath("playaudio/a.elf")},
        "cellSysmoduleInitialize() : 0\n"
        "[Audio environment: 2-ch]\n"
        "cellAudioInit() : 0\n"
        "opening first port\n"
        "cellAudioPortOpen() : 0  port 0\n"
        "cellAudioGetPortConfig() : 0\n"
        "readIndexAddr=10102c10\n"
        "status=1\n"
        "nChannel=2\n"
        "nBlock=8\n"
        "portSize=4000\n"
        "cellAudioCreateNotifyEventQueue() : 0\n"
        "cellAudioSetNotifyEventQueue() : 0\n"
        "cellAudioPortStart() : 0\n"
        "cellAudioPortStop() : 0\n"
        "soundMain exit = 0\n"
        "cellAudioDeleteNotifyEventQueue() : 0\n"
        "sys_event_queue_destroy() : 0\n"
        "cellAudioPortClose() : 0\n"
        "cellAudioQuit() : 0\n", true, { "--capture-audio" }
    );
    auto expected = read_all_bytes(testPath("playaudio/host_root/usr/local/cell/sample_data/sound/waveform/Sample-48k-stereo.raw"));
    auto actual = read_all_bytes("/tmp/ps3emu_audio_port0.bin");
    REQUIRE(actual.size() > 1500 * 1024);
    actual.erase(begin(actual), begin(actual) + 1024 * 100);
    actual.erase(end(actual) - 1024 * 100, end(actual));
    auto it = std::search(begin(expected), end(expected), begin(actual), end(actual));
    bool found = it != end(expected);
    REQUIRE(found);
}
