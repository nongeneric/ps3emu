#pragma once

#include <vector>
#include <string>

std::string startWaitGetOutput(std::vector<std::string> args,
                               std::vector<std::string> ps3runArgs = {});
struct CompileInfo {
    std::string cpp;
    std::string so;
    bool debug;
    bool trace;
};

bool rewrite(std::string elf, std::string cpp, std::string args, std::string& output);
bool compile(CompileInfo const& info);
bool rewrite_and_compile(std::string elf);
