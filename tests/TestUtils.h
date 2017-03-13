#pragma once

#include <vector>
#include <string>

std::string startWaitGetOutput(std::vector<std::string> args,
                               std::vector<std::string> ps3runArgs = {});
void test_interpreter_and_rewriter(std::vector<std::string> args, std::string output);
