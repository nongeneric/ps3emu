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

bool isAbsoluteBranch(void* instr);
bool isTaken(void* branchInstr, uint64_t cia, PPUThread* thread);
uint64_t getTargetAddress(void* branchInstr, uint64_t cia);

template <DasmMode M, typename S>
void ppu_dasm(void* instr, uint64_t cia, S* state);
