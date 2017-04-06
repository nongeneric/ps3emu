#pragma once

#include <string>

constexpr bool static_debug =
#if !defined(NDEBUG) || defined(TESTS)
    true
#else
    false
#endif
    ;

struct CompileInfo {
    std::string cpp;
    std::string so;
    bool debug;
    bool trace;
};

std::string ps3toolPath();
std::string rewrite(std::string elf, std::string cpp, std::string args);
std::string compileRule();
std::string compile(CompileInfo const& info);
bool exec(std::string line, std::string& output);
