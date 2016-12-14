#include <catch.hpp>
//#include "ps3emu/rsx/TextureCache.h"

struct TestTextureInfo {
    uint32_t format;
    uint32_t pitch;
    uint32_t height;
};

struct TestTexture {
    TestTextureInfo _info;
    TestTextureInfo info() {
        return _info;
    }
};

// TEST_CASE("texture_cache_basic") {
//     TextureCache<TestTexture> cache;
//     int createCalled = 0;
//     auto create = [&] {
//         createCalled++;
//         return new TestTexture({0x13, 1024, 200});
//     };
//     auto res = cache.get(MemoryLocation::Local, 0x10000, 1024, 200, 0x13, create);
//     REQUIRE(createCalled == 1);
//     REQUIRE(res);
//     REQUIRE(res->xOffset == 0.f);
//     REQUIRE(res->yOffset == 0.f);
//     REQUIRE(res->xScale == 1.f);
//     REQUIRE(res->yScale == 1.f);
// 
//     res = cache.get(MemoryLocation::Local, 0x10000, 1024, 50, 0x13, create);
//     REQUIRE(createCalled == 1);
//     REQUIRE(res);
//     REQUIRE(res->xOffset == 0.f);
//     REQUIRE(res->yOffset == 0.f);
//     REQUIRE(res->xScale == 0.25f);
//     REQUIRE(res->yScale == 0.25f);
// }
