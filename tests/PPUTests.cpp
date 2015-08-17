#include "../ps3emu/PPU.h"
#include "../ps3emu/ppu_dasm.h"
#include <catch.hpp>

TEST_CASE("read write memory") {
    PPU ppu;
    uint32_t original = 0xA1B2C3D4, read;
    ppu.writeMemory(0x400, &original, 4, true);
    ppu.readMemory(0x400, &read, 4);
    REQUIRE(original == read);
    
    ppu.store<4>(0x400, 0x1122334455667788);
    REQUIRE(ppu.load<4>(0x400) == 0x55667788);
}

TEST_CASE("list1") {
/*
10350:       38 40 00 0c     li      r2,12
10354:       38 60 00 07     li      r3,7
10358:       38 62 00 0a     addi    r3,r2,10
1035c:       7c 83 12 14     add     r4,r3,r2
*/
    PPU ppu;
    uint64_t cia = 0x10350;
    uint8_t instr[] = {
        0x38, 0x40, 0x00, 0x0c,
        0x38, 0x60, 0x00, 0x07,
        0x38, 0x62, 0x00, 0x0a,
        0x7c, 0x83, 0x12, 0x14
    };
    for (auto i = 0u; i < sizeof(instr); i += 4) {
        ppu_dasm<DasmMode::Emulate>(instr + i, cia + i, &ppu);
    }
    REQUIRE(ppu.getGPR(2) == 12);
    REQUIRE(ppu.getGPR(3) == 22);
    REQUIRE(ppu.getGPR(4) == 34);
}

TEST_CASE("list2") {
/*
10350:       38 60 00 37     li      r3,55           # 37
10354:       38 80 00 1d     li      r4,29           # 1d
10358:       48 00 00 0d     bl      0x10364
1035c:       60 00 00 00     nop
# int func(int r3, int r4) { return (r4 >> 2) + (r3 << 5); }
10360:       48 00 00 14     b       0x10374
10364:       78 63 f0 82     rldicl  r3,r3,62,2
10368:       78 84 28 00     rotldi  r4,r4,5
1036c:       7c 63 22 14     add     r3,r3,r4
10370:       4e 80 00 20     blr
10374:       60 00 00 00     nop
*/
    PPU ppu;
    auto base = 0x10350;
    ppu.setNIP(base);
    uint8_t instr[] = {
          0x38, 0x60, 0x00, 0x37
        , 0x38, 0x80, 0x00, 0x1d
        , 0x48, 0x00, 0x00, 0x0d
        , 0x60, 0x00, 0x00, 0x00
        , 0x48, 0x00, 0x00, 0x14
        , 0x78, 0x63, 0xf0, 0x82
        , 0x78, 0x84, 0x28, 0x00
        , 0x7c, 0x63, 0x22, 0x14
        , 0x4e, 0x80, 0x00, 0x20
        , 0x60, 0x00, 0x00, 0x00
    };
    for (auto i = 0u; i < sizeof(instr); i += 4) {
        auto nip = ppu.getNIP();
        ppu.setNIP(nip + 4);
        ppu_dasm<DasmMode::Emulate>(instr + nip - base, nip, &ppu);
    }
    REQUIRE(ppu.getNIP() == 0x10378);
    REQUIRE(ppu.getGPR(3) == (55 >> 2) + (29 << 5));
}

TEST_CASE("emu stw") {
    PPU ppu;
    ppu.setGPR(29, 0x1122334455667788);
    ppu.setGPR(11, 0x400000);
    ppu.setMemory(0x400000, 0, 8, true);
    uint8_t instr[] = { 0x93, 0xab, 0x00, 0x00 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE(ppu.load<4>(0x400000) == 0x55667788); 
}

TEST_CASE("emu mtctr") {
    PPU ppu;
    ppu.setGPR(0, 0x11223344);
    uint8_t instr[] = { 0x7c, 0x09, 0x03, 0xa6 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE(ppu.getCTR() == 0x11223344); 
}

TEST_CASE("mask") {
    REQUIRE(mask<8>(1, 6) == 0x7e);
    REQUIRE(mask<64>(63, 63) == 1);
    REQUIRE(mask<64>(32, 63) == 0xffffffff);
    REQUIRE(mask<5>(0, 2) == 28);
    REQUIRE(mask<5>(2, 0) == 23);
    REQUIRE(mask<5>(4, 0) == 17);
}

TEST_CASE("bit_test") {
    REQUIRE(bit_test(21, 6, 0) == 0);
    REQUIRE(bit_test(21, 6, 1) == 1);
    REQUIRE(bit_test(21, 6, 2) == 0);
    REQUIRE(bit_test(21, 6, 3) == 1);
    REQUIRE(bit_test(21, 6, 5) == 1);
}

TEST_CASE("emu cmpld") {
    PPU ppu;
    ppu.setCR(0);
    ppu.setGPR(30, 10);
    ppu.setGPR(8, 10);
    // cmpld cr7,r30,r8
    uint8_t instr[] = { 0x7f, 0xbe, 0x40, 0x40 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE((ppu.getCR() & 0xf) == 2);
    ppu.setGPR(30, 20);
    ppu.setGPR(8, 10);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE((ppu.getCR() & 0xf) == 4);
    ppu.setGPR(30, 20);
    ppu.setGPR(8, 30);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE((ppu.getCR() & 0xf) == 8);
}

TEST_CASE("list3") {
    /*
0000000000000000 <loop-0x1035c>:
        ...
   10350:       38 60 00 00     li      r3,0
   10354:       38 80 00 03     li      r4,3
   10358:       38 40 00 05     li      r2,5

000000000001035c <loop>:
   1035c:       98 47 00 00     stb     r2,0(r7)
   10360:       7f a4 18 40     cmpld   cr7,r4,r3
   10364:       38 e7 00 01     addi    r7,r7,1
   10368:       38 63 00 01     addi    r3,r3,1
   1036c:       40 9c ff f0     bge     cr7,1035c <loop>
   10370:       60 00 00 00     nop
     */
    PPU ppu;
    auto base = 0x10350;
    auto mem = 0x400000;
    ppu.setNIP(base);
    ppu.setMemory(mem, 0, 4, true);
    ppu.setGPR(7, mem);
    uint8_t instr[] = {
          0x38, 0x60, 0x00, 0x00
        , 0x38, 0x80, 0x00, 0x03
        , 0x38, 0x40, 0x00, 0x05
        , 0x98, 0x47, 0x00, 0x00
        , 0x7f, 0xa4, 0x18, 0x40
        , 0x38, 0xe7, 0x00, 0x01
        , 0x38, 0x63, 0x00, 0x01
        , 0x40, 0x9c, 0xff, 0xf0
        , 0x60, 0x00, 0x00, 0x00
    };
    int i = 0;
    for (;; i++) {
        if (i == 50 || ppu.getNIP() == 0x10370)
            break;
        auto nip = ppu.getNIP();
        ppu.setNIP(nip + 4);
        ppu_dasm<DasmMode::Emulate>(instr + nip - base, nip, &ppu);
    }
    REQUIRE(i < 50);
    REQUIRE(ppu.getNIP() == 0x10370);
    REQUIRE(ppu.load<4>(mem) == 0x05050505);
}

TEST_CASE("set crf") {
    PPU ppu;
    ppu.setCR(0);
    ppu.setCRF_sign(7, 4);
    REQUIRE(ppu.getCR() == 8);
    ppu.setCR(0xf);
    ppu.setCRF_sign(7, 4);
    REQUIRE(ppu.getCR() == 9);
}

TEST_CASE("branch info bge") {
    PPU ppu;
    ppu.setGPR(10, ~0ull);
    // bge cr7,103b4
    uint8_t instr[] = { 0x40, 0x9c, 0xff, 0xe0 };
    REQUIRE(isAbsoluteBranch(instr));
    REQUIRE(getTargetAddress(instr, 0x103d4) == 0x103b4);
    ppu.setCRF_sign(7, 2);
    REQUIRE(isTaken(instr, 0x103d4, &ppu));
    ppu.setCRF_sign(7, 4);
    REQUIRE(!isTaken(instr, 0x103d4, &ppu));
}