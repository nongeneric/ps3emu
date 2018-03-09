#pragma once

#include "ps3emu/int.h"
#include "GLShader.h"
#include <unordered_map>
#include <memory>
#include <array>

struct FragmentShaderKey {
    uint64_t hash;
    uint32_t size;
    uint32_t constCount;
    bool mrt;
    bool operator==(const FragmentShaderKey& other) const {
        return hash == other.hash && size == other.size &&
               constCount == other.constCount && mrt == other.mrt;
    }
};

struct FragmentShaderKeyHash {
    inline uint64_t operator()(const FragmentShaderKey& key) const noexcept {
        return key.hash ^ key.size ^ key.constCount ^ (key.mrt * 277);
    }
};

class FragmentShaderCache {
    std::unordered_map<FragmentShaderKey, std::unique_ptr<FragmentShader>, FragmentShaderKeyHash> _cache;

public:
    FragmentShaderKey unzip(uint8_t* ptr, std::array<float, 4>* fconst, bool mrt);
    FragmentShader* retrieve(const FragmentShaderKey& key);
    void insert(const FragmentShaderKey& key, FragmentShader* shader);
};
