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
