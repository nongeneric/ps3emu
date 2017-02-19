#pragma once

#include "../ppu/ppu_dasm.h"

template <DasmMode M, typename S>
void SPUDasm(void* instr, uint32_t cia, S* state);
InstructionInfo analyzeSpu(uint32_t instr, uint32_t cia);
