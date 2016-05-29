#include "../ps3emu/MainMemory.h"
#include "../ps3emu/ppu/PPUThread.h"
#include "../ps3emu/ppu/ppu_dasm.h"
#include "../ps3emu/libs/sys.h"
#include <catch.hpp>

TEST_CASE("sys_memory_allocate") {
    MainMemory mm;
    PPUThread th(&mm);
    mm.setMemory(0x400000, 0, 4, true);
    th.setGPR(11, 348);
    th.setGPR(3, 1 << 21);
    th.setGPR(4, SYS_MEMORY_GRANULARITY_1M);
    th.setGPR(5, 0x400000);
    uint8_t instr[] = { 0x44, 0x00, 0x00, 0x02 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(3) == 0 );
    uint64_t va;
    mm.readMemory(0x400000, &va, 4);
    REQUIRE( va > 0 );
}