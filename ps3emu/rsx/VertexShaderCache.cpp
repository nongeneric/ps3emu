#include "VertexShaderCache.h"
#include "xxhash.h"

VertexShader* VertexShaderCache::retrieve(const uint8_t* from,
                                          uint32_t size,
                                          std::array<uint8_t, 16> ranks) {
    VertexShaderKey key { XXH64(from, size, 0), ranks };
    auto it = _cache.find(key);
    if (it == end(_cache))
        return nullptr;
    return it->second.get();
}

void VertexShaderCache::insert(const uint8_t* from,
                               uint32_t size,
                               std::array<uint8_t, 16> ranks,
                               VertexShader* shader) {
    VertexShaderKey key { XXH64(from, size, 0), ranks };
    _cache[key].reset(shader);
}
