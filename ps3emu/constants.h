#pragma once

#include <stdint.h>

typedef uint32_t ps3_uintptr_t;
typedef int emu_void_t;
static constexpr emu_void_t emu_void = 0;

static constexpr ps3_uintptr_t GcmControlRegisters = 0x40100040;
static constexpr ps3_uintptr_t GcmLabelBaseOffset = 0x40300000;
static constexpr ps3_uintptr_t GcmLocalMemoryBase = 0xc0000000;
static constexpr ps3_uintptr_t GcmLocalMemorySize = 0xf900000;
static constexpr uint32_t RsxMemoryFrequency = 650000000;
static constexpr uint32_t RsxCoreFrequency = 500000000;
static constexpr ps3_uintptr_t StackBase = 0xd0000000u;
static constexpr uint32_t StackSize = 0x10000u;

static constexpr ps3_uintptr_t DefaultGcmBufferOverflowCallback = 0x7f000000;
static constexpr ps3_uintptr_t DefaultGcmDefaultContextData = DefaultGcmBufferOverflowCallback + 8;
static constexpr ps3_uintptr_t FunctionDescriptorsVa = DefaultGcmDefaultContextData + 16;

static constexpr uint32_t DefaultMainMemoryPageBits = 12;
static constexpr ps3_uintptr_t MaxMainMemoryAddress = 0xffffffff;
static constexpr uint32_t DefaultMainMemoryOffsetBits = 32 - DefaultMainMemoryPageBits;
static constexpr uint32_t DefaultMainMemoryPageSize = 1u << DefaultMainMemoryOffsetBits;
static constexpr uint32_t DefaultMainMemoryPageCount = MaxMainMemoryAddress / DefaultMainMemoryPageSize;

static constexpr unsigned VertexShaderConstantCount = 468;
static constexpr unsigned VertexShaderConstantBinding = 16;
static constexpr unsigned VertexShaderSamplesInfoBinding = 17;
static constexpr unsigned FragmentShaderSamplesInfoBinding = 18;