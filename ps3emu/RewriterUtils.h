#pragma once

#include <string>

struct CompileInfo {
    std::string cpp;
    std::string so;
    bool debug;
    bool trace;
};

bool rewrite(std::string elf, std::string cpp, std::string args, std::string& output);
bool compile(CompileInfo const& info);
