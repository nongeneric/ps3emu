#pragma once

#include <stdint.h>

typedef uint32_t ps3_uintptr_t;
typedef int emu_void_t;
typedef unsigned __int128 slow_uint128_t;
typedef __int128 slow_int128_t;
static constexpr emu_void_t emu_void = 0;

static constexpr ps3_uintptr_t EmuInternalArea = 0x10000000;
static constexpr ps3_uintptr_t EmuInternalAreaSize = 16ul << 20;

static constexpr ps3_uintptr_t HeapArea = 0x30000000;
static constexpr uint32_t HeapAreaSize = 0x10000000;

static constexpr ps3_uintptr_t GcmControlRegisters = 0x40100040;
static constexpr ps3_uintptr_t GcmLabelBaseOffset = 0x40300000;
static constexpr ps3_uintptr_t RsxFbBaseAddr = 0xc0000000;

static constexpr ps3_uintptr_t RawSpuBaseAddr = 0xe0000000;
static constexpr ps3_uintptr_t RawSpuProblemOffset = 0x00040000;
static constexpr ps3_uintptr_t RawSpuOffset = 0x00100000;

static constexpr ps3_uintptr_t SpuThreadBaseAddr = 0xf0000000;
static constexpr ps3_uintptr_t SpuThreadOffsetAddr = 0x100000;
static constexpr ps3_uintptr_t SpuThreadSnr1 = 0x05400c;
static constexpr ps3_uintptr_t SpuThreadSnr2 = 0x05c00c;

static constexpr uint32_t GcmLocalMemorySize = 0xf900000;
static constexpr uint32_t RsxMemoryFrequency = 650000000;
static constexpr uint32_t RsxCoreFrequency = 500000000;

static constexpr ps3_uintptr_t StackArea = 0xd0000000u;
static constexpr uint32_t StackAreaSize = 32u << 20;

static constexpr uint32_t DefaultStackSize = 0x10000u;

static constexpr uint32_t DefaultMainMemoryPageBits = 12;
static constexpr ps3_uintptr_t MaxMainMemoryAddress = 0xffffffff;
static constexpr uint32_t DefaultMainMemoryOffsetBits = 32 - DefaultMainMemoryPageBits;
static constexpr uint32_t DefaultMainMemoryPageSize = 1u << DefaultMainMemoryOffsetBits;
static constexpr uint32_t DefaultMainMemoryPageCount = MaxMainMemoryAddress / DefaultMainMemoryPageSize;

static constexpr unsigned VertexShaderConstantCount = 468;
static constexpr unsigned FragmentShaderConstantCount = 512 / 2;

static constexpr unsigned VertexTextureUnit = 1;
static constexpr unsigned FragmentTextureUnit = 5;

static constexpr uint32_t LocalStorageSize = 256 * 1024;

enum Bindings {
    VertexShaderConstantBinding = 17,
    FragmentShaderConstantBinding,
    VertexShaderSamplesInfoBinding,
    FragmentShaderSamplesInfoBinding,
    VertexShaderViewportMatrixBinding,
};

static constexpr unsigned TransformFeedbackBufferBinding = 0;

static constexpr unsigned MutexIdBase = 0x1000;
static constexpr unsigned CondIdBase = 0x2000;
static constexpr unsigned LwMutexIdBase = 0x3000;
static constexpr unsigned LwCondIdBase = 0x4000;
static constexpr unsigned QueueIdBase = 0x5000;
static constexpr unsigned EventFlagIdBase = 0x6000;
