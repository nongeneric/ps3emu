#pragma once

#include "ps3emu/libs/spu/SpuImage.h"
#include "ps3emu/dasm_utils.h"
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

struct EmbeddedElfInfo {
    uint32_t startOffset;
    uint32_t size;
    const Elf32_be_Ehdr* header;
};

std::vector<BasicBlock> discoverBasicBlocks(
    uint32_t start,
    uint32_t length,
    uint32_t imageBase,
    std::stack<uint32_t> leads,
    std::ostream& log,
    std::function<InstructionInfo(uint32_t)> analyze);

std::vector<EmbeddedElfInfo> discoverEmbeddedSpuElfs(std::vector<uint8_t> const& elf);
