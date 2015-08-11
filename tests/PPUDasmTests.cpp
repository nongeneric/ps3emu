#include "../disassm/ppu_dasm.h"
#include <catch.hpp>

TEST_CASE("bl instruction") {
    char instr[] = { 0x48, 0x00, 0x01, 0x09 };
    std::string res;
    ppu_dasm<DasmMode::Print>(instr, 0x10248, &res);
    REQUIRE(res == "bl 10350");
}