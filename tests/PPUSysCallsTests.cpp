#include "../ps3emu/PPU.h"
#include "../ps3emu/ppu_dasm.h"
#include "../libs/sys.h"
#include <catch.hpp>

TEST_CASE("sys_memory_allocate") {
    PPU ppu;
    ppu.setMemory(0x400000, 0, 4, true);
    ppu.setGPR(11, 348);
    ppu.setGPR(3, 1 << 21);
    ppu.setGPR(4, SYS_MEMORY_GRANULARITY_1M);
    ppu.setGPR(5, 0x400000);
    uint8_t instr[] = { 0x44, 0x00, 0x00, 0x02 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE( ppu.getGPR(3) == 0 );
    uint64_t va;
    ppu.readMemory(0x400000, &va, 4);
    REQUIRE( va > 0 );
}