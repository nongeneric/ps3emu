#include "../ps3emu/ppu/ppu_dasm.h"
#include "../ps3emu/BitField.h"
#include <catch/catch.hpp>

TEST_CASE("bitfield_common") {
    BitField<0, 32> word;
    auto ptr = reinterpret_cast<uint32_t*>(&word);
    *ptr = 0;
    REQUIRE(word.u() == 0);
    REQUIRE(word.s() == 0);
    BitField<4, 7> _3bits;
    ptr = reinterpret_cast<uint32_t*>(&_3bits);
    *ptr = 0;
    REQUIRE(_3bits.s() == 0);
    *ptr = -1;
    REQUIRE(_3bits.s() == -1);
    REQUIRE(_3bits.u() == 7);
}

TEST_CASE("bitfield_write") {
    union {
        uint32_t val;
        BitField<0, 8> bf1;
        BitField<8, 16> bf2;
        BitField<16, 32> bf3;
    } f = { 0 };
    f.bf1.set(0x4f);
    f.bf2.set(0x12);
    f.bf3.set(0xabcd);
    REQUIRE( f.val == 0x4f12abcd );
    REQUIRE( f.bf2.u() == 0x12 );
}

TEST_CASE("bitfield_write_2") {
    union {
        uint32_t val;
        BitField<0, 5> r;
        BitField<5, 11> g;
        BitField<11, 16> b;
    } f { 0 };
    f.r.set(2);
    f.g.set(4);
    f.b.set(2);
    REQUIRE( f.r.u() == 2 );
    REQUIRE( f.g.u() == 4 );
    REQUIRE( f.b.u() == 2 );
    REQUIRE( f.val == 0x10820000 );
}

TEST_CASE("bitfield_write_3") {
    union {
        uint32_t val;
        BitField<10, 13> f;
    } f { 0 };
    f.f.set(-1);
    REQUIRE( f.f.u() == 0b111 );
    REQUIRE( f.val == 0x380000 );
}


TEST_CASE("bl instruction") {
    uint8_t instr[] = { 0x48, 0x00, 0x01, 0x09 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x10248, &res);
    REQUIRE(res == "bl 10350");
}

TEST_CASE("signed stdu") {
    uint8_t instr[] = { 0xf8, 0x21, 0xff, 0x91 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x10248, &res);
    REQUIRE(res == "stdu r1,-112(r1)");
}

TEST_CASE("mtlr") {
    uint8_t instr[] = { 0x7c, 0x08, 0x03, 0xa6 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x10248, &res);
    REQUIRE(res == "mtlr r0");
}

TEST_CASE("clrldi") {
    uint8_t instr[] = { 0x79, 0x04, 0x00, 0x20 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x10248, &res);
    REQUIRE(res == "clrldi r4,r8,32");
}

TEST_CASE("stw") {
    uint8_t instr[] = { 0x93, 0xab, 0x00, 0x00 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x10248, &res);
    REQUIRE(res == "stw r29,0(r11)");
}

TEST_CASE("cmpld") {
    uint8_t instr[] = { 0x7f, 0xbf, 0x40, 0x40 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x10248, &res);
    REQUIRE(res == "cmpld cr7,r31,r8");
}

TEST_CASE("bge") {
    uint8_t instr[] = { 0x40, 0x9c, 0xff, 0xe0 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x103d4, &res);
    REQUIRE(res == "bge cr7,103b4");
}

TEST_CASE("blt") {
    uint8_t instr[] = { 0x41, 0x9c, 0x00, 0x60 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x10754, &res);
    REQUIRE(res == "blt cr7,107b4");
}

TEST_CASE("blt cr0") {
    uint8_t instr[] = { 0x41, 0x80, 0x00, 0x0c };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x18070, &res);
    REQUIRE(res == "blt 1807c");
}

TEST_CASE("bdnz") {
    uint8_t instr[] = { 0x42, 0x00, 0xff, 0xe8 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x181bc, &res);
    REQUIRE(res == "bdnz 181a4");
}

TEST_CASE("beq+") {
    uint8_t instr[] = { 0x41, 0xfe, 0xff, 0xb0 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x12554, &res);
    REQUIRE(res == "beq+ cr7,12504");
}

TEST_CASE("beq-") {
    uint8_t instr[] = { 0x41, 0xde, 0x00, 0xac };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x12524, &res);
    REQUIRE(res == "beq- cr7,125d0");
}

TEST_CASE("beqlr") {
    uint8_t instr[] = { 0x4d, 0x86, 0x00, 0x20 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x12524, &res);
    REQUIRE(res == "beqlr cr1");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x42, 0x85, 0x70, 0x79 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "bcl 20,4*cr1+gt,27640");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x41, 0x16, 0x70, 0x79 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "bdnztl 4*cr5+eq,27640");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7f, 0xbe, 0x40, 0x40 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "cmpld cr7,r30,r8");
}

TEST_CASE("rotate dword ext mnemonics") {
/*
rldicr %r0,%r2,10,3
rldicl %r0,%r2,14,54
rldimi %r0,%r2,50,4
rldicl %r0,%r2,10,0
rldicl %r0,%r2,54,0
rldcl  %r0,%r2,13,0
rldicr %r0,%r2,10,53
rldicl %r0,%r2,54,10
rldicl %r0,%r2,0,10
rldicr %r0,%r2,0,53
rldic  %r0,%r2,10,10
rldicl %r0,%r2,1,63
rldimi %r0,%r2,63,0
rldicr %r0,%r2,8,55
rldicl %r0,%r2,0,32
*/
    uint8_t instrs[] = {
          0x78, 0x40, 0x50, 0xc4
        , 0x78, 0x40, 0x75, 0xa0
        , 0x78, 0x40, 0x91, 0x0e
        , 0x78, 0x40, 0x50, 0x00
        , 0x78, 0x40, 0xb0, 0x02
        , 0x78, 0x40, 0x68, 0x10
        , 0x78, 0x40, 0x55, 0x64
        , 0x78, 0x40, 0xb2, 0x82
        , 0x78, 0x40, 0x02, 0x80
        , 0x78, 0x40, 0x05, 0x64
        , 0x78, 0x40, 0x52, 0x88
        , 0x78, 0x40, 0x0f, 0xe0
        , 0x78, 0x40, 0xf8, 0x0e
        , 0x78, 0x40, 0x45, 0xe4
        , 0x78, 0x40, 0x00, 0x20
    };
    std::string strs[] = {
        "extldi r0,r2,4,10",
        "extrdi r0,r2,10,4",
        "insrdi r0,r2,10,4",
        "rotldi r0,r2,10",
        "rotrdi r0,r2,10",
        "rotld r0,r2,r13",
        "sldi r0,r2,10",
        "srdi r0,r2,10",
        "clrldi r0,r2,10",
        "clrrdi r0,r2,10",
        "clrlsldi r0,r2,20,10",
        "srdi r0,r2,63",
        "insrdi r0,r2,1,0",
        "sldi r0,r2,8",
        "clrldi r0,r2,32"
    };
    int j = 0;
    for (auto i = instrs; i < std::end(instrs); i += 4) {
        std::string res;
        ppu_dasm<DasmMode::Print>(i, 0, &res);
        REQUIRE(res == strs[j++]);
    }
}

TEST_CASE("rotate word ext mnemonics") {
/*
rlwinm %r0,%r2,20,0,9
rlwinm %r0,%r2,30,22,31
rlwimi %r0,%r2,12,22,19
rlwimi %r0,%r2,2,20,19
rlwinm %r0,%r2,10,0,31
rlwinm %r0,%r2,22,0,31
rlwnm %r0,%r2,13,0,31
rlwinm %r0,%r2,10,0,21
rlwinm %r0,%r2,22,10,31
rlwinm %r0,%r2,0,10,31
rlwinm %r0,%r2,0,0,21
rlwinm %r0,%r2,10,10,21
rlwinm %r10,%r0,5,0,26
rlwimi %r0,%r2,12,20,29
rlwimi %r0,%r2,2,20,29
*/
    uint8_t instrs[] = {
          0x54, 0x40, 0xa0, 0x12
        , 0x54, 0x40, 0xf5, 0xbe
        , 0x50, 0x40, 0x65, 0xa6
        , 0x50, 0x40, 0x15, 0x26
        , 0x54, 0x40, 0x50, 0x3e
        , 0x54, 0x40, 0xb0, 0x3e
        , 0x5c, 0x40, 0x68, 0x3e
        , 0x54, 0x40, 0x50, 0x2a
        , 0x54, 0x40, 0xb2, 0xbe
        , 0x54, 0x40, 0x02, 0xbe
        , 0x54, 0x40, 0x00, 0x2a
        , 0x54, 0x40, 0x52, 0xaa
        , 0x54, 0x0a, 0x28, 0x34
        , 0x50, 0x40, 0x65, 0x3a
        , 0x50, 0x40, 0x15, 0x3a
    };
    std::string strs[] = {
        "extlwi r0,r2,10,20",
        "extrwi r0,r2,10,20",
        "rlwimi r0,r2,12,22,19",
        "rlwimi r0,r2,2,20,19",
        "rotlwi r0,r2,10",
        "rotrwi r0,r2,10",
        "rotlw r0,r2,r13",
        "slwi r0,r2,10",
        "srwi r0,r2,10",
        "clrlwi r0,r2,10",
        "clrrwi r0,r2,10",
        "clrlslwi r0,r2,20,10",
        "slwi r10,r0,5",
        "inslwi r0,r2,10,20",
        "insrwi r0,r2,10,20"
    };
    int j = 0;
    for (auto i = instrs; i < std::end(instrs); i += 4) {
        std::string res;
        ppu_dasm<DasmMode::Print>(i, 0, &res);
        REQUIRE(res == strs[j++]);
    }
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7d, 0x80, 0x00, 0x26 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "mfcr r12");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7f, 0x60, 0x58, 0x30 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "slw r0,r27,r11");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7d, 0x29, 0x00, 0xd0 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "neg r9,r9");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7d, 0x90, 0x81, 0x20 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "mtocrf 8,r12");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7c, 0x00, 0x1a, 0x2c };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "dcbt r0,r3");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7d, 0x60, 0x00, 0x74 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "cntlzd r0,r11");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7c, 0x09, 0xfe, 0x70 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "srawi r9,r0,31");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x21, 0x60, 0x00, 0x10 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "subfic r11,r0,16");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7d, 0x29, 0xfb, 0x96 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "divwu r9,r9,r31");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7d, 0x3f, 0x49, 0xd6 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "mullw r9,r31,r9");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x98, 0x61, 0x01, 0x12 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "stb r3,274(r1)");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x2f, 0xa0, 0x00, 0x00 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "cmpdi cr7,r0,0");
}

TEST_CASE("") {
    uint8_t instr[] = { 0xfc, 0x1f, 0xf8, 0x24 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "fdiv f0,f31,f31");
}

TEST_CASE("") {
    uint8_t instr[] = { 0xec, 0x00, 0xe7, 0x7a };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "fmadds f0,f0,f29,f28");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x4e, 0x80, 0x00, 0x21 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "blrl");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x4e, 0x80, 0x04, 0x21 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "bctrl");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7d, 0x6b, 0x00, 0x34 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "cntlzw r11,r11");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7c, 0x00, 0x04, 0xac };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "sync 0");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7c, 0x9f, 0xf0, 0x50 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "subf r4,r31,r30");
}

TEST_CASE("") {
    uint8_t instr[] = { 0xec, 0x0a, 0x58, 0x38 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "fmsubs f0,f10,f0,f11");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7c, 0x49, 0x59, 0xce };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "stvx v2,r9,r11");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7c, 0x00, 0x48, 0xce };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "lvx v0,r0,r9");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x10, 0x40, 0x00, 0x2c };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "vsldoi v2,v0,v0,0");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x10, 0x00, 0x04, 0xc4 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "vxor v0,v0,v0");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7c, 0x1f, 0x00, 0x8e };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "lvewx v0,r31,r0");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x10, 0x00, 0x02, 0x8C };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "vspltw v0,v0,0");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x10, 0x01, 0x01, 0x84 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "vslw v0,v1,v0");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x10, 0x0D, 0x00, 0x6E };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "vmaddfp v0,v13,v1,v0");
}

TEST_CASE("") {
    uint8_t instr[] = { 0xfc, 0x00, 0x06, 0x5e };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "fctidz f0,f0");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7c, 0x00, 0x48, 0x4c };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "lvsr v0,r0,r9");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x10, 0x01, 0x04, 0x44 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "vandc v0,v1,v0");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x7f, 0xe0, 0x00, 0x88 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "td 31,r0,r0");
}

TEST_CASE("") {
    uint8_t instr[] = { 0x10, 0x00, 0x60, 0x84 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "vrlw v0,v0,v12");
}

TEST_CASE("mcrf_cr1_cr7") {
    uint8_t instr[] = { 0x4c, 0x9c, 0x00, 0x00 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "mcrf cr1,cr7");
}

TEST_CASE("ppu_dasm_1") {
    uint8_t instr[] = { 
        0x11, 0x41, 0x03, 0x4c,
        0x10, 0x0b, 0x08, 0x4c,
        0x10, 0x2b, 0x09, 0x4c,
        0x10, 0x00, 0x51, 0x48,
    };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x205c8, &res);
    REQUIRE(res == "vspltish v10,1");
    ppu_dasm<DasmMode::Print>(instr + 4, 0x205c8, &res);
    REQUIRE(res == "vmrghh v0,v11,v1");
    ppu_dasm<DasmMode::Print>(instr + 8, 0x205c8, &res);
    REQUIRE(res == "vmrglh v1,v11,v1");
    ppu_dasm<DasmMode::Print>(instr + 12, 0x205c8, &res);
    REQUIRE(res == "vmulosh v0,v0,v10");
}

TEST_CASE("analyze_1") {
    auto info = analyze(0x4e800020, 0x12e3c); // blr
    REQUIRE(!info.passthrough);
    REQUIRE(info.flow);
    REQUIRE(!info.target);
}

TEST_CASE("analyze_2") {
    auto info = analyze(0x4800d181, 0x10b40); // bl
    REQUIRE(info.passthrough);
    REQUIRE(info.flow);
    REQUIRE(*info.target == 0x1dcc0);
}

TEST_CASE("analyze_3") {
    auto info = analyze(0x4C00012C, 0x10b40); // isync
    REQUIRE(!info.flow);
    REQUIRE(!info.target);
}
