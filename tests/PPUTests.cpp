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

TEST_CASE("neg") {
    PPU ppu;
    ppu.setGPR(3, ~0ull);
    // neg r0,r3
    uint8_t instr[] = { 0x7c, 0x03, 0x00, 0xd0 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE( ppu.getGPR(0) == 1 );
}

TEST_CASE("strlen") {
/*   
0000000000017ff4 <.strlen>:
   17ff4:       7c 03 00 d0     neg     r0,r3
   17ff8:       78 00 07 60     clrldi  r0,r0,61                # 3d
   17ffc:       7c a5 2a 78     xor     r5,r5,r5
   18000:       28 00 00 00     cmplwi  r0,0
   18004:       7c 64 1b 78     mr      r4,r3
   18008:       41 82 00 1c     beq     18024 <.strlen+0x30>
   1800c:       7c 09 03 a6     mtctr   r0
   18010:       88 04 00 00     lbz     r0,0(r4)
   18014:       7c 25 00 00     cmpd    r5,r0
   18018:       38 84 00 01     addi    r4,r4,1
   1801c:       41 82 00 70     beq     1808c <.strlen+0x98>
   18020:       42 00 ff f0     bdnz    18010 <.strlen+0x1c>
   18024:       38 c0 00 80     li      r6,128          # 80
   18028:       7c 00 22 2c     dcbt    r0,r4
   1802c:       7c 06 22 2c     dcbt    r6,r4
   18030:       38 c0 7f 7f     li      r6,32639                # 7f7f
   18034:       78 c6 80 2c     rldimi  r6,r6,16,32             # 20
   18038:       39 00 00 10     li      r8,16
   1803c:       78 c6 00 0e     rldimi  r6,r6,32,0
   18040:       38 04 01 00     addi    r0,r4,256               # 100
   18044:       7c 00 02 2c     dcbt    r0,r0
   18048:       7d 09 03 a6     mtctr   r8
   1804c:       e8 04 00 00     ld      r0,0(r4)
   18050:       7c 07 30 38     and     r7,r0,r6
   18054:       7c 09 33 78     or      r9,r0,r6
   18058:       7d 47 32 14     add     r10,r7,r6
   1805c:       7d 2b 50 f8     nor     r11,r9,r10
   18060:       7d 60 00 74     cntlzd  r0,r11
   18064:       78 00 e8 c2     rldicl  r0,r0,61,3
   18068:       28 00 00 08     cmplwi  r0,8
   1806c:       38 84 00 08     addi    r4,r4,8
   18070:       41 80 00 0c     blt     1807c <.strlen+0x88>
   18074:       42 00 ff d8     bdnz    1804c <.strlen+0x58>
   18078:       4b ff ff c8     b       18040 <.strlen+0x4c>
   1807c:       38 84 ff f8     addi    r4,r4,-8                # fffffff8
   18080:       7c 84 02 14     add     r4,r4,r0
   18084:       7c 63 20 50     subf    r3,r3,r4
   18088:       4e 80 00 20     blr
   1808c:       38 84 ff ff     addi    r4,r4,-1
   18090:       7c 63 20 50     subf    r3,r3,r4
   18094:       4e 80 00 20     blr
*/
    uint8_t instr[] = { 
        0x7c, 0x03, 0x00, 0xd0, 0x78, 0x00, 0x07, 0x60, 0x7c, 0xa5, 0x2a, 0x78,
        0x28, 0x00, 0x00, 0x00, 0x7c, 0x64, 0x1b, 0x78, 0x41, 0x82, 0x00, 0x1c,
        0x7c, 0x09, 0x03, 0xa6, 0x88, 0x04, 0x00, 0x00, 0x7c, 0x25, 0x00, 0x00,
        0x38, 0x84, 0x00, 0x01, 0x41, 0x82, 0x00, 0x70, 0x42, 0x00, 0xff, 0xf0,
        0x38, 0xc0, 0x00, 0x80, 0x7c, 0x00, 0x22, 0x2c, 0x7c, 0x06, 0x22, 0x2c,
        0x38, 0xc0, 0x7f, 0x7f, 0x78, 0xc6, 0x80, 0x2c, 0x39, 0x00, 0x00, 0x10,
        0x78, 0xc6, 0x00, 0x0e, 0x38, 0x04, 0x01, 0x00, 0x7c, 0x00, 0x02, 0x2c,
        0x7d, 0x09, 0x03, 0xa6, 0xe8, 0x04, 0x00, 0x00, 0x7c, 0x07, 0x30, 0x38,
        0x7c, 0x09, 0x33, 0x78, 0x7d, 0x47, 0x32, 0x14, 0x7d, 0x2b, 0x50, 0xf8,
        0x7d, 0x60, 0x00, 0x74, 0x78, 0x00, 0xe8, 0xc2, 0x28, 0x00, 0x00, 0x08,
        0x38, 0x84, 0x00, 0x08, 0x41, 0x80, 0x00, 0x0c, 0x42, 0x00, 0xff, 0xd8,
        0x4b, 0xff, 0xff, 0xc8, 0x38, 0x84, 0xff, 0xf8, 0x7c, 0x84, 0x02, 0x14,
        0x7c, 0x63, 0x20, 0x50, 0x4e, 0x80, 0x00, 0x20, 0x38, 0x84, 0xff, 0xff,
        0x7c, 0x63, 0x20, 0x50, 0x4e, 0x80, 0x00, 0x20
    };
    PPU ppu;
    auto base = 0x17ff4;
    ppu.setNIP(base);
    ppu.setLR(0);
    const char* str = "hello there";
    ppu.writeMemory(0x400000, str, strlen(str) + 1, true);
    ppu.setGPR(3, 0x400000);
    for (;;) {
        if (ppu.getNIP() == 0)
            break;
        auto nip = ppu.getNIP();
        ppu.setNIP(nip + 4);
        ppu_dasm<DasmMode::Emulate>(instr + nip - base, nip, &ppu);
    }
    REQUIRE( ppu.getGPR(3) == strlen(str) );
}

TEST_CASE("clrldi r0,r0,61") {
    PPU ppu;
    ppu.setGPR(0, ~0ull);
    uint8_t instr[] = { 0x78, 0x00, 0x07, 0x60 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE( ppu.getGPR(0) == 7 );
}

TEST_CASE("cntlzd r0,r11") {
    uint8_t instr[] = { 0x7d, 0x60, 0x00, 0x74 };
    PPU ppu;
    ppu.setGPR(0, 100);
    ppu.setGPR(11, 0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE( ppu.getGPR(0) == 64 );
    ppu.setGPR(11, 1);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE( ppu.getGPR(0) == 63 );
    ppu.setGPR(11, 0xffffffff);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE( ppu.getGPR(0) == 32 );
}

TEST_CASE("nor r11,r9,r10") {
    PPU ppu;
    ppu.setGPR(11, 500);
    ppu.setGPR(9, 0x7f7f7f7f7f7f7f7full);
    ppu.setGPR(10, 0xf2eeece49feef4f3ull);
    uint8_t instr[] = { 0x7d, 0x2b, 0x50, 0xf8 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE( ppu.getGPR(11) == 0 );
}

TEST_CASE("subf r3,r3,r4") {
    PPU ppu;
    ppu.setGPR(3, 500);
    ppu.setGPR(4, 700);
    uint8_t instr[] = { 0x7c, 0x63, 0x20, 0x50 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &ppu);
    REQUIRE( ppu.getGPR(3) == 200 );
}

