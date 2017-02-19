#pragma once

#include <vector>
#include <experimental/string_view>

std::vector<uint8_t> read_all_bytes(std::experimental::string_view path);
