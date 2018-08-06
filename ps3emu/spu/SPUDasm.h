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

union SPUForm {
    uint32_t u;
    BitField<0, 4> OP4;
    BitField<0, 7> OP7;
    BitField<0, 6> OP6;
    BitField<0, 8> OP8;
    BitField<0, 9> OP9;
    BitField<0, 10> OP10;
    BitField<0, 11> OP11;
    BitField<11, 18, BitFieldType::GPR> RB;
    BitField<18, 25, BitFieldType::GPR> RA;
    BitField<25, 32, BitFieldType::GPR> RC;
    BitField<25, 32, BitFieldType::GPR> RT;
    BitField<4, 11, BitFieldType::GPR> RT_ABC;
    BitField<18, 25> CA;
    BitField<11, 18, BitFieldType::Signed> I7;
    BitField<10, 18, BitFieldType::Signed> I8;
    BitField<8, 18, BitFieldType::Signed> I10;
    I16_t I16;
    BitField<7, 25, BitFieldType::Signed> I18;
    BitField<18, 32> StopAndSignalType;
};

inline constexpr uint32_t SPU_STOPD_OPCODE = 0b00101000000;

template <DasmMode M, typename S>
void SPUDasm(const void* instr, uint32_t cia, S* state);
InstructionInfo analyzeSpu(uint32_t instr, uint32_t cia);
