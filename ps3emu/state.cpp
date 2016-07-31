#include "state.h"

g_state_t::g_state_t()
    : proc(nullptr),
      mm(nullptr),
      memalloc(nullptr),
      content(nullptr),
      rsx(nullptr),
      elf(nullptr) {}

g_state_t g_state;
