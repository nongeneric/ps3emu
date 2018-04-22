#include "ps3emu/enum.h"
#include <catch.hpp>

ENUMF(TestEnum, (A, 1), (B, 2), (C, 4), (D, 2 | 4))
ENUMF(TestEnum2, (A, 0), (B, 1))
ENUM(TestEnum3, (A, 0), (B, 1))

TEST_CASE("enum_test") {
    REQUIRE( enum_traits<TestEnum2>::name() == std::string("TestEnum2") );
    REQUIRE( to_string(TestEnum2::A) == "" );
    REQUIRE( to_string(TestEnum3::A) == "A" );
    
    REQUIRE( (TestEnum::B | TestEnum::C) == TestEnum::D );
    TestEnum b = TestEnum::B;
    TestEnum c = TestEnum::C;
    REQUIRE( (b | c) == TestEnum::D );
    REQUIRE( to_string(b) == "B" );
    REQUIRE( to_string(b | c) == "B | C" );
    REQUIRE(enum_traits<TestEnum>::values() ==
            (std::array<TestEnum, 4>{
                TestEnum::A, TestEnum::B, TestEnum::C, TestEnum::D}));
    auto names = enum_traits<TestEnum>::names();
    REQUIRE((std::vector<std::string>{"A", "B", "C", "D"}) ==
            std::vector<std::string>(begin(names), end(names)));
    
}
