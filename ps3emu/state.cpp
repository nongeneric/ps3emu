#include "state.h"

#include "EmuCallbacks.h"
#include "Config.h"

g_state_t::g_state_t()
    : proc(nullptr),
      mm(nullptr),
      memalloc(nullptr),
      heapalloc(nullptr),
      content(nullptr),
      rsx(nullptr),
      elf(nullptr),
      callbacks(new EmuCallbacks()),
      config(new Config()) {}

g_state_t g_state;
thread_local PPUThread* g_state_t::th = nullptr;
thread_local SPUThread* g_state_t::sth = nullptr;
thread_local bool g_state_t::rewriter_ncall = false;
thread_local ReservationGranule* g_state_t::granule = nullptr;
