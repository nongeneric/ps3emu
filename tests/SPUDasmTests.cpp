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

TEST_CASE("spudasm_bg") {
/*
    bg r20,r19,r18
*/
    MainMemory mm;
    g_state.mm = &mm;
    SPUThread th;
    
    uint8_t instr[] { 0x08, 0x44, 0x89, 0x94 };
    
    th.r(19).w<0>() = 0x00000000;
    th.r(18).w<0>() = 0x00000000;
    
    th.r(19).w<1>() = 0x00000001;
    th.r(18).w<1>() = 0x00000000;
    
    th.r(19).w<2>() = 0x00000000;
    th.r(18).w<2>() = 0x00000001;
    
    th.r(19).w<3>() = 0xffffffff;
    th.r(18).w<3>() = 0xffffffff;
    
    SPUDasm<DasmMode::Emulate>(instr, 0, &th);
    
    REQUIRE( th.r(20).w<0>() == 1 );
    REQUIRE( th.r(20).w<1>() == 0 );
    REQUIRE( th.r(20).w<2>() == 1 );
    REQUIRE( th.r(20).w<3>() == 1 );
    
    th.r(19).w<0>() = 0xff000000;
    th.r(18).w<0>() = 0x000000ff;
    
    th.r(19).w<1>() = 0x000000ff;
    th.r(18).w<1>() = 0xff000000;
    
    th.r(19).w<2>() = 0x80000000;
    th.r(18).w<2>() = 0x80000000;
    
    th.r(19).w<3>() = 0x80000001;
    th.r(18).w<3>() = 0x80000000;
    
    SPUDasm<DasmMode::Emulate>(instr, 0, &th);
    
    REQUIRE( th.r(20).w<0>() == 0 );
    REQUIRE( th.r(20).w<1>() == 1 );
    REQUIRE( th.r(20).w<2>() == 1 );
    REQUIRE( th.r(20).w<3>() == 0 );
}
