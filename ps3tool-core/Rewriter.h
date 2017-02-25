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

inline bool operator<(BasicBlock const& left, BasicBlock const& right) {
    return std::tie(left.start, left.len) < std::tie(right.start, right.len);
}

struct EmbeddedElfInfo {
    uint32_t startOffset = 0;
    uint32_t size = 0;
    bool isJob = false;
    const Elf32_be_Ehdr* header = nullptr;
};

std::vector<BasicBlock> discoverBasicBlocks(
    uint32_t start,
    uint32_t length,
    uint32_t imageBase,
    std::stack<uint32_t> leads,
    std::ostream& log,
    std::function<InstructionInfo(uint32_t)> analyze,
    std::vector<uint32_t>* branches = nullptr);

std::vector<EmbeddedElfInfo> discoverEmbeddedSpuElfs(std::vector<uint8_t> const& elf);
uint32_t vaToOffset(const Elf32_be_Ehdr* header, uint32_t va);
