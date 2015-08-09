#pragma once

#include <stdint.h>
#include <map>
#include <memory>

struct MemoryPage {
    MemoryPage();
    std::unique_ptr<uint8_t> ptr;
    static constexpr uint pageSize = 4 * 1024;
};

class PPU {
    uint64_t _LR;
    uint64_t _CTR;
    uint64_t _XER;
    uint64_t _GPR[64];
    double _FPR[64];
    double _FPSCR;
    uint32_t _CR;
    
    std::map<uint64_t, MemoryPage> _pages;
    
public:
    void writeMemory(uint64_t va, void* buf, uint len);
    void readMemory(uint64_t va, void* buf, uint len);
    void setMemory(uint64_t va, uint8_t value, uint len);
    void run();
    
    inline void setGPR(int i, uint64_t value) {
        _GPR[i - 1] = value;
    }
    
    inline void setFPSCR(double value) {
        _FPSCR = value;
    }
    
    inline void setLR(uint64_t value) {
        _LR = value;
    }
};