#include "ps3emu/spu/SPUDasm.h"
#include "ps3emu/spu/SPUThread.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/state.h"
#include <catch/catch.hpp>

TEST_CASE("spudasm_shufb") {
/*
    shufb r16,r18,r17,r19
*/
    MainMemory mm;
    g_state.mm = &mm;
    SPUThread th;
    
    th.r(18).dw<0>() = 0x1122334455667788ull;
    th.r(18).dw<1>() = 0x99aabbccddeeff00ull;
    th.r(17).dw<0>() = 0x1020304050607080ull;
    th.r(17).dw<1>() = 0x90a0b0c0d0e0f000ull;
    th.r(19).dw<0>() = 0x0002030303030303ull;
    th.r(19).w<2>() = 0x10111253u;
    th.r(19).w<3>() = 0b10000000110000001110000000100000u;
    
    uint8_t instr[] { 0xb2, 0x04, 0x49, 0x13 };
    SPUDasm<DasmMode::Emulate>(instr, 0, &th);
    
    REQUIRE( th.r(16).dw<0>() == 0x1133444444444444 );
    REQUIRE( th.r(16).dw<1>() == 0x1020304000ff8011 );
}
