#include "../ps3emu/ppu_dasm.h"
#include <catch.hpp>

TEST_CASE("common bitfield operations") {
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