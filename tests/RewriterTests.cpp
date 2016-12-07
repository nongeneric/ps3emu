#include "ps3emu/utils.h"
#include "ps3emu/RewriterUtils.h"
#include "TestUtils.h"
#include <catch.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <stdio.h>
#include <stdlib.h>

TEST_CASE("rewriter_simple") {
    std::string output;
    auto res = rewrite(
        "./binaries/rewriter_simple/a.elf",
        "/tmp/x86.cpp",
        "--entries 1022c 10314 1039c 10418 10484 10518 1053c 105ac 10654"
        "--ignored 1045c 104c8",
        output);
    REQUIRE(res);
    res = compile({"/tmp/x86.cpp", "/tmp/x86.so", false, false});
    REQUIRE(res);
    
    output = startWaitGetOutput({"./binaries/rewriter_simple/a.elf"}, {"--x86", "/tmp/x86.so"});
    REQUIRE( output ==
        "test1 ep: 1022c\n"
        "test1 res: 1416\n"
        "test2 ep: 10314 1039c\n"
        "test2 res: -5\n"
        "test3 ep: 10418 1045c(ignore)\n"
        "test3 res: 382\n"
        "test4 ep: 10484 104c8(ignore) 10518\n"
        "test4 res: 98\n"
        "test5 ep: 1053c\n"
        "test5 res: 2008\n"
        "test6 ep: 105ac\n"
        "test6 res: 17\n"
        "test0 ep: 10654\n"
    );
}
