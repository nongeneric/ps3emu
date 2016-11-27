#pragma once

#include <vector>
#include <string>

std::string startWaitGetOutput(std::vector<std::string> args,
                               std::vector<std::string> ps3runArgs = {});
bool rewrite_and_compile(std::string elf);
