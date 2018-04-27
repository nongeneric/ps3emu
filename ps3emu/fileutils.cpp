#include "fileutils.h"

#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <regex>
#include <stdio.h>
#include <stdexcept>

using namespace std::filesystem;

namespace {
  
    template <typename Iterator, bool Files>
    std::vector<std::string> get_files(std::string_view path,
                                std::string_view rxPattern) {
        std::regex rx(begin(rxPattern));
        std::vector<std::string> res;
        for (Iterator i(begin(path)); i != Iterator(); ++i) {
            if (Files && !is_regular_file(i->status()))
                continue;
            if (!Files && !is_directory(i->status()))
                continue;
            std::cmatch m;
            if (!std::regex_match(i->path().filename().string().c_str(), m, rx))
                continue;
            res.push_back(i->path().string());
        }
        return res;
    }
    
};

std::vector<uint8_t> read_all_bytes(std::string_view path) {
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

void write_all_bytes(const void* ptr, uint32_t size, std::string_view path) {
    auto f = fopen(begin(path), "w");
    if (!f)
        throw std::runtime_error("can't open file");
    auto res = fwrite(ptr, 1, size, f);
    if (res != size)
        throw std::runtime_error("incomplete write");
    fclose(f);
}

std::string read_all_text(std::string_view path) {
    auto vec = read_all_bytes(path);
    return std::string((const char*)&vec[0], vec.size());
}

void write_all_text(std::string_view text, std::string_view path) {
    write_all_bytes(&text[0], text.size(), path);
}

void write_all_lines(std::vector<std::string> lines, std::string_view path) {
    auto str = boost::algorithm::join(lines, "\n");
    write_all_text(str, path);
}

std::vector<std::string> get_files(std::string_view path,
                                   std::string_view rxPattern,
                                   bool recursive) {
    if (recursive) {
        return get_files<directory_iterator, true>(path, rxPattern);
    } else {
        return get_files<recursive_directory_iterator, true>(path, rxPattern);
    }
}
