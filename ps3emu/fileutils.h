#pragma once

#include <vector>
#include <string>
#include <string_view>

std::vector<uint8_t> read_all_bytes(std::string_view path);
void write_all_bytes(const void* ptr, uint32_t size, std::string_view path);
std::string read_all_text(std::string_view path);
void write_all_text(std::string_view text, std::string_view path);
void write_all_lines(std::vector<std::string> lines, std::string_view path);
std::vector<std::string> get_files(std::string_view path,
                                   std::string_view rxPattern = ".*?",
                                   bool recursive = false);
