#pragma once

#include "ps3emu/int.h"

inline uint64_t asm_sar(uint64_t x, uint64_t sh) {
    __asm__ (
        "sarq %%rax, %%rcx\n"
        : "=c" (x)
        : "c" (x), "a" (sh)
    );
    return x;
}
