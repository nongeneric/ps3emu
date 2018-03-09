#pragma once

#ifdef DEBUGPAUSE
void emu_assert_failure(const char* expr,
                        const char* prettyFunc,
                        const char* file,
                        int line);
#define EMU_ASSERT(expr)                                                            \
    if (!(expr)) {                                                                  \
        emu_assert_failure(#expr, __PRETTY_FUNCTION__, __FILE__, __LINE__);         \
    }
#else
#include <assert.h>
#define EMU_ASSERT(expr) assert(expr)
#endif

#ifndef NDEBUG
#include <assert.h>
#define HARD_ASSERT(expr) assert(expr)
#else
#define HARD_ASSERT(expr) (void)(expr)
#endif
