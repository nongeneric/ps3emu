#pragma once

#include <stdexcept>
#include <cstdint>
#include "ps3emu/dasm_utils.h"

class PPUThread;

class IllegalInstructionException : public virtual std::runtime_error {
public:
    IllegalInstructionException() : std::runtime_error("illegal instruction") { }
};

class BreakpointException : public virtual std::runtime_error {
public:
    BreakpointException() : std::runtime_error("breakpoint") { }
    BreakpointException(std::string message) : std::runtime_error(message) { }
};

class InfiniteLoopException : public virtual std::runtime_error {
public:
    InfiniteLoopException() : std::runtime_error("infinite loop") { }
};

InstructionInfo analyze(uint32_t instr, uint32_t cia);
bool isAbsoluteBranch(uint32_t instr);
bool isTaken(uint32_t branchInstr, uint32_t cia, PPUThread* thread);
uint64_t getTargetAddress(uint32_t branchInstr, uint32_t cia);

template <DasmMode M, typename S>
void ppu_dasm(void* instr, uint64_t cia, S* state);
