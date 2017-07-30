#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <array>
#include "ps3emu/enum.h"

std::string GenerateFragmentShader(std::vector<uint8_t> const& bytecode,
                                   std::array<int, 16> const& samplerSizes,
                                   bool isFlatColorShading,
                                   bool isMrt);

ENUM(VertexInputType,
    (s1, 1),
    (f32, 2),
    (f16, 3),
    (u8, 4),
    (s16, 5),
    (x11y11z10n, 6),
    (u16, 1000), // gcmviz index array
    (u32, 1001)  // gcmviz index array
)

struct VertexShaderInputFormat {
    VertexInputType type = VertexInputType::f32;
    int rank = 4;
    //int mask;
    // TODO: use dest_mask_t instead of int everywhere
};

uint32_t CalcVertexBytecodeSize(const uint8_t* bytecode);

std::string GenerateVertexShader(const uint8_t* bytecode, 
                                 std::array<VertexShaderInputFormat, 16> const& inputs,
                                 std::array<int, 4> const& samplerSizes,
                                 unsigned loadOffset,
                                 std::vector<unsigned>* usedConsts = nullptr,
                                 bool feedback = false);

std::string PrintFragmentProgram(const uint8_t* instr);
std::string PrintVertexProgram(const uint8_t* instr, bool verbose = false);
std::string PrintFragmentBytecode(const uint8_t* bytecode);
std::string PrintVertexBytecode(const uint8_t* bytecode);
