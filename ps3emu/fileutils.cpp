#include "fileutils.h"

#include <stdio.h>
#include <stdexcept>

std::vector<uint8_t> read_all_bytes(std::experimental::string_view path) {
    auto f = fopen(begin(path), "r");
    if (!f)
        throw std::runtime_error("can't open file");
    fseek(f, 0, SEEK_END);
    auto filesize = ftell(f);
    std::vector<uint8_t> res(filesize);
    fseek(f, 0, SEEK_SET);
    fread(&res[0], 1, res.size(), f);
    fclose(f);
    return res;
}

void write_all_bytes(const void* ptr, uint32_t size, std::experimental::string_view path) {
    auto f = fopen(begin(path), "w");
    if (!f)
        throw std::runtime_error("can't open file");
    auto res = fwrite(ptr, 1, size, f);
    if (res != size)
        throw std::runtime_error("incomplete write");
    fclose(f);
}

std::string read_all_text(std::experimental::string_view path) {
    auto vec = read_all_bytes(path);
    return std::string((const char*)&vec[0], vec.size());
}

void write_all_text(std::experimental::string_view text, std::experimental::string_view path) {
    write_all_bytes(&text[0], text.size(), path);
}
