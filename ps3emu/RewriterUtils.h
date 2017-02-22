#pragma once

#include <string>

struct CompileInfo {
    std::string cpp;
    std::string so;
    bool debug;
    bool trace;
};

std::string rewrite(std::string elf, std::string cpp, std::string args);
std::string compile(CompileInfo const& info);
bool exec(std::string line, std::string& output);
