#pragma once

#include <cstdint>
#include <utility>


class MainMemory;

class InternalMemoryManager {
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
    
    void free(uint32_t ea);
    void free(void* ptr);
};
