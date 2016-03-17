#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <array>

std::string GenerateFragmentShader(std::vector<uint8_t> const& bytecode,
                                   std::array<int, 16> const& samplerSizes,
                                   bool isFlatColorShading);

struct VertexShaderInputFormat {
    bool enabled = false;
    int typeSize = 4;
    int rank = 4;
    //int mask;
    // TODO: use dest_mask_t instead of int everywhere
};

uint32_t CalcVertexBytecodeSize(const uint8_t* bytecode);

std::string GenerateVertexShader(const uint8_t* bytecode, 
                                 std::array<VertexShaderInputFormat, 16> const& inputs,
                                 std::array<int, 4> const& samplerSizes,
                                 unsigned loadOffset,
                                 std::vector<unsigned>* usedConsts = nullptr);

std::string PrintFragmentProgram(const uint8_t* instr);
std::string PrintVertexProgram(const uint8_t* instr, bool verbose = false);