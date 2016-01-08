#pragma once

#include <stdexcept>
#include <cstdint>

class PPUThread;

class IllegalInstructionException : public virtual std::runtime_error {
public:
    IllegalInstructionException() : std::runtime_error("illegal instruction") { }
};

class BreakpointException : public virtual std::runtime_error {
public:
    BreakpointException() : std::runtime_error("breakpoint") { }
};

enum class DasmMode {
    Print, Emulate, Name
};

inline uint64_t ror(uint64_t n, uint8_t s) {
    asm("ror %1,%0" : "+r" (n) : "c" (s));
    return n;
}

inline uint64_t rol(uint64_t n, uint8_t s) {
    asm("rol %1,%0" : "+r" (n) : "c" (s));
    return n;
}

inline uint32_t rol32(uint32_t n, uint8_t s) {
    asm("rol %1,%0" : "+r" (n) : "c" (s));
    return n;
}

bool isAbsoluteBranch(void* instr);
bool isTaken(void* branchInstr, uint64_t cia, PPUThread* thread);
uint64_t getTargetAddress(void* branchInstr, uint64_t cia);

template <DasmMode M, typename S>
void ppu_dasm(void* instr, uint64_t cia, S* state);