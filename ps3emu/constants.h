#pragma once

#include <stdint.h>

typedef uint32_t ps3_uintptr_t;
typedef int emu_void_t;
typedef unsigned __int128 uint128_t;
typedef __int128 int128_t;
static constexpr emu_void_t emu_void = 0;

static constexpr ps3_uintptr_t EmuInternalArea = 0x100000;
static constexpr ps3_uintptr_t EmuInternalAreaSize = 0x1000000;

static constexpr ps3_uintptr_t HeapArea = 0x30000000;
static constexpr uint32_t HeapAreaSize = 0x10000000;

static constexpr ps3_uintptr_t GcmControlRegisters = 0x40100040;
static constexpr ps3_uintptr_t GcmLabelBaseOffset = 0x40300000;
static constexpr ps3_uintptr_t RsxFbBaseAddr = 0xc0000000;

static constexpr ps3_uintptr_t RawSpuBaseAddr = 0xe0000000;
static constexpr ps3_uintptr_t RawSpuProblemOffset = 0x00040000;
static constexpr ps3_uintptr_t RawSpuOffset = 0x00100000;

static constexpr uint32_t GcmLocalMemorySize = 0xf900000;
static constexpr uint32_t RsxMemoryFrequency = 650000000;
static constexpr uint32_t RsxCoreFrequency = 500000000;

static constexpr ps3_uintptr_t StackArea = 0xd0000000u;
static constexpr uint32_t StackAreaSize = 0x8000000u;

static constexpr ps3_uintptr_t TLSArea = StackArea + StackAreaSize;
static constexpr uint32_t TLSAreaSize = 0x7000000u;

static constexpr uint32_t DefaultStackSize = 0x10000u;

static constexpr ps3_uintptr_t FunctionDescriptorsVa = 0x7f000000;

static constexpr uint32_t DefaultMainMemoryPageBits = 12;
static constexpr ps3_uintptr_t MaxMainMemoryAddress = 0xffffffff;
static constexpr uint32_t DefaultMainMemoryOffsetBits = 32 - DefaultMainMemoryPageBits;
static constexpr uint32_t DefaultMainMemoryPageSize = 1u << DefaultMainMemoryOffsetBits;
static constexpr uint32_t DefaultMainMemoryPageCount = MaxMainMemoryAddress / DefaultMainMemoryPageSize;

static constexpr unsigned VertexShaderConstantCount = 468;
static constexpr unsigned FragmentShaderConstantCount = 512 / 2;

static constexpr unsigned VertexTextureUnit = 1;
static constexpr unsigned FragmentTextureUnit = 5;

static constexpr unsigned VertexShaderConstantBinding = 1;
static constexpr unsigned FragmentShaderConstantBinding = 2;
static constexpr unsigned VertexShaderSamplesInfoBinding = 3;
static constexpr unsigned FragmentShaderSamplesInfoBinding = 4;
static constexpr unsigned VertexShaderViewportMatrixBinding = 5;