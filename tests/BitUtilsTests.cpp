#include "ps3emu/int.h"
#include <catch.hpp>
#include <vector>

TEST_CASE("fast_reverse_tests") {
    std::vector<uint8_t> bytes {
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15
    };
    auto vec = bytes;
    *(uint128_t*)&vec[0] = fast_endian_reverse(*(uint128_t*)&vec[0]);
    REQUIRE((vec == std::vector<uint8_t>{ 15, 14, 13, 12, 11, 10, 9,
                                          8, 7, 6, 5, 4, 3, 2, 1, 0}));
    vec = bytes;
    *(uint64_t*)&vec[0] = fast_endian_reverse(*(uint64_t*)&vec[0]);
    REQUIRE((vec == std::vector<uint8_t>{ 7, 6, 5, 4, 3, 2, 1, 0, 8,
                                          9, 10, 11, 12, 13, 14, 15}));
    vec = bytes;
    *(uint32_t*)&vec[0] = fast_endian_reverse(*(uint32_t*)&vec[0]);
    REQUIRE((vec == std::vector<uint8_t>{ 3, 2, 1, 0, 4, 5, 6, 7, 8,
                                          9, 10, 11, 12, 13, 14, 15}));
    vec = bytes;
    *(uint16_t*)&vec[0] = fast_endian_reverse(*(uint16_t*)&vec[0]);
    REQUIRE((vec == std::vector<uint8_t>{ 1, 0, 2, 3, 4, 5, 6, 7, 8,
                                          9, 10, 11, 12, 13, 14, 15}));
    vec = bytes;
    *(uint8_t*)&vec[0] = fast_endian_reverse(*(uint8_t*)&vec[0]);
    REQUIRE((vec == std::vector<uint8_t>{ 0, 1, 2, 3, 4, 5, 6, 7, 8,
                                          9, 10, 11, 12, 13, 14, 15}));
}

TEST_CASE("fast_reverse_range_2") {
    std::vector<uint16_t> initial(100);
    for (auto i = 0u; i < initial.size(); ++i) {
        initial[i] = (i + 13) * 2789;
    }

    auto vec = initial;

    auto validate = [&](unsigned count) {
        for (auto i = 0u; i < count; ++i) {
            if (vec[i] != fast_endian_reverse(initial[i]))
                return false;
        }
        for (auto i = count; i < vec.size(); ++i) {
            if (vec[i] != initial[i])
                return false;
        }
        return true;
    };

    for (int i = 0; i < 10; ++i) {
        vec = initial;
        fast_endian_reverse<2>(&vec[0], &vec[0], i);
        REQUIRE( validate(i) );
    }

    vec = initial;
    fast_endian_reverse<2>(&vec[0], &vec[0], 99);
    REQUIRE( validate(99) );
}

TEST_CASE("fast_reverse_range_4") {
    std::vector<uint32_t> initial(100);
    for (auto i = 0u; i < initial.size(); ++i) {
        initial[i] = (i + 13) * 11022789;
    }

    auto vec = initial;

    auto validate = [&](unsigned count) {
        for (auto i = 0u; i < count; ++i) {
            if (vec[i] != fast_endian_reverse(initial[i]))
                return false;
        }
        for (auto i = count; i < vec.size(); ++i) {
            if (vec[i] != initial[i])
                return false;
        }
        return true;
    };

    for (int i = 0; i < 10; ++i) {
        vec = initial;
        fast_endian_reverse<4>(&vec[0], &vec[0], i);
        REQUIRE( validate(i) );
    }

    vec = initial;
    fast_endian_reverse<4>(&vec[0], &vec[0], 99);
    REQUIRE( validate(99) );
}
