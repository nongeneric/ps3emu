#pragma once

#include "../sys.h"
#include "../../spu/SPUThread.h"
#include <string>
#include <array>
#include <functional>
#include <stdint.h>

class SpuImage {
    std::array<uint8_t, LocalStorageSize> _ls;
    std::string _desc;
    uint32_t _ep;
    uint32_t _src;

public:
    SpuImage(std::function<void(uint32_t, void*, size_t)> read, ps3_uintptr_t src);
    uint8_t* localStorage();
    uint32_t entryPoint();
    uint32_t source();
};