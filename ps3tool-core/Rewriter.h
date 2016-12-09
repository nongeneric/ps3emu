#pragma once

#include <stdint.h>
#include <vector>
#include <functional>
#include <ostream>
#include <stack>

struct BasicBlock {
    uint32_t start = 0;
    uint32_t len = 0;
    std::vector<std::string> body;
};

std::vector<BasicBlock> discoverBasicBlocks(
    uint32_t start,
    uint32_t length,
    uint32_t imageBase,
    std::stack<uint32_t> leads,
    std::ostream& log,
    std::function<uint32_t(uint32_t)> readInstr);
