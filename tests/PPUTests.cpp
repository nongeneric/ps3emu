#include "ps3emu/MainMemory.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/ppu/PPUThread.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3emu/rsx/GLTexture.h"
#include "ps3emu/state.h"
#include "ps3emu/int.h"
#include "ps3emu/ModificationMap.h"
#include <vector>
#include <atomic>
#include <catch/catch.hpp>

TEST_CASE("modification_map_simple") {
    ModificationMap<32, 1024> map;
    map.mark<4>(64);
    REQUIRE(map.marked(64, 1));
    REQUIRE(map.marked(80, 32));
    
    map.reset(64, 32);
    REQUIRE(!map.marked(64, 1));
    
    map.mark(128, 128);
    REQUIRE(map.marked(128, 1));
    REQUIRE(map.marked(255, 1));
    REQUIRE(map.marked(0, 512));
    REQUIRE(!map.marked(512, 1));
    
    map.mark(0, 1024 - 32);
    REQUIRE(map.marked(0, 1024 - 32));
    map.reset(0, 1024 - 32);
    REQUIRE(!map.marked(0, 1024 - 32));
    
    map.mark(64, 110);
    REQUIRE(map.marked(64, 110));
    map.reset(64, 110);
    REQUIRE(!map.marked(64, 110));
    
    map.mark(60, 110);
    REQUIRE(map.marked(60, 110));
    map.reset(60, 110);
    REQUIRE(!map.marked(60, 110));
    
    map.mark(60, 8);
    REQUIRE(map.marked(60, 8));
    map.reset(60, 8);
    REQUIRE(!map.marked(60, 8));
    
    map.mark(60, 2);
    REQUIRE(map.marked(60, 2));
    map.reset(60, 2);
    REQUIRE(!map.marked(60, 2));
}

TEST_CASE("constexpr_log_test") {
    static_assert(constexpr_log2(1u << 20) == 20);
    static_assert(constexpr_log2(1u) == 0);
    static_assert(constexpr_log2(2) == 1);
    static_assert(constexpr_log2(10) == 3);
    REQUIRE(constexpr_log2(1u << 20) == 20);
    REQUIRE(constexpr_log2(1u) == 0);
    REQUIRE(constexpr_log2(2) == 1);
    REQUIRE(constexpr_log2(10) == 3);
}

TEST_CASE("swizzle_coord_convert") {
    SwizzledTextureIterator it(nullptr, 3, 3, 0);
    struct {
        unsigned x;
        unsigned y;
        unsigned unswizzledX;
        unsigned unswizzledY;
    } in[] = {
        { 0, 0, 0, 0 },
        { 1, 0, 1, 0 },
        { 2, 0, 0, 1 },
        { 3, 0, 1, 1 },
        { 4, 0, 2, 0 },
        { 5, 0, 3, 0 },
        { 6, 0, 2, 1 },
        { 7, 0, 3, 1 },
        { 0, 1, 0, 2 },
        { 1, 1, 1, 2 },
        { 2, 1, 0, 3 },
        { 3, 1, 1, 3 },
        { 4, 1, 2, 2 },
        { 5, 1, 3, 2 },
        { 6, 1, 2, 3 },
        { 7, 1, 3, 3 },
        { 0, 2, 4, 0 },
    };
    for (auto i = std::begin(in); i != std::end(in); ++i) {
        unsigned x, y;
        std::tie(x, y) = it.unswizzle(i->x, i->y);
        REQUIRE( x == i->unswizzledX );
        REQUIRE( y == i->unswizzledY );
    }
}

TEST_CASE("read_write_memory") {
    MainMemory mm;
    mm.mark(0, 0x1000, false, "");
    uint32_t original = 0xA1B2C3D4, read;
    mm.writeMemory(0x400, &original, 4);
    mm.readMemory(0x400, &read, 4);
    REQUIRE(original == read);
    
    mm.store32(0x400, (uint32_t)0x1122334455667788);
    REQUIRE(mm.load32(0x400) == 0x55667788);
}

TEST_CASE("read_memory_128") {
    MainMemory mm;
    mm.mark(0, 0x1000, false, "");
    mm.setMemory(0x400, 0, 16);
    uint128_t i128 = 0;
    REQUIRE((mm.load128(0x400) == i128));
}

TEST_CASE("provide_memory") {
    MainMemory mm;
    mm.mark(0, DefaultMainMemoryPageSize * 10, false, "");
    mm.setMemory(DefaultMainMemoryPageSize * 5 + 0x300, 0, 1);
    mm.store32(DefaultMainMemoryPageSize * 5 + 0x300, 0x11223344);
    std::vector<uint8_t> vec(DefaultMainMemoryPageSize * 4);
    mm.provideMemory(DefaultMainMemoryPageSize * 2, DefaultMainMemoryPageSize * 4, vec.data());
    REQUIRE( mm.load32(DefaultMainMemoryPageSize * 5 + 0x300) == 0x11223344 );
    REQUIRE( *(uint32_t*)&vec[DefaultMainMemoryPageSize * 3 + 0x300] == 0x44332211 );
    uint8_t* buf = &vec[DefaultMainMemoryPageSize * 3 + 0x300];
    buf[0] = 0xaa;
    buf[1] = 0xbb;
    buf[2] = 0xcc;
    buf[3] = 0xdd;
    REQUIRE( mm.load32(DefaultMainMemoryPageSize * 5 + 0x300) == 0xaabbccdd );
}

TEST_CASE("internal alloc") {
    MainMemory mm;
    g_state.mm = &mm;
    InternalMemoryManager internal(EmuInternalArea, EmuInternalAreaSize, "");
    uint32_t ea;
    auto pair = internal.internalAlloc<128, std::pair<uint32_t, uint32_t>>(&ea, 10, 20);
    REQUIRE( pair->first == 10 );
    REQUIRE( pair->second == 20 );
    
    pair->first = 0xaaaaaaaa;
    pair->second = 0xbbbbbbbb;
    REQUIRE( mm.load32(ea) == 0xaaaaaaaa );
    REQUIRE( mm.load32(ea + sizeof(int)) == 0xbbbbbbbb );
}

TEST_CASE("internal_alloc_unique_ptr") {
    MainMemory mm;
    g_state.mm = &mm;
    InternalMemoryManager internal(EmuInternalArea, EmuInternalAreaSize, "");
    uint32_t ea;
    auto pair = internal.internalAllocU<128, std::pair<uint32_t, uint32_t>>(&ea, 10, 20);
    REQUIRE( pair->first == 10 );
    REQUIRE( pair->second == 20 );
    
    pair->first = 0xaaaaaaaa;
    pair->second = 0xbbbbbbbb;
    REQUIRE( mm.load32(ea) == 0xaaaaaaaa );
    REQUIRE( mm.load32(ea + sizeof(int)) == 0xbbbbbbbb );
}

TEST_CASE("fixed loads") {
/*
10474:       88 60 00 20     lbz     r3,32(0)
10478:       88 81 ff e0     lbz     r4,-32(r1)              # ffffffe0
1047c:       7c a0 08 ae     lbzx    r5,0,r1
10480:       7c c1 10 ae     lbzx    r6,r1,r2
10484:       8c e1 ff e0     lbzu    r7,-32(r1)              # ffffffe0
10488:       7d 01 10 ee     lbzux   r8,r1,r2
1048c:       a0 60 00 20     lhz     r3,32(0)
10490:       a0 81 ff e0     lhz     r4,-32(r1)              # ffffffe0
10494:       7c a0 0a 2e     lhzx    r5,0,r1
10498:       7c c1 12 2e     lhzx    r6,r1,r2
1049c:       a4 e1 ff e0     lhzu    r7,-32(r1)              # ffffffe0
104a0:       7d 01 12 6e     lhzux   r8,r1,r2
104a4:       a8 60 00 20     lha     r3,32(0)
104a8:       a8 81 ff e0     lha     r4,-32(r1)              # ffffffe0
104ac:       7c a0 0a ae     lhax    r5,0,r1
104b0:       7c c1 12 ae     lhax    r6,r1,r2
104b4:       ac e1 ff e0     lhau    r7,-32(r1)              # ffffffe0
104b8:       7d 01 12 ee     lhaux   r8,r1,r2
104bc:       80 60 00 20     lwz     r3,32(0)
104c0:       80 81 ff e0     lwz     r4,-32(r1)              # ffffffe0
104c4:       7c a0 08 2e     lwzx    r5,0,r1
104c8:       7c c1 10 2e     lwzx    r6,r1,r2
104cc:       84 e1 ff e0     lwzu    r7,-32(r1)              # ffffffe0
104d0:       7d 01 10 6e     lwzux   r8,r1,r2
104d4:       e8 60 00 22     lwa     r3,32(0)
104d8:       e8 81 ff e2     lwa     r4,-32(r1)              # ffffffe0
104dc:       7c a0 0a aa     lwax    r5,0,r1
104e0:       7c c1 12 aa     lwax    r6,r1,r2
104e4:       7c e1 12 ea     lwaux   r7,r1,r2
104e8:       e8 60 00 20     ld      r3,32(0)
104ec:       e8 81 ff e0     ld      r4,-32(r1)              # ffffffe0
104f0:       7c a0 08 2a     ldx     r5,0,r1
104f4:       7c c1 10 2a     ldx     r6,r1,r2
104f8:       e8 e1 ff e1     ldu     r7,-32(r1)              # ffffffe0
104fc:       7d 01 10 6a     ldux    r8,r1,r2
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    mm.mark(0x300000, 0x1000, false, "");
    mm.mark(0, 0x1000, false, "");
    mm.setMemory(0x300000, 0, 200);
    mm.setMemory(32, 0, 16);
    uint8_t instr[] = { 
          0x88, 0x60, 0x00, 0x20
        , 0x88, 0x81, 0xff, 0xe0
        , 0x7c, 0xa0, 0x08, 0xae
        , 0x7c, 0xc1, 0x10, 0xae
        , 0x8c, 0xe1, 0xff, 0xe0
        , 0x7d, 0x01, 0x10, 0xee
        , 0xa0, 0x60, 0x00, 0x20
        , 0xa0, 0x81, 0xff, 0xe0
        , 0x7c, 0xa0, 0x0a, 0x2e
        , 0x7c, 0xc1, 0x12, 0x2e
        , 0xa4, 0xe1, 0xff, 0xe0
        , 0x7d, 0x01, 0x12, 0x6e
        , 0xa8, 0x60, 0x00, 0x20
        , 0xa8, 0x81, 0xff, 0xe0
        , 0x7c, 0xa0, 0x0a, 0xae
        , 0x7c, 0xc1, 0x12, 0xae
        , 0xac, 0xe1, 0xff, 0xe0
        , 0x7d, 0x01, 0x12, 0xee
        , 0x80, 0x60, 0x00, 0x20
        , 0x80, 0x81, 0xff, 0xe0
        , 0x7c, 0xa0, 0x08, 0x2e
        , 0x7c, 0xc1, 0x10, 0x2e
        , 0x84, 0xe1, 0xff, 0xe0
        , 0x7d, 0x01, 0x10, 0x6e
        , 0xe8, 0x60, 0x00, 0x22
        , 0xe8, 0x81, 0xff, 0xe2
        , 0x7c, 0xa0, 0x0a, 0xaa
        , 0x7c, 0xc1, 0x12, 0xaa
        , 0x7c, 0xe1, 0x12, 0xea
        , 0xe8, 0x60, 0x00, 0x20
        , 0xe8, 0x81, 0xff, 0xe0
        , 0x7c, 0xa0, 0x08, 0x2a
        , 0x7c, 0xc1, 0x10, 0x2a
        , 0xe8, 0xe1, 0xff, 0xe1
        , 0x7d, 0x01, 0x10, 0x6a
    };
    auto i = 0u;
    auto next = [&] {
        th.setGPR(1, 0x300020);
        th.setGPR(2, -32);
        ppu_dasm<DasmMode::Emulate>(instr + i * 4, 0, &th);
        i++;
    };
    
    mm.store64(32,       0x55ff44ff332211ff);
    mm.store64(0x300000, 0xeeffddffccbbaaff);
    mm.store64(0x300020, 0xaaffbbffccddeeff);
    
    next();
    REQUIRE( th.getGPR(3) == 0x0000000000000055 );
    next();
    REQUIRE( th.getGPR(4) == 0x00000000000000ee );
    next();
    REQUIRE( th.getGPR(5) == 0x00000000000000aa );
    next();
    REQUIRE( th.getGPR(6) == 0x00000000000000ee );
    next();
    REQUIRE( th.getGPR(7) == 0x00000000000000ee );
    REQUIRE( th.getGPR(1) == 0x300000 );
    next();
    REQUIRE( th.getGPR(8) == 0x00000000000000ee );
    REQUIRE( th.getGPR(1) == 0x300000 );
    
    next();
    REQUIRE( th.getGPR(3) == 0x00000000000055ff );
    next();
    REQUIRE( th.getGPR(4) == 0x000000000000eeff );
    next();
    REQUIRE( th.getGPR(5) == 0x000000000000aaff );
    next();
    REQUIRE( th.getGPR(6) == 0x000000000000eeff );
    next();
    REQUIRE( th.getGPR(7) == 0x000000000000eeff );
    REQUIRE( th.getGPR(1) == 0x300000 );
    next();
    REQUIRE( th.getGPR(8) == 0x000000000000eeff );
    REQUIRE( th.getGPR(1) == 0x300000 );
    
    mm.store64(32,       0xff11223344556677);
    mm.store64(0x300000, 0xffaabbccddee9988);
    mm.store64(0x300020, 0xff00aa11bb22cc33);
    
    next();
    REQUIRE( th.getGPR(3) == 0xffffffffffffff11 );
    next();
    REQUIRE( th.getGPR(4) == 0xffffffffffffffaa );
    next();
    REQUIRE( th.getGPR(5) == 0xffffffffffffff00 );
    next();
    REQUIRE( th.getGPR(6) == 0xffffffffffffffaa );
    next();
    REQUIRE( th.getGPR(7) == 0xffffffffffffffaa );
    REQUIRE( th.getGPR(1) == 0x300000 );
    next();
    REQUIRE( th.getGPR(8) == 0xffffffffffffffaa );
    REQUIRE( th.getGPR(1) == 0x300000 );
    
    mm.store64(32,       0x55ff44ff332211ff);
    mm.store64(0x300000, 0xeeffddffccbbaaff);
    mm.store64(0x300020, 0xaaffbbffccddeeff);
    
    next();
    REQUIRE( th.getGPR(3) == 0x0000000055ff44ff );
    next();
    REQUIRE( th.getGPR(4) == 0x00000000eeffddff );
    next();
    REQUIRE( th.getGPR(5) == 0x00000000aaffbbff );
    next();
    REQUIRE( th.getGPR(6) == 0x00000000eeffddff );
    next();
    REQUIRE( th.getGPR(7) == 0x00000000eeffddff );
    REQUIRE( th.getGPR(1) == 0x300000 );
    next();
    REQUIRE( th.getGPR(8) == 0x00000000eeffddff );
    REQUIRE( th.getGPR(1) == 0x300000 );
    
    mm.store64(32,       0xff11223344556677);
    mm.store64(0x300000, 0xffaabbccddee9988);
    mm.store64(0x300020, 0xff00aa11bb22cc33);
    
    next();
    REQUIRE( th.getGPR(3) == 0xffffffffff112233 );
    next();
    REQUIRE( th.getGPR(4) == 0xffffffffffaabbcc );
    next();
    REQUIRE( th.getGPR(5) == 0xffffffffff00aa11 );
    next();
    REQUIRE( th.getGPR(6) == 0xffffffffffaabbcc );
    next();
    REQUIRE( th.getGPR(7) == 0xffffffffffaabbcc );
    REQUIRE( th.getGPR(1) == 0x300000 );
    
    mm.store64(32,       0x55ff44ff332211ff);
    mm.store64(0x300000, 0xeeffddffccbbaaff);
    mm.store64(0x300020, 0xaaffbbffccddeeff);
    
    next();
    REQUIRE( th.getGPR(3) == 0x55ff44ff332211ff );
    next();
    REQUIRE( th.getGPR(4) == 0xeeffddffccbbaaff );
    next();
    REQUIRE( th.getGPR(5) == 0xaaffbbffccddeeff );
    next();
    REQUIRE( th.getGPR(6) == 0xeeffddffccbbaaff );
    next();
    REQUIRE( th.getGPR(7) == 0xeeffddffccbbaaff );
    REQUIRE( th.getGPR(1) == 0x300000 );
    next();
    REQUIRE( th.getGPR(8) == 0xeeffddffccbbaaff );
    REQUIRE( th.getGPR(1) == 0x300000 );
}

TEST_CASE("fixed_stores") {
/*
1050c:       98 60 00 10     stb     r3,16(0)
10510:       98 61 ff f0     stb     r3,-16(r1)              # fffffff0
10514:       7c 60 09 ae     stbx    r3,0,r1
10518:       7c 61 11 ae     stbx    r3,r1,r2
1051c:       9c 61 ff f0     stbu    r3,-16(r1)              # fffffff0
10520:       7c 61 11 ee     stbux   r3,r1,r2
10524:       b0 60 00 10     sth     r3,16(0)
10528:       b0 61 ff f0     sth     r3,-16(r1)              # fffffff0
1052c:       7c 60 0b 2e     sthx    r3,0,r1
10530:       7c 61 13 2e     sthx    r3,r1,r2
10534:       b4 61 ff f0     sthu    r3,-16(r1)              # fffffff0
10538:       7c 61 13 6e     sthux   r3,r1,r2
1053c:       90 60 00 10     stw     r3,16(0)
10540:       90 61 ff f0     stw     r3,-16(r1)              # fffffff0
10544:       7c 60 09 2e     stwx    r3,0,r1
10548:       7c 61 11 2e     stwx    r3,r1,r2
1054c:       94 61 ff f0     stwu    r3,-16(r1)              # fffffff0
10550:       7c 61 11 6e     stwux   r3,r1,r2
10554:       f8 60 00 10     std     r3,16(0)
10558:       f8 61 ff f0     std     r3,-16(r1)              # fffffff0
1055c:       7c 60 09 2a     stdx    r3,0,r1
10560:       7c 61 11 2a     stdx    r3,r1,r2
10564:       f8 61 ff f1     stdu    r3,-16(r1)              # fffffff0
10568:       7c 61 11 6a     stdux   r3,r1,r2
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    mm.mark(0x300000, 0x1000, false, "");
    mm.mark(0, 0x1000, false, "");
    uint8_t instr[] = { 
          0x98, 0x60, 0x00, 0x10
        , 0x98, 0x61, 0xff, 0xf0
        , 0x7c, 0x60, 0x09, 0xae
        , 0x7c, 0x61, 0x11, 0xae
        , 0x9c, 0x61, 0xff, 0xf0
        , 0x7c, 0x61, 0x11, 0xee
        , 0xb0, 0x60, 0x00, 0x10
        , 0xb0, 0x61, 0xff, 0xf0
        , 0x7c, 0x60, 0x0b, 0x2e
        , 0x7c, 0x61, 0x13, 0x2e
        , 0xb4, 0x61, 0xff, 0xf0
        , 0x7c, 0x61, 0x13, 0x6e
        , 0x90, 0x60, 0x00, 0x10
        , 0x90, 0x61, 0xff, 0xf0
        , 0x7c, 0x60, 0x09, 0x2e
        , 0x7c, 0x61, 0x11, 0x2e
        , 0x94, 0x61, 0xff, 0xf0
        , 0x7c, 0x61, 0x11, 0x6e
        , 0xf8, 0x60, 0x00, 0x10
        , 0xf8, 0x61, 0xff, 0xf0
        , 0x7c, 0x60, 0x09, 0x2a
        , 0x7c, 0x61, 0x11, 0x2a
        , 0xf8, 0x61, 0xff, 0xf1
        , 0x7c, 0x61, 0x11, 0x6a
    };
    th.setGPR(3, 0x1122334455667788);
    auto i = 0u;
    auto next = [&] {
        mm.setMemory(0x300010, 0, 100);
        mm.setMemory(16, 0, 100);
        th.setGPR(1, 0x300010);
        th.setGPR(2, 0x40);
        ppu_dasm<DasmMode::Emulate>(instr + 4*i, 0, &th);
        i++;
    };
    
    next();
    REQUIRE( mm.load64(16) ==       0x8800000000000000 );
    next();
    REQUIRE( mm.load64(0x300000) == 0x8800000000000000 );
    next();
    REQUIRE( mm.load64(0x300010) == 0x8800000000000000 );
    next();
    REQUIRE( mm.load64(0x300050) == 0x8800000000000000 );
    next();
    REQUIRE( mm.load64(0x300000) == 0x8800000000000000 );
    REQUIRE( th.getGPR(1) == 0x300000 );
    next();
    REQUIRE( mm.load64(0x300050) == 0x8800000000000000 );
    REQUIRE( th.getGPR(1) == 0x300050 );
    
    next();
    REQUIRE( mm.load64(16) ==       0x7788000000000000 );
    next();
    REQUIRE( mm.load64(0x300000) == 0x7788000000000000 );
    next();
    REQUIRE( mm.load64(0x300010) == 0x7788000000000000 );
    next();
    REQUIRE( mm.load64(0x300050) == 0x7788000000000000 );
    next();
    REQUIRE( mm.load64(0x300000) == 0x7788000000000000 );
    REQUIRE( th.getGPR(1) == 0x300000 );
    next();
    REQUIRE( mm.load64(0x300050) == 0x7788000000000000 );
    REQUIRE( th.getGPR(1) == 0x300050 );
    
    next();
    REQUIRE( mm.load64(16) ==       0x5566778800000000 );
    next();
    REQUIRE( mm.load64(0x300000) == 0x5566778800000000 );
    next();
    REQUIRE( mm.load64(0x300010) == 0x5566778800000000 );
    next();
    REQUIRE( mm.load64(0x300050) == 0x5566778800000000 );
    next();
    REQUIRE( mm.load64(0x300000) == 0x5566778800000000 );
    REQUIRE( th.getGPR(1) == 0x300000 );
    next();
    REQUIRE( mm.load64(0x300050) == 0x5566778800000000 );
    REQUIRE( th.getGPR(1) == 0x300050 );
    
    next();
    REQUIRE( mm.load64(16) ==       0x1122334455667788 );
    next();
    REQUIRE( mm.load64(0x300000) == 0x1122334455667788 );
    next();
    REQUIRE( mm.load64(0x300010) == 0x1122334455667788 );
    next();
    REQUIRE( mm.load64(0x300050) == 0x1122334455667788 );
    next();
    REQUIRE( mm.load64(0x300000) == 0x1122334455667788 );
    REQUIRE( th.getGPR(1) == 0x300000 );
    next();
    REQUIRE( mm.load64(0x300050) == 0x1122334455667788 );
    REQUIRE( th.getGPR(1) == 0x300050 );
}

TEST_CASE("fixed load store with reversal") {
/*
10570:       7c 60 0e 2c     lhbrx   r3,0,r1
10574:       7c 81 16 2c     lhbrx   r4,r1,r2
10578:       7c a0 0c 2c     lwbrx   r5,0,r1
1057c:       7c c1 14 2c     lwbrx   r6,r1,r2
10580:       7c e0 0f 2c     sthbrx  r7,0,r1
10584:       7c e1 17 2c     sthbrx  r7,r1,r2
10588:       7c e0 0d 2c     stwbrx  r7,0,r1
1058c:       7c e1 15 2c     stwbrx  r7,r1,r2
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    mm.mark(0x300000, 0x1000, false, "");
    uint8_t instr[] = {
          0x7c, 0x60, 0x0e, 0x2c
        , 0x7c, 0x81, 0x16, 0x2c
        , 0x7c, 0xa0, 0x0c, 0x2c
        , 0x7c, 0xc1, 0x14, 0x2c
        , 0x7c, 0xe0, 0x0f, 0x2c
        , 0x7c, 0xe1, 0x17, 0x2c
        , 0x7c, 0xe0, 0x0d, 0x2c
        , 0x7c, 0xe1, 0x15, 0x2c
    };
    
    mm.setMemory(0x300010, 0, 100);
    th.setGPR(1, 0x300010);
    th.setGPR(2, 0x40);
    mm.store64(0x300010, 0x1122334455667788);
    mm.store64(0x300050, 0xaabbccddeeff0099);
    
    ppu_dasm<DasmMode::Emulate>(instr + 0*4, 0, &th);
    REQUIRE( th.getGPR(3) == 0x2211 );
    ppu_dasm<DasmMode::Emulate>(instr + 1*4, 0, &th);
    REQUIRE( th.getGPR(4) == 0xbbaa );
    ppu_dasm<DasmMode::Emulate>(instr + 2*4, 0, &th);
    REQUIRE( th.getGPR(5) == 0x44332211 );
    ppu_dasm<DasmMode::Emulate>(instr + 3*4, 0, &th);
    REQUIRE( th.getGPR(6) == 0xddccbbaa );
    
    th.setGPR(7, 0x1122334455667788);
    
    mm.setMemory(0x300010, 0, 100);
    ppu_dasm<DasmMode::Emulate>(instr + 4*4, 0, &th);
    REQUIRE( mm.load64(0x300010) == 0x2211000000000000 );
    mm.setMemory(0x300010, 0, 100);
    ppu_dasm<DasmMode::Emulate>(instr + 5*4, 0, &th);
    REQUIRE( mm.load64(0x300050) == 0x2211000000000000 );
    mm.setMemory(0x300010, 0, 100);
    ppu_dasm<DasmMode::Emulate>(instr + 6*4, 0, &th);
    REQUIRE( mm.load64(0x300010) == 0x4433221100000000 );
    mm.setMemory(0x300010, 0, 100);
    ppu_dasm<DasmMode::Emulate>(instr + 7*4, 0, &th);
    REQUIRE( mm.load64(0x300050) == 0x4433221100000000 );
}

TEST_CASE("fixed arithmetic") {
/*
10594:       38 40 00 10     li      r2,16
10598:       38 61 00 10     addi    r3,r1,16
1059c:       38 80 ff f0     li      r4,-16          # fffffff0
105a0:       38 a1 ff f0     addi    r5,r1,-16               # fffffff0
105a4:       3c c0 00 10     lis     r6,16
105a8:       3c e1 00 10     addis   r7,r1,16
105ac:       3d 00 ff f0     lis     r8,-16          # fffffff0
105b0:       3d 21 ff f0     addis   r9,r1,-16               # fffffff0
105b4:       7d 40 0a 14     add     r10,r0,r1
105b8:       7d 61 00 50     subf    r11,r1,r0
105bc:       7d 81 04 50     subfo   r12,r1,r0
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    mm.mark(0, 0x1000, false, "");
    uint8_t instr[] = {
          0x38, 0x40, 0x00, 0x10
        , 0x38, 0x61, 0x00, 0x10
        , 0x38, 0x80, 0xff, 0xf0
        , 0x38, 0xa1, 0xff, 0xf0
        , 0x3c, 0xc0, 0x00, 0x10
        , 0x3c, 0xe1, 0x00, 0x10
        , 0x3d, 0x00, 0xff, 0xf0
        , 0x3d, 0x21, 0xff, 0xf0
        , 0x7d, 0x40, 0x0a, 0x14
        , 0x7d, 0x61, 0x00, 0x50
        , 0x7d, 0x81, 0x04, 0x50
    };
    th.setGPR(0, 0x400);
    th.setGPR(1, 0x100);
    for (auto i = 0u; i < sizeof(instr); i += 4) {
        ppu_dasm<DasmMode::Emulate>(instr + i, 0, &th);
    }
    REQUIRE( th.getGPR(2) == 16 );
    REQUIRE( th.getGPR(3) == 0x110 );
    REQUIRE( th.getGPR(4) == -16ul );
    REQUIRE( th.getGPR(5) == 0xf0 );
    REQUIRE( th.getGPR(6) == 0x100000 );
    REQUIRE( th.getGPR(7) == 0x100100 );
    REQUIRE( th.getGPR(8) == 0xfffffffffff00000 );
    REQUIRE( th.getGPR(9) == -1048320ul );
    REQUIRE( th.getGPR(10) == 0x500 );
    REQUIRE( th.getGPR(11) == 0x300 );
    REQUIRE( th.getGPR(12) == 0x300 );
}

TEST_CASE("list1") {
/*
10350:       38 40 00 0c     li      r2,12
10354:       38 60 00 07     li      r3,7
10358:       38 62 00 0a     addi    r3,r2,10
1035c:       7c 83 12 14     add     r4,r3,r2
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint64_t cia = 0x10350;
    uint8_t instr[] = {
        0x38, 0x40, 0x00, 0x0c,
        0x38, 0x60, 0x00, 0x07,
        0x38, 0x62, 0x00, 0x0a,
        0x7c, 0x83, 0x12, 0x14
    };
    for (auto i = 0u; i < sizeof(instr); i += 4) {
        ppu_dasm<DasmMode::Emulate>(instr + i, cia + i, &th);
    }
    REQUIRE(th.getGPR(2) == 12);
    REQUIRE(th.getGPR(3) == 22);
    REQUIRE(th.getGPR(4) == 34);
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
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;;
    auto base = 0x10350;
    th.setNIP(base);
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
        auto nip = th.getNIP();
        th.setNIP(nip + 4);
        ppu_dasm<DasmMode::Emulate>(instr + nip - base, nip, &th);
    }
    REQUIRE(th.getNIP() == 0x10378);
    REQUIRE(th.getGPR(3) == (55 >> 2) + (29 << 5));
}

TEST_CASE("emu stw") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    mm.mark(0x400000, 0x1000, false, "");
    th.setGPR(29, 0x1122334455667788);
    th.setGPR(11, 0x400000);
    mm.setMemory(0x400000, 0, 8);
    uint8_t instr[] = { 0x93, 0xab, 0x00, 0x00 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE(mm.load32(0x400000) == 0x55667788); 
}

TEST_CASE("emu mtctr") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setGPR(0, 0x11223344);
    uint8_t instr[] = { 0x7c, 0x09, 0x03, 0xa6 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE(th.getCTR() == 0x11223344); 
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
    REQUIRE(bit_test(0x48000084, 32, 31) == 0);
    REQUIRE(bit_test(0x48000084, 32, 30) == 0);
    REQUIRE(bit_test(0x48000084, 32, 29) == 1);
    REQUIRE(bit_test(0x48000084, 32, 28) == 0);
    REQUIRE(bit_set(0x48000084, 30, 1) == 0x48000086);
    REQUIRE(bit_set(0x48000086, 30, 0) == 0x48000084);
}

TEST_CASE("emu cmpld") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setCR(0);
    th.setGPR(30, 10);
    th.setGPR(8, 10);
    // cmpld cr7,r30,r8
    uint8_t instr[] = { 0x7f, 0xbe, 0x40, 0x40 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE((th.getCR() & 0xf) == 2);
    th.setGPR(30, 20);
    th.setGPR(8, 10);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE((th.getCR() & 0xf) == 4);
    th.setGPR(30, 20);
    th.setGPR(8, 30);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE((th.getCR() & 0xf) == 8);
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
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    auto base = 0x10350;
    auto mem = 0x400000;
    th.setNIP(base);
    mm.mark(mem, 0x1000, false, "");
    mm.setMemory(mem, 0, 4);
    th.setGPR(7, mem);
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
        if (i == 50 || th.getNIP() == 0x10370)
            break;
        auto nip = th.getNIP();
        th.setNIP(nip + 4);
        ppu_dasm<DasmMode::Emulate>(instr + nip - base, nip, &th);
    }
    REQUIRE(i < 50);
    REQUIRE(th.getNIP() == 0x10370);
    REQUIRE(mm.load32(mem) == 0x05050505);
}

TEST_CASE("set crf") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setCR(0);
    th.setCRF_sign(7, 4);
    REQUIRE(th.getCR() == 8);
    th.setCR(0xf);
    th.setCRF_sign(7, 4);
    REQUIRE(th.getCR() == 9);
}

TEST_CASE("branch_info_bge") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setGPR(10, ~0ull);
    // bge cr7,103b4
    uint32_t instr = 0x409cffe0;
    REQUIRE(isAbsoluteBranch(instr));
    REQUIRE(getTargetAddress(instr, 0x103d4) == 0x103b4);
    th.setCRF_sign(7, 2);
    REQUIRE(isTaken(instr, 0x103d4, &th));
    th.setCRF_sign(7, 4);
    REQUIRE(!isTaken(instr, 0x103d4, &th));
}

TEST_CASE("neg") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setGPR(3, ~0ull);
    // neg r0,r3
    uint8_t instr[] = { 0x7c, 0x03, 0x00, 0xd0 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 1 );
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
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    auto base = 0x17ff4;
    th.setNIP(base);
    th.setLR(0);
    const char* str = "hello there";
    mm.mark(0x400000, 0x1000, false, "");
    mm.writeMemory(0x400000, str, strlen(str) + 1);
    th.setGPR(3, 0x400000);
    for (;;) {
        if (th.getNIP() == 0)
            break;
        auto nip = th.getNIP();
        th.setNIP(nip + 4);
        ppu_dasm<DasmMode::Emulate>(instr + nip - base, nip, &th);
    }
    REQUIRE( th.getGPR(3) == strlen(str) );
}

TEST_CASE("clrldi r0,r0,61") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setGPR(0, ~0ull);
    uint8_t instr[] = { 0x78, 0x00, 0x07, 0x60 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 7 );
}

TEST_CASE("cntlzd r0,r11") {
    uint8_t instr[] = { 0x7d, 0x60, 0x00, 0x74 };
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setGPR(0, 100);
    th.setGPR(11, 0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 64 );
    th.setGPR(11, 1);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 63 );
    th.setGPR(11, 0xffffffff);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 32 );
}

TEST_CASE("nor r11,r9,r10") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setGPR(11, 500);
    th.setGPR(9, 0x7f7f7f7f7f7f7f7full);
    th.setGPR(10, 0xf2eeece49feef4f3ull);
    uint8_t instr[] = { 0x7d, 0x2b, 0x50, 0xf8 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(11) == 0 );
}

TEST_CASE("subf r3,r3,r4") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setGPR(3, 500);
    th.setGPR(4, 700);
    uint8_t instr[] = { 0x7c, 0x63, 0x20, 0x50 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(3) == 200 );
}

TEST_CASE("lwz r11,0(r10)") {
    MainMemory mm;
    g_state.mm = &mm;
    mm.mark(0x400000, 0x1000, false, "");
    PPUThread th;
    uint32_t i = 0x66778899;
    mm.writeMemory(0x400000, &i, 4);
    th.setGPR(10, 0x400000);
    uint8_t instr[] = { 0x81, 0x6a, 0x00, 0x00 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(11) == 0x99887766 );
}

TEST_CASE("lis r9,22616") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 0x3d, 0x20, 0x58, 0x58 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(9) == 0x58580000 );
}

TEST_CASE("sradi_1") {
    // sradi r0,r1,3
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 0x7c, 0x20, 0x1e, 0x74 };

    th.setGPR(1, 27);
    th.setCA(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 3 );
    REQUIRE( th.getCA() == 0);
    
    th.setGPR(1, 0xffffffffffffff1b);
    th.setCA(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 0xffffffffffffffe3 );
    REQUIRE( th.getCA() == 1);
    
    th.setGPR(1, 0xfffffffffffffff8);
    th.setCA(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 0xffffffffffffffff );
    REQUIRE( th.getCA() == 0);
}

TEST_CASE("sradi_2") {
    // sradi r0,r1,0
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 0x7c, 0x20, 0x06, 0x74 };
    
    th.setGPR(1, 0xffffffffffffffff);
    th.setCA(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 0xffffffffffffffff );
    REQUIRE( th.getCA() == 0);
    
    th.setGPR(1, 10);
    th.setCA(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 10 );
    REQUIRE( th.getCA() == 0);
}

TEST_CASE("addic r0,r1,2") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 0x30, 0x01, 0x00, 0x02 };
    th.setGPR(1, 10);
    th.setCA(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 12 );
    REQUIRE( th.getCA() == 0 );
    
    th.setGPR(1, ~0ull);
    th.setCA(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 1 );
    REQUIRE( th.getCA() == 1 );
}

TEST_CASE("addic. r0,r1,2") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 0x34, 0x01, 0x00, 0x02 };
    th.setGPR(1, 10);
    th.setCA(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 12 );
    REQUIRE( th.getCA() == 0 );
    REQUIRE( th.getCRF_sign(0) == 2 );
    
    th.setGPR(1, ~0ull);
    th.setCA(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 1 );
    REQUIRE( th.getCA() == 1 );
    REQUIRE( th.getCRF_sign(0) == 2 );
}

TEST_CASE("subfic r0,r1,2") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 0x20, 0x01, 0x00, 0x02 };
    th.setGPR(1, 1);
    th.setCA(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 1 );
    REQUIRE( th.getCA() == 0 );
    
    th.setGPR(1, 4);
    th.setCA(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 0xfffffffffffffffe );
    REQUIRE( th.getCA() == 0 );
}


TEST_CASE("memcpy") {
    uint8_t instr[] = { 
        0x2b, 0xa5, 0x00, 0x07, 0x78, 0x63, 0x00, 0x20, 0x78, 0x84, 0x00, 0x20, 0x7c, 0x6b, 
        0x1b, 0x78, 0x41, 0x9d, 0x00, 0x70, 0x2c, 0x25, 0x00, 0x00, 0x4d, 0x82, 0x00, 0x20, 
        0x28, 0xa5, 0x00, 0x0f, 0x40, 0x85, 0x00, 0x24, 0x78, 0xac, 0xe8, 0xc2, 0x7d, 0x89, 
        0x03, 0xa6, 0xe8, 0xe4, 0x00, 0x00, 0x38, 0xa5, 0xff, 0xf8, 0x38, 0x84, 0x00, 0x08, 
        0xf8, 0xeb, 0x00, 0x00, 0x39, 0x6b, 0x00, 0x08, 0x42, 0x00, 0xff, 0xec, 0x2c, 0x25, 
        0x00, 0x00, 0x4d, 0x82, 0x00, 0x20, 0x7c, 0xa9, 0x03, 0xa6, 0x88, 0x04, 0x00, 0x00, 
        0x38, 0x84, 0x00, 0x01, 0x98, 0x0b, 0x00, 0x00, 0x39, 0x6b, 0x00, 0x01, 0x42, 0x00, 
        0xff, 0xf0, 0x4e, 0x80, 0x00, 0x20, 0x7c, 0xe9, 0x03, 0xa6, 0x8d, 0x04, 0xff, 0xff, 
        0x38, 0xa5, 0xff, 0xff, 0x9d, 0x0b, 0xff, 0xff, 0x42, 0x00, 0xff, 0xf4, 0x48, 0x00, 
        0x00, 0x34, 0x7c, 0x00, 0x22, 0x2c, 0x78, 0x86, 0x05, 0x20, 0x78, 0x69, 0x05, 0x20, 
        0x7c, 0xa6, 0x48, 0x40, 0x40, 0x84, 0x01, 0x4c, 0x7c, 0x84, 0x2a, 0x14, 0x7d, 0x63, 
        0x2a, 0x14, 0x39, 0x44, 0xff, 0x80, 0x7c, 0x00, 0x52, 0x2c, 0x78, 0x87, 0x07, 0x60, 
        0x2f, 0x27, 0x00, 0x00, 0x40, 0x9a, 0xff, 0xbc, 0x2c, 0xa5, 0x00, 0x00, 0x4d, 0x86, 
        0x00, 0x20, 0x79, 0x60, 0x07, 0xa0, 0x2f, 0x20, 0x00, 0x01, 0x41, 0x9a, 0x03, 0x34, 
        0x2c, 0xa0, 0x00, 0x02, 0x41, 0x86, 0x03, 0xf4, 0x2c, 0x20, 0x00, 0x03, 0x41, 0x82, 
        0x02, 0x54, 0x79, 0x6c, 0x07, 0x60, 0x2f, 0xac, 0x00, 0x00, 0x40, 0x9e, 0x04, 0x40, 
        0x28, 0x25, 0x00, 0x2f, 0x40, 0x81, 0x00, 0x94, 0x38, 0x84, 0xff, 0xd0, 0x38, 0xa5, 
        0xff, 0xd0, 0x39, 0x6b, 0xff, 0xd0, 0x28, 0xa5, 0x00, 0x2f, 0xe8, 0x04, 0x00, 0x28, 
        0xe9, 0x24, 0x00, 0x20, 0xe9, 0x84, 0x00, 0x18, 0xe9, 0x44, 0x00, 0x10, 0xe9, 0x04, 
        0x00, 0x08, 0xe8, 0xe4, 0x00, 0x00, 0xf8, 0x0b, 0x00, 0x28, 0xf9, 0x2b, 0x00, 0x20, 
        0xf9, 0x8b, 0x00, 0x18, 0xf9, 0x4b, 0x00, 0x10, 0xf9, 0x0b, 0x00, 0x08, 0xf8, 0xeb, 
        0x00, 0x00, 0x40, 0x85, 0x00, 0x50, 0x38, 0x84, 0xff, 0xd0, 0x39, 0x6b, 0xff, 0xd0, 
        0x38, 0xa5, 0xff, 0xd0, 0x38, 0xc4, 0xff, 0x00, 0xe8, 0x04, 0x00, 0x28, 0xe9, 0x24, 
        0x00, 0x20, 0xe9, 0x84, 0x00, 0x18, 0xe9, 0x44, 0x00, 0x10, 0xe9, 0x04, 0x00, 0x08, 
        0xe8, 0xe4, 0x00, 0x00, 0xf8, 0x0b, 0x00, 0x28, 0xf9, 0x2b, 0x00, 0x20, 0xf9, 0x8b, 
        0x00, 0x18, 0xf9, 0x4b, 0x00, 0x10, 0xf9, 0x0b, 0x00, 0x08, 0xf8, 0xeb, 0x00, 0x00, 
        0x7c, 0x00, 0x32, 0x2c, 0x28, 0x25, 0x00, 0x2f, 0x41, 0x81, 0xff, 0x74, 0x2b, 0x25, 
        0x00, 0x07, 0x40, 0x99, 0x00, 0x1c, 0x78, 0xa6, 0xe8, 0xc2, 0x7c, 0xc9, 0x03, 0xa6, 
        0xe8, 0xe4, 0xff, 0xf9, 0x38, 0xa5, 0xff, 0xf8, 0xf8, 0xeb, 0xff, 0xf9, 0x42, 0x00, 
        0xff, 0xf4, 0x2f, 0xa5, 0x00, 0x00, 0x7c, 0xa9, 0x03, 0xa6, 0x4d, 0x9e, 0x00, 0x20, 
        0x8c, 0xc4, 0xff, 0xff, 0x9c, 0xcb, 0xff, 0xff, 0x42, 0x00, 0xff, 0xf8, 0x4e, 0x80, 
        0x00, 0x20, 0x38, 0xc0, 0x00, 0x08, 0x7c, 0xe7, 0x30, 0x50, 0x7c, 0xe9, 0x03, 0xa6, 
        0x88, 0xc4, 0x00, 0x00, 0x38, 0xa5, 0xff, 0xff, 0x38, 0x84, 0x00, 0x01, 0x98, 0xcb, 
        0x00, 0x00, 0x39, 0x6b, 0x00, 0x01, 0x42, 0x00, 0xff, 0xec, 0x48, 0x00, 0x00, 0x18, 
        0x39, 0x44, 0x00, 0x80, 0x7c, 0x00, 0x52, 0x2c, 0x78, 0x87, 0x07, 0x60, 0x2c, 0xa7, 
        0x00, 0x00, 0x40, 0x86, 0xff, 0xc8, 0x2c, 0x25, 0x00, 0x00, 0x4d, 0x82, 0x00, 0x20, 
        0x79, 0x60, 0x07, 0xa0, 0x2c, 0xa0, 0x00, 0x01, 0x41, 0x86, 0x01, 0x8c, 0x2c, 0x20, 
        0x00, 0x02, 0x41, 0x82, 0x02, 0x54, 0x2f, 0xa0, 0x00, 0x03, 0x41, 0x9e, 0x00, 0xac, 
        0x79, 0x66, 0x07, 0x60, 0x2f, 0x26, 0x00, 0x00, 0x40, 0x9a, 0x03, 0x54, 0x2b, 0xa5, 
        0x01, 0x00, 0x40, 0x9d, 0xfd, 0xf8, 0x38, 0xa5, 0xff, 0xd0, 0xe8, 0xe4, 0x00, 0x00, 
        0xe8, 0x04, 0x00, 0x08, 0x28, 0x25, 0x00, 0x2f, 0xe9, 0x24, 0x00, 0x10, 0xe8, 0xc4, 
        0x00, 0x18, 0xe9, 0x44, 0x00, 0x20, 0xe9, 0x04, 0x00, 0x28, 0x38, 0x84, 0x00, 0x30, 
        0xf8, 0xeb, 0x00, 0x00, 0xf8, 0x0b, 0x00, 0x08, 0xf9, 0x2b, 0x00, 0x10, 0xf8, 0xcb, 
        0x00, 0x18, 0xf9, 0x4b, 0x00, 0x20, 0xf9, 0x0b, 0x00, 0x28, 0x39, 0x6b, 0x00, 0x30, 
        0x40, 0x81, 0xfd, 0xb4, 0xe8, 0x04, 0x00, 0x00, 0x38, 0xa5, 0xff, 0xd0, 0xe9, 0x24, 
        0x00, 0x08, 0xe8, 0xc4, 0x00, 0x10, 0xe9, 0x44, 0x00, 0x18, 0xe9, 0x04, 0x00, 0x20, 
        0xe8, 0xe4, 0x00, 0x28, 0x38, 0x84, 0x00, 0x30, 0xf8, 0x0b, 0x00, 0x00, 0xf9, 0x2b, 
        0x00, 0x08, 0x39, 0x84, 0x01, 0x00, 0xf8, 0xcb, 0x00, 0x10, 0xf9, 0x4b, 0x00, 0x18, 
        0xf9, 0x0b, 0x00, 0x20, 0xf8, 0xeb, 0x00, 0x28, 0x39, 0x6b, 0x00, 0x30, 0x7c, 0x00, 
        0x62, 0x2c, 0x2b, 0xa5, 0x00, 0x2f, 0x41, 0x9d, 0xff, 0x74, 0x4b, 0xff, 0xfd, 0x64, 
        0x28, 0x25, 0x00, 0x0f, 0x40, 0x81, 0xfd, 0x84, 0x78, 0xa7, 0xe1, 0x02, 0x7c, 0xe9, 
        0x03, 0xa6, 0xe8, 0xe4, 0x00, 0x00, 0x39, 0x04, 0x00, 0x80, 0x38, 0xa5, 0xff, 0xf0, 
        0xe9, 0x84, 0x00, 0x08, 0x38, 0x84, 0x00, 0x10, 0x78, 0xea, 0x45, 0xe4, 0x79, 0x86, 
        0x46, 0x20, 0x7c, 0x00, 0x42, 0x2c, 0x99, 0x8b, 0x00, 0x0f, 0x78, 0xe8, 0x46, 0x20, 
        0x7c, 0xc9, 0x53, 0x78, 0x78, 0xe0, 0x46, 0x02, 0x99, 0x0b, 0x00, 0x00, 0x79, 0x8a, 
        0x46, 0x02, 0x91, 0x2b, 0x00, 0x05, 0x79, 0x86, 0xc2, 0x02, 0x90, 0x0b, 0x00, 0x01, 
        0x91, 0x4b, 0x00, 0x09, 0xb0, 0xcb, 0x00, 0x0d, 0x39, 0x6b, 0x00, 0x10, 0x42, 0x00, 
        0xff, 0xb0, 0x4b, 0xff, 0xfd, 0x24, 0x28, 0xa5, 0x00, 0x0f, 0x40, 0x85, 0xfe, 0x70, 
        0x78, 0xa6, 0xe1, 0x02, 0x7c, 0xc9, 0x03, 0xa6, 0x38, 0x84, 0xff, 0xf0, 0x39, 0x6b, 
        0xff, 0xf0, 0x38, 0xa5, 0xff, 0xf0, 0xe9, 0x44, 0x00, 0x08, 0xe9, 0x84, 0x00, 0x00, 
        0x39, 0x04, 0xff, 0x80, 0x79, 0x40, 0x46, 0x20, 0x99, 0x4b, 0x00, 0x0f, 0x79, 0x89, 
        0x45, 0xe4, 0x79, 0x87, 0x46, 0x20, 0x7c, 0x00, 0x42, 0x2c, 0x7c, 0x06, 0x4b, 0x78, 
        0x79, 0x48, 0xc2, 0x02, 0x98, 0xeb, 0x00, 0x00, 0x79, 0x49, 0x46, 0x02, 0x90, 0xcb, 
        0x00, 0x05, 0x79, 0x80, 0x46, 0x02, 0xb1, 0x0b, 0x00, 0x0d, 0x91, 0x2b, 0x00, 0x09, 
        0x90, 0x0b, 0x00, 0x01, 0x42, 0x00, 0xff, 0xb0, 0x4b, 0xff, 0xfe, 0x10, 0x2b, 0x25, 
        0x00, 0x0f, 0x40, 0x99, 0xfc, 0xb4, 0x78, 0xa8, 0xe1, 0x02, 0x7d, 0x09, 0x03, 0xa6, 
        0xe9, 0x84, 0x00, 0x00, 0x39, 0x04, 0x00, 0x80, 0x38, 0xa5, 0xff, 0xf0, 0xe9, 0x44, 
        0x00, 0x08, 0x38, 0x84, 0x00, 0x10, 0x79, 0x86, 0xc1, 0xe4, 0x79, 0x49, 0xc2, 0x20, 
        0x7c, 0x00, 0x42, 0x2c, 0x99, 0x4b, 0x00, 0x0f, 0x79, 0x87, 0x46, 0x20, 0x7d, 0x29, 
        0x33, 0x78, 0x79, 0x88, 0xc2, 0x20, 0x98, 0xeb, 0x00, 0x00, 0x79, 0x80, 0xc2, 0x02, 
        0x91, 0x2b, 0x00, 0x07, 0x79, 0x46, 0xc2, 0x02, 0xb1, 0x0b, 0x00, 0x01, 0x90, 0x0b, 
        0x00, 0x03, 0x90, 0xcb, 0x00, 0x0b, 0x39, 0x6b, 0x00, 0x10, 0x42, 0x00, 0xff, 0xb0, 
        0x4b, 0xff, 0xfc, 0x54, 0x2b, 0xa5, 0x00, 0x0f, 0x40, 0x9d, 0xfd, 0xa0, 0x78, 0xa6, 
        0xe1, 0x02, 0x7c, 0xc9, 0x03, 0xa6, 0x38, 0x84, 0xff, 0xf0, 0x39, 0x6b, 0xff, 0xf0, 
        0x38, 0xa5, 0xff, 0xf0, 0xe8, 0xc4, 0x00, 0x00, 0xe9, 0x04, 0x00, 0x08, 0x39, 0x44, 
        0xff, 0x80, 0x78, 0xcc, 0xc1, 0xe4, 0x79, 0x00, 0xc2, 0x20, 0x99, 0x0b, 0x00, 0x0f, 
        0x78, 0xc7, 0x46, 0x20, 0x7c, 0x00, 0x52, 0x2c, 0x7c, 0x09, 0x63, 0x78, 0x79, 0x0a, 
        0xc2, 0x02, 0x98, 0xeb, 0x00, 0x00, 0x78, 0xcc, 0xc2, 0x02, 0x91, 0x2b, 0x00, 0x07, 
        0x78, 0xc0, 0xc2, 0x20, 0x91, 0x4b, 0x00, 0x0b, 0x91, 0x8b, 0x00, 0x03, 0xb0, 0x0b, 
        0x00, 0x01, 0x42, 0x00, 0xff, 0xb0, 0x4b, 0xff, 0xfd, 0x40, 0x28, 0xa5, 0x00, 0x0f, 
        0x40, 0x85, 0xfb, 0xe4, 0x78, 0xaa, 0xe1, 0x02, 0x7d, 0x49, 0x03, 0xa6, 0xe9, 0x84, 
        0x00, 0x00, 0x39, 0x04, 0x00, 0x80, 0x38, 0xa5, 0xff, 0xf0, 0xe8, 0xc4, 0x00, 0x08, 
        0x38, 0x84, 0x00, 0x10, 0x79, 0x8a, 0x83, 0xe4, 0x78, 0xc7, 0x84, 0x20, 0x7c, 0x00, 
        0x42, 0x2c, 0xb0, 0xcb, 0x00, 0x0e, 0x79, 0x88, 0x84, 0x20, 0x7c, 0xe9, 0x53, 0x78, 
        0x79, 0x80, 0x84, 0x02, 0xb1, 0x0b, 0x00, 0x00, 0x78, 0xc7, 0x84, 0x02, 0x91, 0x2b, 
        0x00, 0x06, 0x90, 0x0b, 0x00, 0x02, 0x90, 0xeb, 0x00, 0x0a, 0x39, 0x6b, 0x00, 0x10, 
        0x42, 0x00, 0xff, 0xb8, 0x4b, 0xff, 0xfb, 0x8c, 0x2b, 0x25, 0x00, 0x0f, 0x40, 0x99,
        0xfc, 0xd8, 0x78, 0xa8, 0xe1, 0x02, 0x7d, 0x09, 0x03, 0xa6, 0x38, 0x84, 0xff, 0xf0, 
        0x39, 0x6b, 0xff, 0xf0, 0x38, 0xa5, 0xff, 0xf0, 0xe9, 0x84, 0x00, 0x08, 0xe9, 0x04, 
        0x00, 0x00, 0x39, 0x44, 0xff, 0x80, 0x79, 0x80, 0x84, 0x20, 0xb1, 0x8b, 0x00, 0x0e, 
        0x79, 0x06, 0x83, 0xe4, 0x79, 0x07, 0x84, 0x20, 0x7c, 0x00, 0x52, 0x2c, 0x7c, 0x09, 
        0x33, 0x78, 0x79, 0x8a, 0x84, 0x02, 0xb0, 0xeb, 0x00, 0x00, 0x79, 0x00, 0x84, 0x02, 
        0x91, 0x2b, 0x00, 0x06, 0x91, 0x4b, 0x00, 0x0a, 0x90, 0x0b, 0x00, 0x02, 0x42, 0x00, 
        0xff, 0xb8, 0x4b, 0xff, 0xfc, 0x80, 0x2b, 0xa5, 0x00, 0x17, 0x40, 0x9d, 0xfc, 0x78, 
        0x38, 0x84, 0xff, 0xe8, 0x38, 0xa5, 0xff, 0xe8, 0x39, 0x6b, 0xff, 0xe8, 0x2b, 0xa5, 
        0x00, 0x17, 0xe9, 0x44, 0x00, 0x10, 0x39, 0x84, 0xff, 0x00, 0xe9, 0x04, 0x00, 0x08, 
        0xe8, 0xc4, 0x00, 0x00, 0x79, 0x40, 0x00, 0x22, 0x79, 0x09, 0x00, 0x22, 0x7c, 0x00, 
        0x62, 0x2c, 0x91, 0x4b, 0x00, 0x14, 0x78, 0xcc, 0x00, 0x22, 0x90, 0x0b, 0x00, 0x10, 
        0x91, 0x2b, 0x00, 0x08, 0x91, 0x8b, 0x00, 0x00, 0x91, 0x0b, 0x00, 0x0c, 0x90, 0xcb, 
        0x00, 0x04, 0x4b, 0xff, 0xff, 0xb4, 0x2b, 0xa5, 0x00, 0x17, 0x40, 0x9d, 0xfa, 0xd0, 
        0xe9, 0x44, 0x00, 0x00, 0x38, 0xc4, 0x01, 0x00, 0x38, 0xa5, 0xff, 0xe8, 0xe9, 0x04, 
        0x00, 0x08, 0xe9, 0x84, 0x00, 0x10, 0x79, 0x40, 0x00, 0x22, 0x79, 0x09, 0x00, 0x22, 
        0x7c, 0x00, 0x32, 0x2c, 0x91, 0x4b, 0x00, 0x04, 0x79, 0x86, 0x00, 0x22, 0x90, 0x0b, 
        0x00, 0x00, 0x2b, 0xa5, 0x00, 0x17, 0x91, 0x2b, 0x00, 0x08, 0x91, 0x0b, 0x00, 0x0c, 
        0x38, 0x84, 0x00, 0x18, 0x90, 0xcb, 0x00, 0x10, 0x91, 0x8b, 0x00, 0x14, 0x39, 0x6b, 
        0x00, 0x18, 0x4b, 0xff, 0xff, 0xb4 };
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    auto base = 0x17a30;
    uint64_t src = 0x400000;
    uint64_t dest = 0x600000;
    mm.mark(src, 0x1000, false, "");
    mm.mark(dest, 0x1000, false, "");
    th.setNIP(base);
    th.setLR(0);
    const char* str = "hello there";
    mm.writeMemory(src, str, strlen(str) + 1);
    mm.setMemory(dest, 0, 100);
    th.setGPR(3, dest);
    th.setGPR(4, src);
    th.setGPR(5, strlen(str) + 1);
    for (;;) {
        if (th.getNIP() == 0)
            break;
        auto nip = th.getNIP();
        th.setNIP(nip + 4);
        ppu_dasm<DasmMode::Emulate>(instr + nip - base, nip, &th);
    }
    char buf[100];
    mm.readMemory(dest, buf, sizeof buf);
    REQUIRE( std::string(buf) == "hello there" );
}

TEST_CASE("lwz r27,112(r1)") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    auto mem = 0x400000;
    mm.mark(mem, 0x1000, false, "");
    mm.setMemory(mem, 0, 8);
    mm.store64(mem, 0x11223344aabbccdd);
    th.setGPR(1, mem - 112);
    uint8_t instr[] = { 0x83, 0x61, 0x00, 0x70 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(27) == 0x11223344 );
}

TEST_CASE("fadd f31,f31,f0") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setFPRd(31, 1.);
    th.setFPRd(0, 2.);
    uint8_t instr[] = { 0xff, 0xff, 0x00, 0x2a };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getFPRd(31) == 3. );
}

TEST_CASE("fcmpu cr7,f1,f30") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setFPRd(1, 1.);
    th.setFPRd(30, 2.);
    uint8_t instr[] = { 0xff, 0x81, 0xf0, 0x00 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getCRF(7) == 8 );
}

TEST_CASE("float loads") {
/*
1043c:       c0 21 00 00     lfs     f1,0(r1)
10440:       7c 41 04 2e     lfsx    f2,r1,r0
10444:       c4 61 00 00     lfsu    f3,0(r1)
10448:       7c 81 04 6e     lfsux   f4,r1,r0
1044c:       c8 a2 00 00     lfd     f5,0(r2)
10450:       7c c2 04 ae     lfdx    f6,r2,r0
10454:       cc e2 00 00     lfdu    f7,0(r2)
10458:       7d 02 04 ee     lfdux   f8,r2,r0
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    auto mem = 0x400000;
    mm.mark(mem, 0x1000, false, "");
    mm.setMemory(mem, 0, 16);
    mm.store64(mem,     0x3f92339c00000000); // float
    mm.store64(mem + 8, 0x3ff2467381d7dbf5); // double
    uint8_t instr[] = { 
        0xc0, 0x21, 0x00, 0x00, 0x7c, 0x41, 0x04, 0x2e, 0xc4, 0x61, 0x00,
        0x00, 0x7c, 0x81, 0x04, 0x6e, 0xc8, 0xa2, 0x00, 0x00, 0x7c, 0xc2,
        0x04, 0xae, 0xcc, 0xe2, 0x00, 0x00, 0x7d, 0x02, 0x04, 0xee
    };
    for (auto i = 0u; i < sizeof(instr); i += 4) {
        th.setGPR(0, 0);
        th.setGPR(1, mem);
        th.setGPR(2, mem + 8);
        ppu_dasm<DasmMode::Emulate>(instr + i, 0, &th);
    }
    REQUIRE( th.getFPR(1) == 0x3ff2467380000000 ); // float -> double
    REQUIRE( th.getFPR(2) == 0x3ff2467380000000 );
    REQUIRE( th.getFPR(3) == 0x3ff2467380000000 );
    REQUIRE( th.getFPR(4) == 0x3ff2467380000000 );
    REQUIRE( th.getFPR(5) == 0x3ff2467381d7dbf5 ); // double -> double
    REQUIRE( th.getFPR(6) == 0x3ff2467381d7dbf5 );
    REQUIRE( th.getFPR(7) == 0x3ff2467381d7dbf5 );
    REQUIRE( th.getFPR(8) == 0x3ff2467381d7dbf5 );
}

TEST_CASE("float loads with update") {
/*
   10654:       c4 61 00 70     lfsu    f3,112(r1)              # 70
   10658:       7c 81 04 6e     lfsux   f4,r1,r0
   1065c:       cc e2 ff c4     lfdu    f7,-60(r2)              # ffffffc4
   10660:       7d 02 04 ee     lfdux   f8,r2,r0
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    auto mem = 0x400000;
    mm.mark(mem, 0x1000, false, "");
    mm.setMemory(mem, 0, 16);
    uint8_t instr[] = { 
          0xc4, 0x61, 0x00, 0x70
        , 0x7c, 0x81, 0x04, 0x6e
        , 0xcc, 0xe2, 0xff, 0xc4
        , 0x7d, 0x02, 0x04, 0xee
    };
    
    th.setGPR(1, mem - 112);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(1) == mem );
    th.setGPR(1, mem - 500);
    th.setGPR(0, 500);
    ppu_dasm<DasmMode::Emulate>(instr + 4, 0, &th);
    REQUIRE( th.getGPR(1) == mem );
    th.setGPR(2, mem + 60);
    ppu_dasm<DasmMode::Emulate>(instr + 8, 0, &th);
    REQUIRE( th.getGPR(1) == mem );
    th.setGPR(2, mem + 200);
    th.setGPR(0, -200);
    ppu_dasm<DasmMode::Emulate>(instr + 12, 0, &th);
    REQUIRE( th.getGPR(2) == mem );
}

TEST_CASE("float stores") {
/*
10414:  d0 21 00 00     stfs    f1,0(r1)
10418:  7c 22 05 2e     stfsx   f1,r2,r0
1041c:  d4 23 00 00     stfsu   f1,0(r3)
10420:  7c 24 05 6e     stfsux  f1,r4,r0
10424:  d8 25 00 00     stfd    f1,0(r5)
10428:  7c 26 05 ae     stfdx   f1,r6,r0
1042c:  dc 27 00 00     stfdu   f1,0(r7)
10430:  7c 28 05 ee     stfdux  f1,r8,r0
10434:  7c 29 07 ae     stfiwx  f1,r9,r0
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    auto mem = 0x400000;
    mm.mark(mem, 0x1000, false, "");
    mm.setMemory(mem, 0, 100);
    th.setFPRd(1, 1.1);
    th.setGPR(0, 0);
    for (int i = 0; i < 9; ++i) {
        th.setGPR(i + 1, mem + i * 8);   
    }
    uint8_t instr[] = { 
        0xd0, 0x21, 0x00 ,0x00, 0x7c, 0x22, 0x05, 0x2e, 0xd4, 0x23, 0x00, 0x00, 
        0x7c, 0x24, 0x05, 0x6e, 0xd8, 0x25, 0x00, 0x00, 0x7c, 0x26, 0x05, 0xae,
        0xdc, 0x27, 0x00, 0x00, 0x7c, 0x28, 0x05, 0xee, 0x7c, 0x29, 0x07, 0xae
    };
    for (auto i = 0u; i < sizeof(instr); i += 4) {
        ppu_dasm<DasmMode::Emulate>(instr + i, 0, &th);
    }
    REQUIRE( mm.load64(mem + 0*8) == 0x3f8ccccd00000000 );
    REQUIRE( mm.load64(mem + 1*8) == 0x3f8ccccd00000000 );
    REQUIRE( mm.load64(mem + 2*8) == 0x3f8ccccd00000000 );
    REQUIRE( th.getGPR(3) == mem + 2*8);
    REQUIRE( mm.load64(mem + 3*8) == 0x3f8ccccd00000000 );
    REQUIRE( th.getGPR(4) == mem + 3*8);
    REQUIRE( mm.load64(mem + 4*8) == 0x3ff199999999999a );
    REQUIRE( mm.load64(mem + 5*8) == 0x3ff199999999999a );
    REQUIRE( mm.load64(mem + 6*8) == 0x3ff199999999999a );
    REQUIRE( th.getGPR(7) == mem + 6*8);
    REQUIRE( mm.load64(mem + 7*8) == 0x3ff199999999999a );
    REQUIRE( th.getGPR(8) == mem + 7*8);
    REQUIRE( mm.load64(mem + 8*8) == 0x9999999a00000000 );
}

TEST_CASE("float moves") {
/*
10460:       fc 40 08 90     fmr     f2,f1
10464:       fc 40 08 50     fneg    f2,f1
10468:       fc 40 0a 10     fabs    f2,f1
1046c:       fc 40 09 10     fnabs   f2,f1
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 
          0xfc, 0x40, 0x08, 0x90
        , 0xfc, 0x40, 0x08, 0x50
        , 0xfc, 0x40, 0x0a, 0x10
        , 0xfc, 0x40, 0x09, 0x10
    };
    th.setFPR(1, 0x3ff2467381d7dbf5);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getFPR(2) == 0x3ff2467381d7dbf5 );
    ppu_dasm<DasmMode::Emulate>(instr + 4, 0, &th);
    REQUIRE( th.getFPR(2) == 0xbff2467381d7dbf5 );
    th.setFPR(1, 0xbff2467381d7dbf5);
    ppu_dasm<DasmMode::Emulate>(instr + 8, 0, &th);
    REQUIRE( th.getFPR(2) == 0x3ff2467381d7dbf5 );
    ppu_dasm<DasmMode::Emulate>(instr + 12, 0, &th);
    REQUIRE( th.getFPR(2) == 0xbff2467381d7dbf5 );
}

TEST_CASE("mtocrf 8,r17") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 0x7e, 0x30, 0x81, 0x20 };
    th.setGPR(17, 0x1111111123456789);
    th.setCR(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getCRF(4) == 6 );
    REQUIRE( th.getCR() == 0x00006000 );
    th.setCR(0x24242888);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getCRF(4) == 6 );
    REQUIRE( th.getCR() == 0x24246888 );
    th.setGPR(17, 0x48008084);
    th.setCR(0x28002082);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getCRF(4) == 8 );
    REQUIRE( th.getCR() == 0x28008082 );
}

TEST_CASE("arithmetic_shifts") {
/*
11808:       7c a0 fe 70     srawi   r0,r5,31                # 1f
11824:       7c a5 07 b4     extsw   r5,r5
127a0:       7c 0b fe 70     srawi   r11,r0,31               # 1f
1a040:       7d 2b fe 70     srawi   r11,r9,31               # 1f
105dc:       7c 41 0e 70     srawi   r1,r2,1
105e0:       7c 41 0e 74     sradi   r1,r2,1
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 
          0x7c, 0xa0, 0xfe, 0x70
        , 0x7c, 0xa5, 0x07, 0xb4
        , 0x7c, 0x0b, 0xfe, 0x70
        , 0x7d, 0x2b, 0xfe, 0x70
        , 0x7c, 0x41, 0x0e, 0x70
        , 0x7c, 0x41, 0x0e, 0x74
    };
    th.setGPR(5, 0);
    th.setGPR(0, 100);
    th.setGPR(11, 150);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 0 );
    
    th.setGPR(5, -1ul);
    ppu_dasm<DasmMode::Emulate>(instr + 4, 0, &th);
    REQUIRE( th.getGPR(5) == -1ull );
    
    th.setGPR(0, 0x65);
    ppu_dasm<DasmMode::Emulate>(instr + 8, 0, &th);
    REQUIRE( th.getGPR(11) == 0 );
    
    th.setGPR(9, 0x497e6);
    ppu_dasm<DasmMode::Emulate>(instr + 12, 0, &th);
    REQUIRE( th.getGPR(11) == 0 );
    
    th.setGPR(2, 0x80000000);
    ppu_dasm<DasmMode::Emulate>(instr + 16, 0, &th);
    REQUIRE( th.getGPR(1) == 0xffffffffc0000000 );
    REQUIRE( th.getCA() == 0 );
    
    th.setGPR(2, 0x80000001);
    ppu_dasm<DasmMode::Emulate>(instr + 16, 0, &th);
    REQUIRE( th.getGPR(1) == 0xffffffffc0000000 );
    REQUIRE( th.getCA() == 1 );
    
    th.setGPR(2, 0x8000000000000000);
    ppu_dasm<DasmMode::Emulate>(instr + 20, 0, &th);
    REQUIRE( th.getGPR(1) == 0xc000000000000000 );
    REQUIRE( th.getCA() == 0 );
    
    th.setGPR(2, 0x8000000000000001);
    ppu_dasm<DasmMode::Emulate>(instr + 20, 0, &th);
    REQUIRE( th.getGPR(1) == 0xc000000000000000 );
    REQUIRE( th.getCA() == 1 );
}

TEST_CASE("shifts") {
/*
105e8:       7c 41 18 36     sld     r1,r2,r3
105ec:       7c 41 18 30     slw     r1,r2,r3
105f0:       7c 41 1c 36     srd     r1,r2,r3
105f4:       7c 41 1c 30     srw     r1,r2,r3
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 
          0x7c, 0x41, 0x18, 0x36
        , 0x7c, 0x41, 0x18, 0x30
        , 0x7c, 0x41, 0x1c, 0x36
        , 0x7c, 0x41, 0x1c, 0x30
    };
    th.setGPR(2, 0xc000000000000001);
    th.setGPR(3, 1);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(1) == 0x8000000000000002 );
    
    th.setGPR(2, 0xff000000c0000001);
    ppu_dasm<DasmMode::Emulate>(instr + 4, 0, &th);
    REQUIRE( th.getGPR(1) == 0x0000000080000002 );
    
    th.setGPR(2, 0xc000000000000001);
    ppu_dasm<DasmMode::Emulate>(instr + 8, 0, &th);
    REQUIRE( th.getGPR(1) == 0x6000000000000000 );
    
    th.setGPR(2, 0xff00000fc0000001);
    ppu_dasm<DasmMode::Emulate>(instr + 12, 0, &th);
    REQUIRE( th.getGPR(1) == 0x0000000060000000 );
    
    th.setGPR(2, 1);
    th.setGPR(3, 55);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(1) == 0x80000000000000 );
    
    ppu_dasm<DasmMode::Emulate>(instr + 4, 0, &th);
    REQUIRE( th.getGPR(1) == 0 );
    
    th.setGPR(3, 22);
    ppu_dasm<DasmMode::Emulate>(instr + 4, 0, &th);
    REQUIRE( th.getGPR(1) == 0x400000 );
    
    th.setGPR(2, 0x8000000000000000);
    th.setGPR(3, 59);
    ppu_dasm<DasmMode::Emulate>(instr + 8, 0, &th);
    REQUIRE( th.getGPR(1) == 16 );
    
    ppu_dasm<DasmMode::Emulate>(instr + 12, 0, &th);
    REQUIRE( th.getGPR(1) == 0 );
    
    th.setGPR(2, 0x80000000);
    th.setGPR(3, 28);
    ppu_dasm<DasmMode::Emulate>(instr + 12, 0, &th);
    REQUIRE( th.getGPR(1) == 8 );
    
    th.setGPR(2, 0xff0000000000ff00);
    th.setGPR(3, 80);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(1) == 0 );
    ppu_dasm<DasmMode::Emulate>(instr + 8, 0, &th);
    REQUIRE( th.getGPR(1) == 0 );
    
    th.setGPR(3, 50);
    ppu_dasm<DasmMode::Emulate>(instr + 4, 0, &th);
    REQUIRE( th.getGPR(1) == 0 );
    ppu_dasm<DasmMode::Emulate>(instr + 12, 0, &th);
    REQUIRE( th.getGPR(1) == 0 );
}

TEST_CASE("rolls") {
    REQUIRE( rol<uint64_t>(0x8000000000000001ull, 1) == 3 );
    REQUIRE( rol<uint32_t>(0x40000001, 2) == 5 );
}

TEST_CASE("fixed mulls") {
/*
   105fc:       1c 82 00 78     mulli   r4,r2,120               # 78
   10600:       7c a2 19 d2     mulld   r5,r2,r3
   10604:       7c c2 19 d6     mullw   r6,r2,r3
   10608:       7c e2 18 92     mulhd   r7,r2,r3
   1060c:       7d 02 18 96     mulhw   r8,r2,r3
   10610:       7d 22 18 12     mulhdu  r9,r2,r3
   10614:       7d 42 18 16     mulhwu  r10,r2,r3
   10618:       1d 62 ff 38     mulli   r11,r2,-200             # ffffff38
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 
          0x1c, 0x82, 0x00, 0x78
        , 0x7c, 0xa2, 0x19, 0xd2
        , 0x7c, 0xc2, 0x19, 0xd6
        , 0x7c, 0xe2, 0x18, 0x92
        , 0x7d, 0x02, 0x18, 0x96
        , 0x7d, 0x22, 0x18, 0x12
        //, 0x7d, 0x42, 0x18, 0x16
        , 0x1d, 0x62, 0xff, 0x38
    };
    th.setGPR(2, 0x12345678abcdef90);
    th.setGPR(3, 0xa1b2c3d4e5f67890);
    
    for (auto i = 0u; i < sizeof(instr); i += 4) {
        ppu_dasm<DasmMode::Emulate>(instr + i, 0, &th);
    }
    
    REQUIRE( th.getGPR(4) == 0x8888889088884b80 );
    REQUIRE( th.getGPR(5) == 0x15c296d930824100 );
    REQUIRE( th.getGPR(6) == 0x9a54a01930824100 );
    REQUIRE( th.getGPR(7) == 0xb7fa0b30583d7de );
    REQUIRE( (th.getGPR(8) & 0xffffffff) == 0x89037f9 );
    REQUIRE( th.getGPR(9) == 0xb7fa0b30583d7de );
    //REQUIRE( th.getGPR(10) == 0x8888889088884b80 );
    REQUIRE( th.getGPR(11) == 0xc71c71b9c71cd780 );
        
    th.setGPR(2, 0xffffffffffff8a69ull);
    th.setGPR(3, 0x14f8b589ull);
    ppu_dasm<DasmMode::Emulate>(instr + 16, 0, &th);
    REQUIRE( (th.getGPR(8) & 0xffffffffull) == 0xfffff65dull);
    
    th.setGPR(2, ~0ull);
    th.setGPR(3, 10);
    ppu_dasm<DasmMode::Emulate>(instr + 20, 0, &th);
    REQUIRE( (th.getGPR(9) ) == 9);
}

TEST_CASE("rlwinm r0,r9,0,17,27") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setGPR(9, 0xffffffff);
    uint8_t instr[] = { 0x55, 0x20, 0x04, 0x76 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(0) == 0x7ff0 );
}

TEST_CASE("fixed divs") {
/*
   10624:       7c 82 1b d2     divd    r4,r2,r3
   10628:       7c a2 1b d6     divw    r5,r2,r3
   1062c:       7c c2 1b 92     divdu   r6,r2,r3
   10630:       7c e2 1b 96     divwu   r7,r2,r3
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 
          0x7c, 0x82, 0x1b, 0xd2
        , 0x7c, 0xa2, 0x1b, 0xd6
        , 0x7c, 0xc2, 0x1b, 0x92
        , 0x7c, 0xe2, 0x1b, 0x96
    };
    th.setGPR(2, 0xf1f23456abcdef09);
    th.setGPR(3, 0x1234567887654321);
    
    for (auto i = 0u; i < sizeof(instr); i += 4) {
        ppu_dasm<DasmMode::Emulate>(instr + i, 0, &th);
    }
    
    REQUIRE( th.getGPR(4) == 0 );
    REQUIRE( (th.getGPR(5) & 0xffffffff) == 0 );
    REQUIRE( th.getGPR(6) == 13 );
    REQUIRE( (th.getGPR(7) & 0xffffffff) == 1 );
    
    th.setGPR(3, 2);
    
    for (auto i = 0u; i < sizeof(instr); i += 4) {
        ppu_dasm<DasmMode::Emulate>(instr + i, 0, &th);
    }
    
    REQUIRE( th.getGPR(4) == -506344709675354235 );
    REQUIRE( (th.getGPR(5) & 0xffffffff) == -706283643 );
    REQUIRE( th.getGPR(6) == 8717027327179421572 );
    REQUIRE( (th.getGPR(7) & 0xffffffff) == 0x55e6f784 );
}

TEST_CASE("fcfid f1,f2") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 0xfc, 0x20, 0x16, 0x9c };
    th.setFPR(2, 0x12345678abcdef90);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getFPR(1) == 0x43b2345678abcdf0 );
}

TEST_CASE("mtspr") {
/*
   10644:       7c 21 03 a6     mtxer   r1
   10648:       7c 28 03 a6     mtlr    r1
   1064c:       7c 29 03 a6     mtctr   r1
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = {
          0x7c, 0x21, 0x03, 0xa6
        , 0x7c, 0x28, 0x03, 0xa6
        , 0x7c, 0x29, 0x03, 0xa6
    };
    th.setGPR(1, 0x12345678abcdef90);
    th.setCR(0);
    th.setXER(0);
    th.setLR(0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getXER() == 0x12345678abcdef90 );
    ppu_dasm<DasmMode::Emulate>(instr + 4, 0, &th);
    REQUIRE( th.getLR() == 0x12345678abcdef90 );
    ppu_dasm<DasmMode::Emulate>(instr + 8, 0, &th);
    REQUIRE( th.getCTR() == 0x12345678abcdef90 );
}

TEST_CASE("mfcr r1") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 0x7c, 0x20, 0x00, 0x26 };
    th.setCR(0x12345678);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(1) == 0x12345678 );
}

TEST_CASE("fctiwz f1,f2") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 0xfc, 0x20, 0x10, 0x1e };
    th.setFPRd(2, 0);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( (th.getFPR(1) & 0xffffffff) == 0 );    
    th.setFPRd(2, 3.14);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( (th.getFPR(1) & 0xffffffff) == 3 );    
    th.setFPRd(2, -3.14);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( (th.getFPR(1) & 0xffffffff) == -3 );
    th.setFPRd(2, 4294967295.);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( (th.getFPR(1) & 0xffffffff) == 0x7fffffff );
    th.setFPRd(2, -4294967295.);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( (th.getFPR(1) & 0xffffffff) == 0x80000000 );
}

TEST_CASE("fixed logical") {
/*
   10674:       70 43 00 71     andi.   r3,r2,113               # 71
   10678:       74 44 03 d6     andis.  r4,r2,982               # 3d6
   1067c:       70 45 ff ff     andi.   r5,r2,65535             # ffff
   10680:       74 46 ff ff     andis.  r6,r2,65535             # ffff
   10684:       60 47 03 0c     ori     r7,r2,780               # 30c
   10688:       64 48 03 e7     oris    r8,r2,999               # 3e7
   1068c:       64 49 f0 01     oris    r9,r2,61441             # f001
   10690:       68 4a 00 0c     xori    r10,r2,12
   10694:       6c 4b 00 21     xoris   r11,r2,33               # 21
   10698:       6c 4c 80 01     xoris   r12,r2,32769            # 8001
   1069c:       7c 2d 10 38     and     r13,r1,r2
   106a0:       7c 2e 13 78     or      r14,r1,r2
   106a4:       7c 2f 12 78     xor     r15,r1,r2
   106a8:       7c 30 13 b8     nand    r16,r1,r2
   106ac:       7c 31 10 f8     nor     r17,r1,r2
   106b0:       7c 32 12 38     eqv     r18,r1,r2
   106b4:       7c 33 10 78     andc    r19,r1,r2
   106b8:       7c 34 13 38     orc     r20,r1,r2
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 
          0x70, 0x43, 0x00, 0x71
        , 0x74, 0x44, 0x03, 0xd6
        , 0x70, 0x45, 0xff, 0xff
        , 0x74, 0x46, 0xff, 0xff
        , 0x60, 0x47, 0x03, 0x0c
        , 0x64, 0x48, 0x03, 0xe7
        , 0x64, 0x49, 0xf0, 0x01
        , 0x68, 0x4a, 0x00, 0x0c
        , 0x6c, 0x4b, 0x00, 0x21
        , 0x6c, 0x4c, 0x80, 0x01
        , 0x7c, 0x2d, 0x10, 0x38
        , 0x7c, 0x2e, 0x13, 0x78
        , 0x7c, 0x2f, 0x12, 0x78
        , 0x7c, 0x30, 0x13, 0xb8
        , 0x7c, 0x31, 0x10, 0xf8
        , 0x7c, 0x32, 0x12, 0x38
        , 0x7c, 0x33, 0x10, 0x78
        , 0x7c, 0x34, 0x13, 0x38
    };
    th.setGPR(1, 0x1234567887654321);
    th.setGPR(2, 0xabcdef1198765430);
    
    for (auto i = 0u; i < sizeof(instr); i += 4) {
        ppu_dasm<DasmMode::Emulate>(instr + i, 0, &th);
    }
    
    REQUIRE( th.getGPR(3) == 48 );
    REQUIRE( th.getGPR(4) == 5636096 );
    REQUIRE( th.getGPR(5) == 21552 );
    REQUIRE( th.getGPR(6) == 2557870080 );
    REQUIRE( th.getGPR(7) == 0xabcdef119876573c );
    REQUIRE( th.getGPR(8) == 0xabcdef119bf75430 );
    REQUIRE( th.getGPR(9) == 0xabcdef11f8775430 );
    REQUIRE( th.getGPR(10) == 0xabcdef119876543c );
    REQUIRE( th.getGPR(11) == 0xabcdef1198575430 );
    REQUIRE( th.getGPR(12) == 0xabcdef1118775430 );
    REQUIRE( th.getGPR(13) == 0x204461080644020 );
    REQUIRE( th.getGPR(14) == 0xbbfdff799f775731 );
    REQUIRE( th.getGPR(15) == 0xb9f9b9691f131711 );
    REQUIRE( th.getGPR(16) == 0xfdfbb9ef7f9bbfdf );
    REQUIRE( th.getGPR(17) == 0x440200866088a8ce );
    REQUIRE( th.getGPR(18) == 0x46064696e0ece8ee );
    REQUIRE( th.getGPR(19) == 0x1030106807010301 );
    REQUIRE( th.getGPR(20) == 0x563656fee7edebef );
}

TEST_CASE("extend sign") {
/*
   106c0:       7c 23 07 74     extsb   r3,r1
   106c4:       7c 44 07 74     extsb   r4,r2
   106c8:       7c 25 07 34     extsh   r5,r1
   106cc:       7c 46 07 34     extsh   r6,r2
   106d0:       7c 27 07 b4     extsw   r7,r1
   106d4:       7c 48 07 b4     extsw   r8,r2
*/
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 
          0x7c, 0x23, 0x07, 0x74
        , 0x7c, 0x44, 0x07, 0x74
        , 0x7c, 0x25, 0x07, 0x34
        , 0x7c, 0x46, 0x07, 0x34
        , 0x7c, 0x27, 0x07, 0xb4
        , 0x7c, 0x48, 0x07, 0xb4
    };
    th.setGPR(1, 0xff77777781228384);
    th.setGPR(2, 0xff77777711223344);
    
    for (auto i = 0u; i < sizeof(instr); i += 4) {
        ppu_dasm<DasmMode::Emulate>(instr + i, 0, &th);
    }
    
    REQUIRE( th.getGPR(3) == 0xffffffffffffff84 );
    REQUIRE( th.getGPR(4) == 0x44 );
    REQUIRE( th.getGPR(5) == 0xffffffffffff8384 );
    REQUIRE( th.getGPR(6) == 0x3344 );
    REQUIRE( th.getGPR(7) == 0xffffffff81228384 );
    REQUIRE( th.getGPR(8) == 0x11223344 );
}

TEST_CASE("blrl") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setLR(0x33114450);
    uint8_t instr[] = { 0x4e, 0x80, 0x00, 0x21 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getNIP() == 0x33114450 );
}

TEST_CASE("bctrl") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setCTR(0x33114450);
    uint8_t instr[] = { 0x4e, 0x80, 0x04, 0x21 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getNIP() == 0x33114450 );
}

TEST_CASE("cntlzw r11,r11") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    th.setGPR(11, 0x8000);
    uint8_t instr[] = { 0x7d, 0x6b, 0x00, 0x34 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(11) == 16 );
}

TEST_CASE("vsldoi") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    unsigned __int128 i = 0x1122334455667788ull;
    i <<= 64;
    i |= 0xaabbccddeeff0099ull;
    
    unsigned __int128 res = 0x22334455667788aaull;
    res <<= 64;
    res |= 0xbbccddeeff009911ull;
    
    th.setV(0, i);
    // vsldoi v2,v0,v0,1
    uint8_t instr[] = { 0x10, 0x40, 0x00, 0x6c };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( (th.getV(2) == res) );
    
    // vsldoi v2,v0,v0,0
    uint8_t instr2[] = { 0x10, 0x40, 0x00, 0x2c };
    ppu_dasm<DasmMode::Emulate>(instr2, 0, &th);
    REQUIRE( (th.getV(2) == i) );
}

TEST_CASE("vxor v0,v0,v0") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    unsigned __int128 i = 0x1122334455667788ull;
    i <<= 64;
    i |= 0xaabbccddeeff0099ull;
    th.setV(0, i);
    uint8_t instr[] = { 0x10, 0x00, 0x04, 0xc4 };
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( (th.getV(0) == 0) );
}

TEST_CASE("addc r5,r4,r3") {
    MainMemory mm;
    g_state.mm = &mm;
    PPUThread th;
    uint8_t instr[] = { 0x7C, 0xA4, 0x18, 0x14 };
    
    th.setGPR(3, 100);
    th.setGPR(4, 200);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(5) == 300 );
    REQUIRE( th.getCA() == 0 );
    
    th.setGPR(3, 0xffffffffffffffffull);
    th.setGPR(4, 0xffffffffffffffffull);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(5) == -2ull );
    REQUIRE( th.getCA() == 1 );
    
    th.setGPR(3, 1);
    th.setGPR(4, 2);
    ppu_dasm<DasmMode::Emulate>(instr, 0, &th);
    REQUIRE( th.getGPR(5) == 3 );
    REQUIRE( th.getCA() == 0 );
}

TEST_CASE("ppu_failed_store_should_not_destroy_reservation") {
    MainMemory mm;
    mm.mark(0x10000, 0x1000, false, "");
    mm.setMemory(0x10000, 0x11, 1000);
    
    boost::thread ppu([&] {
        uint32_t buf;
        ReservationGranule granule;
        g_state.granule = &granule;
        mm.loadReserve<4>(0x10000, &buf);
        REQUIRE(buf == 0x11111111);
        buf = 0xaabbccdd;
        REQUIRE(mm.writeCond<4>(0x10000, &buf) == true);
        REQUIRE(mm.load32(0x10000) == 0xddccbbaa);
    });
    ppu.join();
    
    std::atomic<bool> step1(false), step2(false);
    
    // th1 gets reservation
    boost::thread th1([&] {
        ReservationGranule granule;
        g_state.granule = &granule;
        uint32_t buf = 0;
        mm.loadReserve<4>(0x10000, &buf);
        // same-thread store invalidates the reservation for a cond store of
        // any other thread, but doesn't destroy the reservation
        mm.store32(0x10000, 0xddccbbaa);
        
        step1 = true;
        while (!step2) ;
        
        // reservation still active, th2 didn't destroy it with writeCond
        buf = 0xffffffff;
        REQUIRE(mm.writeCond<4>(0x10000, &buf) == true);
        REQUIRE(mm.load32(0x10000) == 0xffffffff);
    });
    
    while (!step1) ;
    
    // th2 doesn't have reservation, makes a conditional store
    boost::thread th2([&] {
        ReservationGranule granule;
        g_state.granule = &granule;
        auto buf = 0x12345678;
        REQUIRE(mm.writeCond<4>(0x10000, &buf) == false);
        REQUIRE(mm.load32(0x10000) == 0xddccbbaa);
    });
    th2.join();
    
    step2 = true;
    th1.join();
}

TEST_CASE("ppu_lwarx_should_not_destroy_reservations_of_other_threads") {
    MainMemory mm;
    mm.mark(0x10000, 0x1000, false, "");
    mm.setMemory(0x10000, 0x11, 1000);
    
    std::atomic<bool> th1_step1 = false, th1_step2 = false;
    std::atomic<bool> th2_step1 = false, th2_step2 = false;
    
    boost::thread th1([&] {
        uint32_t buf;
        ReservationGranule granule;
        g_state.granule = &granule;
        
        // th1 gets reservation
        mm.loadReserve<4>(0x10000, &buf);
        
        th1_step1 = true;
        while (!th2_step1) ;
        
        // th1 moves its reservation to another location
        mm.loadReserve<4>(0x10200, &buf);
        
        th1_step2 = true;
    });
    
    boost::thread th2([&] {
        ReservationGranule granule;
        g_state.granule = &granule;
        uint32_t buf = 0;
        
        while (!th1_step1) ;
        
        // th2 gets reservation at the same address
        mm.loadReserve<4>(0x10000, &buf);

        th2_step1 = true;
        while (!th1_step2) ;
        
        // th2 still has reservation
        buf = 0xffffffff;
        REQUIRE(mm.writeCond<4>(0x10000, &buf) == true);
        REQUIRE(mm.load32(0x10000) == 0xffffffff);
    });
    
    th1.join();
    th2.join();
}
