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

class InfiniteLoopException : public virtual std::runtime_error {
public:
    InfiniteLoopException() : std::runtime_error("infinite loop") { }
};

enum class DasmMode {
    Print, Emulate, Name, Rewrite
};

struct PPUInstructionInfo {
    bool isFunctionCall = false;
    bool isConditionalBranch = false;
    bool isAlwaysTaken = false;
    bool isBCLR = false;
    bool isBLR = false;
    bool isBCCTR = false;
    uint32_t targetVa = 0;
};

PPUInstructionInfo analyze(uint32_t instr, uint32_t cia);
bool isAbsoluteBranch(uint32_t instr);
bool isTaken(uint32_t branchInstr, uint32_t cia, PPUThread* thread);
uint64_t getTargetAddress(uint32_t branchInstr, uint32_t cia);

template <DasmMode M, typename S>
void ppu_dasm(void* instr, uint64_t cia, S* state);
