#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <array>

std::string GenerateFragmentShader(std::vector<uint8_t> const& bytecode);

struct VertexShaderInputFormat {
    int typeSize = 4;
    int rank = 4;
    //int mask;
    // TODO: use dest_mask_t instead of int everywhere
};

std::string GenerateVertexShader(const uint8_t* bytecode, 
                                 std::array<VertexShaderInputFormat, 16> const& inputs);
