#pragma once

#include <stdint.h>

typedef uint32_t ps3_uintptr_t;
typedef int emu_void_t;
constexpr emu_void_t emu_void = 0;

constexpr ps3_uintptr_t GcmControlRegisters = 0x40100040;
constexpr ps3_uintptr_t GcmLabelBaseOffset = 0x40300000;
constexpr ps3_uintptr_t GcmLocalMemoryBase = 0xc0000000;
constexpr ps3_uintptr_t GcmLocalMemorySize = 0xf900000;
constexpr uint32_t RsxMemoryFrequency = 650000000;
constexpr uint32_t RsxCoreFrequency = 500000000;
constexpr ps3_uintptr_t StackBase = 0xd0000000u;
constexpr uint32_t StackSize = 0x10000u;

constexpr uint32_t DefaultMainMemoryPageBits = 12;
constexpr ps3_uintptr_t MaxMainMemoryAddress = 0xffffffff;
constexpr uint32_t DefaultMainMemoryOffsetBits = 32 - DefaultMainMemoryPageBits;
constexpr uint32_t DefaultMainMemoryPageSize = 1u << DefaultMainMemoryOffsetBits;
constexpr uint32_t DefaultMainMemoryPageCount = MaxMainMemoryAddress / DefaultMainMemoryPageSize;