#include "../ps3emu/PPU.h"
#include <catch.hpp>

TEST_CASE("read write memory") {
    PPU ppu;
    uint32_t original = 0xA1B2C3D4, read;
    ppu.writeMemory(0x400, &original, 4);
    ppu.readMemory(0x400, &read, 4);
    REQUIRE(original == read);
}