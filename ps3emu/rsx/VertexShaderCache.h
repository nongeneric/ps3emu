#pragma once

#include "ps3emu/int.h"
#include "GLShader.h"
#include <unordered_map>
#include <memory>

struct VertexShaderKey {
    uint64_t hash;
    std::array<uint8_t, 16> ranks;
    bool operator==(const VertexShaderKey& other) const {
        return hash == other.hash && ranks == other.ranks;
    }
};

struct VertexShaderKeyHash {
    inline uint64_t operator()(const VertexShaderKey& key) const noexcept {
        auto ranks = (uint64_t*)&key.ranks;
        return key.hash ^ ranks[0] ^ ranks[1];
    }
};

class VertexShaderCache {
    std::unordered_map<VertexShaderKey, std::unique_ptr<VertexShader>, VertexShaderKeyHash> _cache;

public:
    VertexShader* retrieve(const uint8_t* from, uint32_t size, std::array<uint8_t, 16> ranks);
    void insert(const uint8_t* from,
                uint32_t size,
                std::array<uint8_t, 16> ranks,
                VertexShader* shader);
};
