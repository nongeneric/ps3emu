#pragma once

#include "ExecutionMap.h"
#include "InstrDb.h"
#include "ps3emu/constants.h"
#include "ps3emu/int.h"
#include <tuple>
#include <vector>
#include <atomic>

struct ExecutionMapCollection {
    std::atomic<bool> enabled;
    ExecutionMap<20u << 20u> ppu;
    std::vector<std::tuple<InstrDbEntry, ExecutionMap<LocalStorageSize>>> spu;
    ExecutionMapCollection();
};
