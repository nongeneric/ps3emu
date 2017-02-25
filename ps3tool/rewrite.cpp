#include "ps3tool.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/spu/SPUDasm.h"
#include "ps3emu/utils.h"
#include "ps3tool-core/Rewriter.h"
#include "ps3emu/rewriter.h"
#include "ps3emu/fileutils.h"

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
#include <ps3emu/spu/SPUDasm.cpp>
#include <ps3emu/rewriter.h>
#include <ps3emu/log.h>

#include <stdio.h>
#include <functional>
#include <string>
#include <vector>

FILE *f, *spuf;
void init() {
#ifdef TRACE
    auto tracefile = "/tmp/ps3trace-rewriter";
    f = fopen(tracefile, "w");
    auto sputracefile = "/tmp/ps3trace-spu-rewriter";
    spuf = fopen(sputracefile, "w");
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

void logSPU(uint32_t nip, SPUThread* thread) {
    if (!trace)
        return;
    fprintf(spuf, "pc:%08x;", nip);
    
    for (auto i = 0u; i < 128; ++i) {
        auto v = thread->r(i);
        fprintf(spuf, "r%03d:%08x%08x%08x%08x;", i, 
                v.w<0>(),
                v.w<1>(),
                v.w<2>(),
                v.w<3>());
    }
    fprintf(spuf, "\n");
    fflush(spuf);
}
#else
#define log(a, b)
#define logSPU(a, b)
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

struct BasicBlockInfo {
    uint32_t start = 0;
    uint32_t len = 0;
    std::vector<std::string> body;
    uint32_t label = 0;
};

struct SegmentInfo {
    std::string description;
    std::string suffix;
    bool isSpu;
    std::vector<BasicBlockInfo> blocks;
};

SegmentInfo rewritePPU(RewriteCommand const& command, std::ofstream& log) {
    ELFLoader elf;
    RewriterStore rewriterStore;
    elf.load(command.elf);
    uint32_t vaBase, segmentSize;
    elf.map([&](auto va, auto size, auto index) {
        if (index == 0) {
            vaBase = va;
            segmentSize = size;
        }
    }, command.imageBase, {}, &rewriterStore);
    
    std::stack<uint32_t> leads;
    if (!command.imageBase) {
        fdescr epDescr;
        g_state.mm->readMemory(elf.entryPoint(), &epDescr, sizeof(fdescr));
        leads.push(epDescr.va);
    }
    
    for (auto ep : command.entryPoints) {
        leads.push(ep + command.imageBase);
    }
    
    auto blocks = discoverBasicBlocks(
        vaBase, segmentSize, command.imageBase, leads, log, [&](uint32_t cia) {
            return analyze(g_state.mm->load32(cia), cia);
        });
    
    SegmentInfo info;
    
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
        info.blocks.push_back({block.start, block.len, block.body, block.start});
    }
    
    info.isSpu = false;
    info.suffix = "ppu";
    return info;
}

std::vector<uint32_t> selectStart(std::vector<BasicBlock> blocks) {
    std::vector<uint32_t> leads;
    std::transform(begin(blocks), end(blocks), std::back_inserter(leads), [&](auto& block) {
        return block.start;
    });
    return leads;
}

std::vector<uint32_t> selectSubset(std::vector<uint32_t> leads, uint32_t start, uint32_t len) {
    std::vector<uint32_t> res;
    std::copy_if(begin(leads), end(leads), std::back_inserter(res), [&](auto lead) {
        return subset<uint32_t>(lead, 4, start, len);
    });
    return res;
}

template <typename T>
std::stack<T> toStack(std::vector<T> container) {
    std::stack<T> stack;
    for (auto& e : container) {
        stack.push(e);
    }
    return stack;
}

std::vector<SegmentInfo> rewriteSPU(RewriteCommand const& command, std::ofstream& log) {
    auto body = read_all_bytes(command.elf);
    auto elfs = discoverEmbeddedSpuElfs(body);
    uint32_t vaBase = 0x10000u;
    if (!elfs.empty() && elfs[0].header == 0) {
        assert(elfs.size() == 1);
        // the file itself is an spu elf
        g_state.mm->mark(vaBase, body.size(), false, "elf");
        g_state.mm->writeMemory(vaBase, &body[0], body.size());
    } else {
        ELFLoader elfLoader;
        elfLoader.load(command.elf);
        uint32_t mappedSize;
        RewriterStore rewriterStore;
        elfLoader.map([&](auto va, auto size, auto index) {
            if (index == 0) {
                vaBase = va;
            }
            mappedSize = va + size;
        }, command.imageBase, {}, &rewriterStore);
        body.resize(mappedSize);
        g_state.mm->mark(vaBase, mappedSize, false, "elf");
        g_state.mm->readMemory(vaBase, &body[0], body.size());
        elfs = discoverEmbeddedSpuElfs(body);
    }
    
    std::vector<SegmentInfo> infos;
    
    for (auto& elf : elfs) {
        std::vector<Elf32_be_Phdr> copySegments;
        
        auto elfSegment = (Elf32_be_Phdr*)&body[elf.startOffset + elf.header->e_phoff];
        for (auto i = 0u; i < elf.header->e_phnum; ++i) {
            if (elfSegment[i].p_type != SYS_SPU_SEGMENT_TYPE_COPY)
                continue;
            copySegments.push_back(elfSegment[i]);
        }
        
        auto spuElfVaToParentVaInCurrentSegment = [&](uint32_t va, Elf32_be_Phdr& segment) {
            assert(subset<uint32_t>(va, 4, segment.p_vaddr, segment.p_filesz));
            auto spuElfOffset = va - segment.p_vaddr + segment.p_offset;
            return vaBase + elf.startOffset + spuElfOffset;
        };
        
        auto leads = command.entryPoints;
        leads.push_back(elf.header->e_entry);
            
        uint32_t oldSize = 0;
        while (oldSize != leads.size()) {
            oldSize = leads.size();
            for (auto& segment : copySegments) {
                auto segmentLeads = selectSubset(leads, segment.p_vaddr, segment.p_filesz);
                if (segmentLeads.empty())
                    continue;
                auto blocks = discoverBasicBlocks(
                    segment.p_vaddr, segment.p_filesz, 0, toStack(segmentLeads), log, [&](uint32_t cia) {
                        auto elfVa = spuElfVaToParentVaInCurrentSegment(cia, segment);
                        return analyzeSpu(g_state.mm->load32(elfVa), cia);
                    }, &leads);
            }
            std::sort(begin(leads), end(leads));
            leads.erase(std::unique(begin(leads), end(leads)), end(leads));
        }
                
        for (auto& segment : copySegments) {
            auto segmentLeads = selectSubset(leads, segment.p_vaddr, segment.p_filesz);
            if (segmentLeads.empty())
                continue;
            
            auto blocks = discoverBasicBlocks(
                segment.p_vaddr, segment.p_filesz, 0, toStack(segmentLeads), log, [&](uint32_t cia) {
                    auto elfVa = spuElfVaToParentVaInCurrentSegment(cia, segment);
                    return analyzeSpu(g_state.mm->load32(elfVa), cia);
                });
            
            if (elf.isJob) {
                // a spurs job has its EP pointed to a special stub that looks like a series of instructions
                //     44012850       10: xori r80,r80,4
                //     32000080       14: br 18
                //     44012850       18: xori r80,r80,4
                //     32000280       1c: br 30
                //     ascii "bin2"
                // this stub consists of two basic blocks and must never be overwritten
                blocks.erase(begin(blocks), begin(blocks) + 2);
            }
            
            SegmentInfo info;
            for (auto& block : blocks) {
                if (elf.isJob) {
                    // jobs are position independent
                    block.body.push_back(ssnprintf(
                        "if (!pic_offset_set) { pic_offset = bb_va - 0x%x; pic_offset_set = true; }",
                        block.start));
                }
                for (auto cia = block.start; cia < block.start + block.len; cia += 4) {
                    std::string rewritten, printed;
                    big_uint32_t instr = g_state.mm->load32(spuElfVaToParentVaInCurrentSegment(cia, segment));
                    try {
                        SPUDasm<DasmMode::Rewrite>(&instr, cia, &rewritten);
                        SPUDasm<DasmMode::Print>(&instr, cia, &printed);
                    } catch (...) {
                        rewritten = "throw std::runtime_error(\"dasm error\")";
                        printed = "error";
                        auto file = path(command.elf).filename().string();
                        auto rva = cia - command.imageBase;
                        std::cout << ssnprintf("error disassembling instruction %s: %x\n", file, rva);
                    }
                    
                    auto info = analyzeSpu(instr, cia);
                    auto outOfSegmentBr = info.flow && info.target != 0 &&
                                          !subset<uint32_t>(info.target, 4, segment.p_vaddr, segment.p_filesz);
                    
                    auto line = ssnprintf("/* %08x %8x: %-24s*/ logSPU(0x%x, th); %s;", instr, cia, printed, cia, rewritten);
                    if (outOfSegmentBr) {
                        block.body.push_back("#undef SPU_SET_NIP");
                        block.body.push_back("#define SPU_SET_NIP SPU_SET_NIP_INDIRECT");
                    }
                    block.body.push_back(line);
                    if (outOfSegmentBr) {
                        block.body.push_back("#undef SPU_SET_NIP");
                        block.body.push_back("#define SPU_SET_NIP SPU_SET_NIP_INITIAL");
                    }
                }
                info.blocks.push_back({spuElfVaToParentVaInCurrentSegment(block.start, segment),
                                       block.len,
                                       block.body,
                                       block.start});
            }
            
            info.isSpu = true;
            info.suffix = ssnprintf("spu_%x_%x_%x%s",
                                    elf.startOffset,
                                    segment.p_vaddr,
                                    segment.p_vaddr + segment.p_filesz,
                                    elf.isJob ? "_SPURS_JOB" : "");
            infos.push_back(info);
        }
    }
    
    return infos;
}

void HandleRewrite(RewriteCommand const& command) {
    MainMemory mm;
    if (!g_state.mm) {
        g_state.mm = &mm;
    }
    
    std::ofstream log("/tmp/" + path(command.elf).filename().string() + "_rewriter_log");
    assert(log.is_open());
    
    std::vector<SegmentInfo> segmentInfos;
    if (command.isSpu) {
        segmentInfos = rewriteSPU(command, log);
    } else {
        segmentInfos.push_back(rewritePPU(command, log));
    }

    std::ofstream f(command.cpp.c_str());
    assert(f.is_open());
    
    f << header;
    
    auto entrySignature = [&](auto& segment) {
        return ssnprintf("void entryPoint_%s(%sThread* thread, int label%s)",
                         segment.suffix,
                         segment.isSpu ? "SPU" : "PPU",
                         segment.isSpu ? ", uint32_t bb_va" : "");
    };
    
    for (auto& segment : segmentInfos) {
        f << entrySignature(segment) << ";\n";
    }
    
    for (auto& segment : segmentInfos) {
        f << ssnprintf("std::vector<RewrittenBlock> segment_%s_blocks {\n", segment.suffix);
        for (auto& block : segment.blocks) {
            f << ssnprintf("    { 0x%x, 0x%x },\n", block.start, block.len);
        }
        f << "};\n";
    }
    
    for (auto& segment : segmentInfos) {
        f << ssnprintf("RewrittenSegment segment_%s {\n", segment.suffix);
        auto entryName = ssnprintf("entryPoint_%s", segment.suffix);
        auto ppuEntry = segment.isSpu ? "nullptr" : entryName;
        auto spuEntry = segment.isSpu ? entryName : "nullptr";
        auto blocks = ssnprintf("&segment_%s_blocks", segment.suffix);
        f << ssnprintf("    %s, %s, \"%s\", %s,\n",
                        ppuEntry,
                        spuEntry,
                        segment.suffix,
                        blocks);
        f << "};\n";
    }
    
    f << "extern \"C\" {\n";
    f << "    RewrittenSegmentsInfo info {\n";
    f << "        {\n";
    for (auto& segment : segmentInfos) {
        f << ssnprintf("            segment_%s,\n", segment.suffix);
    }
    f << "        }, \n";
    f << "        init\n";
    f << "    };\n";
    f << "};\n";
    
    for (auto& segment : segmentInfos) {
        f << entrySignature(segment) << " {\n";
        f << "    static void* labels[] = {\n";
        for (auto& block : segment.blocks) {
            f << ssnprintf("        &&_0x%xu,\n", block.label);
        }
        f << "    };\n\n";
        f << "    uint32_t pic_offset = 0;\n";
        f << "    bool pic_offset_set = false;\n";
        f << "    goto *labels[label];\n\n";
        for (auto& block : segment.blocks) {
            f << ssnprintf("    _0x%xu:\n", block.label);
            for (auto& line : block.body) {
                f << ssnprintf("        %s\n", line);
            }
        }
        f << "}\n";
    }
}
