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