#pragma once

#include <string>

constexpr bool static_debug =
#if !defined(NDEBUG) || defined(TESTS)
    true
#else
    false
#endif
    ;

std::string ps3toolPath();
std::string rewrite(std::string elf, std::string cpp, std::string args);
std::string compileRule();
std::string linkRule();
std::string compile(std::string ninja);
bool exec(std::string line, std::string& output);
