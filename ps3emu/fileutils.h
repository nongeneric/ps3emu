#pragma once

#include <vector>
#include <string>
#include <experimental/string_view>

std::vector<uint8_t> read_all_bytes(std::experimental::string_view path);
void write_all_bytes(const void* ptr, uint32_t size, std::experimental::string_view path);
std::string read_all_text(std::experimental::string_view path);
void write_all_text(std::experimental::string_view text, std::experimental::string_view path);
void write_all_lines(std::vector<std::string> lines, std::experimental::string_view path);
