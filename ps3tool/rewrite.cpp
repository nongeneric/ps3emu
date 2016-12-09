#include "ps3tool.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/utils.h"
#include "ps3tool-core/Rewriter.h"

#include <boost/endian/arithmetic.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <stack>
#include <set>
#include <vector>
#include <algorithm>
#include <fstream>
#include <assert.h>

using namespace boost::endian;
using namespace boost::filesystem;

auto header =
R"(#define EMU_REWRITER
#include <ps3emu/ppu/PPUThread.h>
#include <ps3emu/ppu/ppu_dasm.cpp>
#include <ps3emu/ppu/rewriter.h>
#include <ps3emu/log.h>

#include <stdio.h>
#include <functional>
#include <string>

FILE* f;
void init() {
#ifdef TRACE
    auto tracefile = "/tmp/ps3trace-rewriter";
    f = fopen(tracefile, "w");
#endif
}

thread_local bool trace = true;

#ifdef TRACE
void log(uint32_t nip, PPUThread* thread) {
    if (!trace)
        return;
    fprintf(f, "pc:%08x;", nip);
    for (auto i = 0u; i < 32; ++i) {
        auto r = thread->getGPR(i);
        fprintf(f, "r%d:%08x%08x;", i, (uint32_t)(r >> 32), (uint32_t)r);
    }
    fprintf(f, "r%d:%08x%08x;", 32, 0, (uint32_t)thread->getLR());
    for (auto i = 0u; i < 32; ++i) {
        auto v = thread->getV(i);
        fprintf(f, "v%d:%08x%08x%08x%08x;", i, 
                (uint32_t)(v >> 96),
                (uint32_t)(v >> 64),
                (uint32_t)(v >> 32),
                (uint32_t)v);
    }
    fprintf(f, "\n");
    fflush(f);
}
#else
#define log(a, b)
#endif

#ifndef NDEBUG
#define LOG_FUNC(x) { INFO(libs) << ssnprintf("REWRITER FUNC: %x", x); }
#define LOG_RET(lr) { INFO(libs) << ssnprintf("REWRITER RET: %x", lr); }
#define LOG_RET_TO(x) { INFO(libs) << ssnprintf("REWRITER RET TO: %x", x); }
#define LOG_CALL(from, to) { INFO(libs) << ssnprintf("REWRITER CALL: %x -> %x", from, to); }
#define LOG_VMENTER(x) { INFO(libs) << ssnprintf("REWRITER VMENTER: %x", x); }
#else
#define LOG_FUNC(x)
#define LOG_RET(lr)
#define LOG_RET_TO(x)
#define LOG_CALL(from, to)
#define LOG_VMENTER(x)
#endif

void entryPoint(PPUThread*, int);

)";

auto infoExport =
R"(
extern "C" {
    RewrittenBlocks info {
        entryPoint,
        blocks,
        sizeof(blocks) / sizeof(RewrittenBlock),
        init
    };
}

)";

void HandleRewrite(RewriteCommand const& command) {
    ELFLoader elf;
    MainMemory mm;
    if (!g_state.mm) {
        g_state.mm = &mm;
    }
    RewriterStore rewriterStore;
    elf.load(command.elf);
    uint32_t vaBase = 0;
    uint32_t segmentSize = 0;
    elf.map([&](auto va, auto size, auto index) {
        if (index == 0) {
            vaBase = va;
            segmentSize = size;
        }
    }, command.imageBase, "", &rewriterStore);
    
    std::ofstream log("/tmp/" + path(command.elf).filename().string() + "_rewriter_log");
    assert(log.is_open());
    
    log << ssnprintf("analyzing segment %x-%x\n", vaBase, vaBase + segmentSize);
    
    std::stack<uint32_t> leads;
    if (command.entryPoints.empty()) {
        fdescr epDescr;
        g_state.mm->readMemory(elf.entryPoint(), &epDescr, sizeof(fdescr));
        leads.push(epDescr.va);
    }
    for (auto ep : command.entryPoints) {
        leads.push(ep + command.imageBase);
    }
    
    auto blocks = discoverBasicBlocks(
        vaBase, segmentSize, command.imageBase, leads, log, [](uint32_t cia) {
            return g_state.mm->load32(cia);
        });
    
    for (auto& block : blocks) {
        for (auto cia = block.start; cia < block.start + block.len; cia += 4) {
            std::string rewritten, printed;
            big_uint32_t instr = g_state.mm->load32(cia);
            try {
                ppu_dasm<DasmMode::Rewrite>(&instr, cia, &rewritten);
                ppu_dasm<DasmMode::Print>(&instr, cia, &printed);
            } catch (...) {
                rewritten = "throw std::runtime_error(\"dasm error\")";
                printed = "error";
                auto file = path(command.elf).filename().string();
                auto rva = cia - command.imageBase;
                std::cout << ssnprintf("error disassembling instruction %s: %x\n", file, rva);
            }
            block.body.push_back(ssnprintf("/*%8x: %-24s*/ log(0x%x, TH); %s;", cia, printed, cia, rewritten));
        }
    }
    
    std::ofstream f(command.cpp.c_str());
    assert(f.is_open());
    f << header;
    f << "RewrittenBlock blocks[] {\n";
    for (auto& block : blocks) {
        f << ssnprintf("    { 0x%x, 0x%x },\n", block.start, block.len);
    }
    f << "};\n";
    f << infoExport;
    f << "void entryPoint(PPUThread* thread, int label) {\n"
      << "    static void* labels[] = {\n";
    for (auto& block : blocks) {
        f << ssnprintf("        &&_0x%xu,\n", block.start);
    }
    f << "    };\n\n";
    f << "    goto *labels[label];\n\n";
    for (auto& block : blocks) {
        f << ssnprintf("    _0x%xu:\n", block.start);
        for (auto& line : block.body) {
            f << ssnprintf("        %s\n", line);
        }
    }
    f << "}\n";
}
