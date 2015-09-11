#include "../ps3emu/shader_dasm.h"
#include <catch.hpp>

TEST_CASE() {
    uint8_t instr[] = { 
        0x3E, 0x01, 0x01, 0x00, 0xC8, 0x01, 0x1C, 0x9D, 
        0xC8, 0x00, 0x00, 0x01, 0xC8, 0x00, 0x3F, 0xE1
    };
    std::string res;
    fragment_dasm(instr, res);
    REQUIRE(res == "MOVR R0, f[COL0]; # last instruction");
}