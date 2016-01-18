#pragma once

#include "../ppu_dasm.h"

template <DasmMode M, typename S>
void SPUDasm(void* instr, uint32_t cia, S* state);