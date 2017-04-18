#pragma once

#include "Rewriter.h"
#include <vector>

std::vector<std::vector<BasicBlock>> partitionBasicBlocks(
    std::vector<BasicBlock> const& blocks, analyze_t analyze);
