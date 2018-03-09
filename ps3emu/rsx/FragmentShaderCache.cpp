#include "FragmentShaderCache.h"
#include "xxhash.h"
#include "ps3emu/shaders/shader_dasm.h"

FragmentShader* FragmentShaderCache::retrieve(const FragmentShaderKey& key) {
    auto it = _cache.find(key);
    if (it == end(_cache))
        return nullptr;
    return it->second.get();
}

void FragmentShaderCache::insert(const FragmentShaderKey& key, FragmentShader* shader) {
    _cache[key].reset(shader);
}

FragmentShaderKey FragmentShaderCache::unzip(uint8_t* ptr,
                                             std::array<float, 4>* fconst,
                                             bool mrt) {
    auto info = get_fragment_bytecode_info(ptr);
    auto constCount = 0u;
    for (auto i = 0u; i < info.length; i += 16) {
        auto it = ptr + i;
        if (info.constMap[i / 16]) {
            read_fragment_imm_val(ptr + i, &(*fconst)[0]);
            fconst++;
            constCount++;
            std::fill(it, it + 16, 0);
        }
    }
    return { XXH64(ptr, info.length, 0), info.length, constCount, mrt };
}
