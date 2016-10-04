#pragma once

#include "ps3emu/libs/graphics/gcm.h"
#include "ps3emu/utils.h"
#include <boost/optional.hpp>
#include <memory>
#include <vector>

template<typename T>
struct CachedTextureResult {
    T* texture;
    float xOffset;
    float yOffset;
    float xScale;
    float yScale;
};

template<typename T>
class TextureCache {
    struct CachedTextureEntry {
        std::unique_ptr<T> texture;
        uint32_t va;
        MemoryLocation location;
        bool dirty;
    };
    
    std::vector<CachedTextureEntry> _cache;
    
public:
    boost::optional<CachedTextureResult<T>> get(MemoryLocation location,
                                                uint32_t va,
                                                uint32_t pitch,
                                                uint32_t height,
                                                uint32_t format,
                                                std::function<T*()> createTexture) {
        auto size = pitch * height;
        for (auto i = (int)_cache.size() - 1; i >= 0; i--) {
            auto& cached = _cache[i];
            auto cachedSize =
                cached.texture->info().height * cached.texture->info().pitch;
            auto sameLocation = cached.location == location;
            auto sameFormat = cached.texture->info().format == format;
            auto isSubset = subset(va, size, cached.va, cachedSize);
            if (sameLocation && sameFormat && isSubset && !cached.dirty) {
                return CachedTextureResult<T>{cached.texture.get(), 0, 0, 0, 0};
            }
            if (cached.dirty || intersects(va, size, cached.va, cachedSize)) {
                _cache.erase(begin(_cache) + i);
            }
        }
    
        CachedTextureEntry entry{
            std::unique_ptr<T>(createTexture()), va, location, false};
        _cache.push_back(std::move(entry));
        return CachedTextureResult<T>{_cache.back().texture.get(), 0, 0, 0, 0};
    }
    
    void invalidate(MemoryLocation location, uint32_t va, uint32_t size) {
        for (auto& entry : _cache) {
            if (entry.location == location &&
                intersects(va,
                           size,
                           entry.va,
                           entry.texture->info().height * entry.texture->info().pitch)) {
                entry.dirty = true;
            }
        }
    }
};
