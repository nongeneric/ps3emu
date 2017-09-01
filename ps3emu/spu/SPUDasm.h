#pragma once

#include "../ppu/ppu_dasm.h"

#define SYS_SPU_THREAD_STOP_YIELD 0x0100
#define SYS_SPU_THREAD_STOP_GROUP_EXIT 0x0101
#define SYS_SPU_THREAD_STOP_THREAD_EXIT 0x0102
#define SYS_SPU_THREAD_STOP_RECEIVE_EVENT 0x0110
#define SYS_SPU_THREAD_STOP_TRY_RECEIVE_EVENT 0x0111
#define SYS_SPU_THREAD_STOP_SWITCH_SYSTEM_MODULE 0x0120

template <DasmMode M, typename S>
void SPUDasm(const void* instr, uint32_t cia, S* state);
InstructionInfo analyzeSpu(uint32_t instr, uint32_t cia);
