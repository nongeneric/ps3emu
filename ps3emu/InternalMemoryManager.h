#pragma once

#include <cstdint>
#include <utility>

class MainMemory;

class InternalMemoryManager {
    uint32_t _current;
    MainMemory* _mm;
public:
    InternalMemoryManager();
    void setMainMemory(MainMemory* mm);
    void* allocInternalMemory(uint32_t* ea, uint32_t size, uint32_t alignment);
    
    template <int Alignment, typename T, typename... Args>
    T* internalAlloc(uint32_t* ea, Args&&... args) {
        auto ptr = allocInternalMemory(ea, sizeof(T), Alignment);
        return new (ptr) T(std::forward<Args>(args)...);
    }
};
