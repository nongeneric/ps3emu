#include "state.h"

#include "ps3emu/EmuCallbacks.h"
#include "ps3emu/Config.h"
#include "ps3emu/execmap/ExecutionMapCollection.h"
#include "ps3emu/BBCallMap.h"

g_state_t::g_state_t()
    : proc(nullptr),
      mm(nullptr),
      memalloc(nullptr),
      heapalloc(nullptr),
      content(nullptr),
      rsx(nullptr),
      elf(nullptr),
      callbacks(new EmuCallbacks()),
      config(new Config()),
      executionMaps(new ExecutionMapCollection()),
      bbcallMap(new BBCallMap()) {}

g_state_t g_state;
thread_local PPUThread* g_state_t::th = nullptr;
thread_local SPUThread* g_state_t::sth = nullptr;
thread_local bool g_state_t::rewriter_ncall = false;
thread_local ReservationGranule* g_state_t::granule = nullptr;
