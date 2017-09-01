#pragma once

#include "ps3emu/libs/spu/SpuImage.h"
#include "ps3emu/dasm_utils.h"
#include <stdint.h>
#include <optional>
#include <set>
#include <vector>
#include <functional>
#include <ostream>
#include <stack>

using analyze_t = std::function<InstructionInfo(uint32_t)>;
using name_t = std::function<std::string(uint32_t)>;
using read_t = std::function<uint32_t(uint32_t)>;
using validate_t = std::function<bool(uint32_t)>;

struct BasicBlock {
    uint32_t start = 0;
    uint32_t len = 0;
    std::vector<std::string> body;
    bool operator<(BasicBlock const& other) {
        return start < other.start;
    }
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
    std::set<uint32_t> leads,
    std::ostream& log,
    analyze_t analyze,
    validate_t validate,
    std::optional<name_t> name = {},
    std::optional<read_t> read = {},
    bool confidence = false);

std::vector<EmbeddedElfInfo> discoverEmbeddedSpuElfs(std::vector<uint8_t> const& elf);
uint32_t vaToOffset(const Elf32_be_Ehdr* header, uint32_t va);
std::set<uint32_t> discoverIndirectLocations(uint32_t start,
                                             uint32_t len,
                                             analyze_t analyze,
                                             name_t name);
std::tuple<uint32_t, uint32_t> discoverJumpTable(uint32_t start,
                                                 uint32_t len,
                                                 uint32_t segmentStart,
                                                 uint32_t segmentLen,
                                                 analyze_t analyze,
                                                 name_t name,
                                                 read_t read);
