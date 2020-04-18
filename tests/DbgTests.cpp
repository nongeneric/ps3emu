#include "dbg-gui/DebugExpr.h"
#include "ps3emu/ppu/PPUThread.h"
#include <catch2/catch.hpp>

TEST_CASE("dbg_expressions") {
    PPUThread th;
    uint64_t r;
    r = evalExpr("0", &th);
    REQUIRE(r == 0);
    r = evalExpr("200", &th);
    REQUIRE(r == 200);
    r = evalExpr("#100", &th);
    REQUIRE(r == 0x100);
    r = evalExpr("0x200", &th);
    REQUIRE(r == 0x200);
    r = evalExpr("5 + #7", &th);
    REQUIRE(r == 12);
    r = evalExpr("1 + 2 + 3", &th);
    REQUIRE(r == 6);
    r = evalExpr("1 + 2 * 3", &th);
    REQUIRE(r == 7);
    r = evalExpr("1 * 2 + 3", &th);
    REQUIRE(r == 5);
}
