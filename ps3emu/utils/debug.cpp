#include "debug.h"

#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/utils.h"
#include "ps3emu/log.h"

void emu_assert_failure(const char* expr,
                        const char* prettyFunc,
                        const char* file,
                        int line) {
    ERROR(libs) << ssnprintf(
        "assert %s at %s, %s:%d\n", expr, prettyFunc, file, line);
    throw BreakpointException();
}
