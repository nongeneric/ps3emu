#include "ps3tool.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/utils.h"

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
    fprintf(f, "r%d:%08x%08x;", 32, 0, thread->getLR());
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

)";

auto infoExport =
R"(
extern "C" {
    RewrittenFunctions info {
        functions, sizeof(functions) / sizeof(RewrittenFunction), init
    };
}

)";

struct FunctionInfo {
    uint32_t ep;
    uint32_t start;
    uint32_t len;
    std::vector<bool> map;
    std::vector<std::string> body;
    std::vector<uint32_t> dirty;
    bool contains(uint32_t va) {
        return (start <= va && va < start + len) && map[(va - start) / 4];
    }
};

void HandleRewrite(RewriteCommand const& command) {
    ELFLoader elf;
    MainMemory mm;
    elf.load(command.elf);
    uint32_t vaBase = 0;
    uint32_t segmentSize = 0;
    elf.map(&mm, [&](auto va, auto size, auto index) {
        if (index == 0) {
            vaBase = va;
            segmentSize = size;
        }
    }, command.imageBase, "");
    
    std::ofstream log("/tmp/" + path(command.elf).filename().string() + "_rewriter_log");
    assert(log.is_open());
    
    log << ssnprintf("analyzing segment %x-%x\n", vaBase, vaBase + segmentSize);
    
    std::vector<FunctionInfo> functions;
    
    std::stack<uint32_t> eps;
    if (command.entryPoints.empty()) {
        fdescr epDescr;
        mm.readMemory(elf.entryPoint(), &epDescr, sizeof(fdescr));
        eps.push(epDescr.va);
    }
    for (auto ep : command.entryPoints) {
        eps.push(ep + command.imageBase);
    }
    std::set<uint32_t> visitedFunctions;
    for (auto ep : command.ignoredEntryPoints) {
        visitedFunctions.insert(ep);
    }
    
    while (!eps.empty()) {
        auto ep = eps.top();
        eps.pop();
        
        if (visitedFunctions.find(ep) != end(visitedFunctions))
            continue;
        visitedFunctions.insert(ep);
        
        log << ssnprintf("following an entry point %x(%x)\n", ep, ep - command.imageBase);
        std::set<uint32_t> visited;
        std::stack<uint32_t> leads;
        leads.push(ep);
        
        while (!leads.empty()) {
            auto cia = leads.top();
            leads.pop();
            log << ssnprintf("following a lead %x(%x)\n", cia, cia - command.imageBase);
            
            for (;;) {
                if (visited.find(cia) != end(visited))
                    break;
                visited.insert(cia);
                auto info = analyze(mm.load32(cia), cia);
                if (info.targetVa) {
                    if (info.isFunctionCall) {
                        log << ssnprintf("function call %x(%x) -> %x(%x)\n",
                                         cia,
                                         cia - command.imageBase,
                                         info.targetVa,
                                         info.targetVa - command.imageBase);
                        eps.push(info.targetVa);
                    } else {
                        log << ssnprintf("new lead %x(%x) -> %x(%x)\n",
                                         cia,
                                         cia - command.imageBase,
                                         info.targetVa,
                                         info.targetVa - command.imageBase);
                        leads.push(info.targetVa);
                    }
                }
                if (info.isAlwaysTaken && !info.isFunctionCall)
                    break;
                cia += 4;
            }
        }

        FunctionInfo info;
        info.ep = ep;
        info.start = *visited.begin();
        info.len = *visited.rbegin() - *visited.begin() + 4;
        info.map.resize(info.len - 1);
        for (auto i = info.start; i != info.start + info.len; i += 4) {
            info.map.at((i - info.start) / 4) = visited.find(i) != end(visited);
        }
        functions.push_back(info);
        log << ssnprintf("function discovered %x-%x\n", ep, ep + functions.back().len);
    }
    
    for (auto& fn : functions) {
        for (auto i = fn.start; i < fn.start + fn.len; i += 4) {
            if (!fn.map.at((i - fn.start) / 4))
                continue;
            big_uint32_t instr = mm.load32(i);
            auto info = analyze(instr, i);
            if (info.isBCCTR) {
                fn.dirty.push_back(i);
            }
        }
    }
    
    for (auto& fn : functions) {
        fn.body.push_back(ssnprintf("LOG_FUNC(0x%x);", fn.start));
        for (auto i = fn.start; i < fn.start + fn.len; i += 4) {
            if (!fn.map.at((i - fn.start) / 4))
                continue;
            std::string rewritten, printed;
            big_uint32_t instr = mm.load32(i);
            auto info = analyze(instr, i);
            try {
                ppu_dasm<DasmMode::Rewrite>(&instr, i, &rewritten);
            } catch (...) {
                auto file = path(command.elf).filename().string();
                auto rva = i - command.imageBase;
                std::cout << ssnprintf("error disassembling instruction %s: %x\n", file, rva);
                continue;
            }
            bool nolabel = false;
            if (info.targetVa) {
                auto it = std::find_if(begin(functions), end(functions), [&](auto& f) {
                    return f.ep == info.targetVa;
                });
                if (fn.contains(info.targetVa) && !info.isFunctionCall) {
                    fn.body.push_back("#ifdef SET_NIP");
                    fn.body.push_back("#undef SET_NIP");
                    fn.body.push_back("#endif");
                    fn.body.push_back(ssnprintf("#define SET_NIP(x) goto _0x%x", info.targetVa));
                } else if (it != end(functions) && it->dirty.empty()) {
                    assert(info.isFunctionCall);
                    fn.body.push_back("#ifdef SET_NIP");
                    fn.body.push_back("#undef SET_NIP");
                    fn.body.push_back("#endif");
                    fn.body.push_back(
                        ssnprintf("#define SET_NIP(x) LOG_CALL(0x%x, 0x%x); f_%x(TH); LOG_RET_TO(0x%x);",
                                  i,
                                  info.targetVa,
                                  info.targetVa,
                                  i + 4));
                } else {
                    assert(info.isFunctionCall);
                    fn.body.push_back(ssnprintf("_0x%x:", i));
                    fn.body.push_back(ssnprintf("LOG_VMENTER(0x%x);", i));
                    fn.body.push_back(ssnprintf("TH->setNIP(0x%x);", i));
                    fn.body.push_back(ssnprintf("TH->vmenter(0x%x);", i + 4));
                    fn.body.push_back(ssnprintf("LOG_RET_TO(0x%x);", i + 4));
                    rewritten = "";
                    nolabel = true;
                }
            }
            ppu_dasm<DasmMode::Print>(&instr, i, &printed);
            std::string log = command.trace ? ssnprintf(" log(0x%x, TH); ", i) : "";
            std::string label = nolabel ? "" : ssnprintf("_0x%x:", i);
            fn.body.push_back(ssnprintf("%s%s\t%s; // %s", label, log, rewritten, printed));
        }
    }
    
    std::ofstream f(command.cpp.c_str());
    assert(f.is_open());
    if (command.trace) {
        f << "#define TRACE\n";
    }
    f << header;
    for (auto& info : functions) {
        if (!info.dirty.empty())
            continue;
        f << ssnprintf("void f_%x(PPUThread* thread);\n", info.ep);
    }
    f << "\n";
    f << "RewrittenFunction functions[] {\n";
    for (auto& info : functions) {
        if (!info.dirty.empty())
            continue;
        f << ssnprintf("    { \"f_%x\", 0x%x, f_%x },\n", info.ep, info.ep, info.ep);
    }
    f << "};\n";
    f << infoExport;
    f << "\n";
    for (auto& info : functions) {
        if (!info.dirty.empty())
            continue;
        f << ssnprintf("void f_%x(PPUThread* thread) {\n", info.ep);
        if (info.ep != info.start) {
            f << ssnprintf("    goto _0x%x;\n", info.ep);
        }
        for (auto& line : info.body) {
            f << ssnprintf("    %s\n", line);
        }
        f << "}\n\n";
    }
}
