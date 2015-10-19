#pragma once

#include <string>
#include <cstdint>
#include <vector>

std::string GenerateShader(std::vector<uint8_t> const& bytecode);
