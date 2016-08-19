#pragma once

#include <cstdint>
#include <utility>

class InternalMemoryManager {
    uint32_t _base;
    
public:
    InternalMemoryManager(uint32_t base, uint32_t size);
    void* allocInternalMemory(uint32_t* ea, uint32_t size, uint32_t alignment);
    
    template <int Alignment, typename T, typename... Args>
    T* internalAlloc(uint32_t* ea, Args&&... args) {
        auto ptr = allocInternalMemory(ea, sizeof(T), Alignment);
        return new (ptr) T(std::forward<Args>(args)...);
    }
    
    void free(uint32_t ea);
    void free(void* ptr);
};
