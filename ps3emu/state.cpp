#include "state.h"

#include "EmuCallbacks.h"

g_state_t::g_state_t()
    : proc(nullptr),
      mm(nullptr),
      memalloc(nullptr),
      heapalloc(nullptr),
      content(nullptr),
      rsx(nullptr),
      elf(nullptr),
      callbacks(new EmuCallbacks()) {}

g_state_t g_state;
thread_local PPUThread* g_state_t::th = nullptr;
