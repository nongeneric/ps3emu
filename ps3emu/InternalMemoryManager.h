#pragma once

#include <cstdint>
#include <utility>
#include <memory>
#include <string>
#include <boost/thread/mutex.hpp>

class InternalMemoryManager {
    uint32_t _base;
    std::string _name;
    boost::mutex _mutex;
    
public:
    InternalMemoryManager(uint32_t base, uint32_t size, std::string name);
    void* allocInternalMemory(uint32_t* ea, uint32_t size, uint32_t alignment);
    
    template <int Alignment, typename T, typename... Args>
    T* internalAlloc(uint32_t* ea, Args&&... args) {
        auto mem = allocInternalMemory(ea, sizeof(T), Alignment);
        return new (mem) T(std::forward<Args>(args)...);
    }
    
    template <int Alignment, typename T, typename... Args>
    auto internalAllocU(uint32_t* ea, Args&&... args) {
        auto mem = allocInternalMemory(ea, sizeof(T), Alignment);
        auto ptr = new (mem) T(std::forward<Args>(args)...);
        auto deleter = [&](auto p) { this->free(p); };
        return std::unique_ptr<T, decltype(deleter)>(ptr, deleter);
    }
    
    void free(uint32_t ea);
    void free(void* ptr);
    unsigned available() const;
};
