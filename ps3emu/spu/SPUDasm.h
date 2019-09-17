#pragma once

#include "../ppu/ppu_dasm.h"
#include "../BitField.h"
#include "ps3emu/int.h"

#define SYS_SPU_THREAD_STOP_YIELD 0x0100
#define SYS_SPU_THREAD_STOP_GROUP_EXIT 0x0101
#define SYS_SPU_THREAD_STOP_THREAD_EXIT 0x0102
#define SYS_SPU_THREAD_STOP_RECEIVE_EVENT 0x0110
#define SYS_SPU_THREAD_STOP_TRY_RECEIVE_EVENT 0x0111
#define SYS_SPU_THREAD_STOP_SWITCH_SYSTEM_MODULE 0x0120

using I16_t = BitField<9, 25, BitFieldType::Signed>;

struct SPUForm {
    uint32_t value;
    BIT_FIELD(OP4, 0, 4)
    BIT_FIELD(OP7, 0, 7)
    BIT_FIELD(OP6, 0, 6)
    BIT_FIELD(OP8, 0, 8)
    BIT_FIELD(OP9, 0, 9)
    BIT_FIELD(OP10, 0, 10)
    BIT_FIELD(OP11, 0, 11)
    BIT_FIELD(RB, 11, 18, BitFieldType::GPR)
    BIT_FIELD(RA, 18, 25, BitFieldType::GPR)
    BIT_FIELD(RC, 25, 32, BitFieldType::GPR)
    BIT_FIELD(RT, 25, 32, BitFieldType::GPR)
    BIT_FIELD(RT_ABC, 4, 11, BitFieldType::GPR)
    BIT_FIELD(CA, 18, 25)
    BIT_FIELD(I7, 11, 18, BitFieldType::Signed)
    BIT_FIELD(I8, 10, 18, BitFieldType::Signed)
    BIT_FIELD(I10, 8, 18, BitFieldType::Signed)
    BIT_FIELD(I16, 9, 25, BitFieldType::Signed)
    BIT_FIELD(I18, 7, 25, BitFieldType::Signed)
    BIT_FIELD(StopAndSignalType, 18, 32)
};

inline constexpr uint32_t SPU_STOPD_OPCODE = 0b00101000000;

template <DasmMode M, typename S>
void SPUDasm(const void* instr, uint32_t cia, S* state);
InstructionInfo analyzeSpu(uint32_t instr, uint32_t cia);
