#pragma once

#include <assert.h>

#ifdef DEBUGPAUSE
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/utils.h"
#include "ps3emu/log.h"
#define EMU_ASSERT(expr)                                                            \
    if (!(expr)) {                                                                  \
        ERROR(libs) << ssnprintf("assert %s at %s, %s:%d\n",                        \
                                 #expr,                                             \
                                 __PRETTY_FUNCTION__,                               \
                                 __FILE__,                                          \
                                 __LINE__);                                         \
        throw BreakpointException();                                                \
    }
#else
#define EMU_ASSERT(expr) assert(expr)
#endif

#ifndef NDEBUG
#define HARD_ASSERT(expr) assert(expr)
#else
#define HARD_ASSERT(expr) (void)(expr)
#endif
