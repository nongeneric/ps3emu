#include "../disassm/ppu_dasm.h"
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
    REQUIRE(res == "stdu 1,-112(1)");
}

TEST_CASE("mtlr") {
    uint8_t instr[] = { 0x7c, 0x08, 0x03, 0xa6 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x10248, &res);
    REQUIRE(res == "mtlr 0");
}

TEST_CASE("clrldi") {
    uint8_t instr[] = { 0x79, 0x04, 0x00, 0x20 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x10248, &res);
    REQUIRE(res == "clrldi 4,8,32");
}

TEST_CASE("stw") {
    uint8_t instr[] = { 0x93, 0xab, 0x00, 0x00 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x10248, &res);
    REQUIRE(res == "stw 29,0(11)");
}