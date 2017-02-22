#pragma once

#include <vector>
#include <experimental/string_view>

std::vector<uint8_t> read_all_bytes(std::experimental::string_view path);
void write_all_bytes(void* ptr, uint32_t size, std::experimental::string_view path);
