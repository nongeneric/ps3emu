#pragma once

#include "ps3emu/int.h"
#include <vector>
#include <string>
#include <array>
#include <memory>
#include <atomic>

class ExecutionMap {
    constexpr static uint32_t size = 20u << 20u;
    std::unique_ptr<std::array<std::atomic<bool>, size>> _map;
    
public:
    ExecutionMap();
    void mark(uint32_t va);
    std::vector<uint32_t> dump(uint32_t start, uint32_t len);
};

std::string serializeEntries(std::vector<uint32_t> const& vec);
std::vector<uint32_t> deserializeEntries(std::string const& text);
