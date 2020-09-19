#include "state.h"

#include "ps3emu/Config.h"
#include "ps3emu/execmap/ExecutionMapCollection.h"
#include "ps3emu/BBCallMap.h"
#include "ps3emu/spu/SPUGroupManager.h"

g_state_t g_state;
thread_local PPUThread* g_state_t::th = nullptr;
thread_local SPUThread* g_state_t::sth = nullptr;
thread_local bool g_state_t::rewriter_ncall = false;
thread_local ReservationGranule* g_state_t::granule = nullptr;

void g_state_t::init() {
    if (config)
        return;
    config = std::make_unique<Config>();
    executionMaps = std::make_unique<ExecutionMapCollection>();
    bbcallMap = std::make_unique<BBCallMap>();
    spuGroupManager = std::make_unique<SPUGroupManager>();
}

g_state_t::~g_state_t() {}