#include "ps3tool.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/spu/SPUDasm.h"
#include "ps3emu/utils.h"
#include "ps3tool-core/Rewriter.h"
#include "ps3tool-core/GraphTools.h"
#include "ps3tool-core/SetTools.h"
#include "ps3tool-core/NinjaScript.h"
#include "ps3emu/RewriterUtils.h"
#include "ps3emu/rewriter.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3emu/execmap/InstrDb.h"

#include <boost/endian/arithmetic.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <stack>
#include <set>
#include <vector>
#include <algorithm>
#include <fstream>
#include <assert.h>
#include <functional>
#include <random>

using namespace boost::algorithm;
using namespace boost::endian;
using namespace boost::filesystem;

auto mainText =
R"(#include "{headerName}.h"
void init() {
}

#ifdef TRACE
#undef th

thread_local bool trace = true;
thread_local bool thread_init = false;
thread_local FILE* f;
thread_local FILE* spuf;

void trace_init() {
    if (thread_init)
        return;
    thread_init = true;
    if (g_state.sth) {
        auto sputracefile = "/tmp/ps3trace-spu-rewriter_" + g_state.sth->getName();
        spuf = fopen(sputracefile.c_str(), "w");
    }
    if (g_state.th) {
        auto tracefile = "/tmp/ps3trace_rewriter_" + g_state.th->getName();
        f = fopen(tracefile.c_str(), "w");
    }
}

void log(uint32_t nip, PPUThread* thread) {
    if (!trace)
        return;
    trace_init();
    fprintf(f, "pc:%08x;", nip);
    for (auto i = 0u; i < 32; ++i) {
        auto r = thread->getGPR(i);
        fprintf(f, "r%d:%08x%08x;", i, (uint32_t)(r >> 32), (uint32_t)r);
    }
    fprintf(f, "r%d:%08x%08x;", 32, 0, (uint32_t)thread->getLR());
    for (auto i = 0u; i < 32; ++i) {
        auto r = thread->r(i);
        fprintf(f, "v%d:%08x%08x%08x%08x;", i, 
                (uint32_t)r.w(0),
                (uint32_t)r.w(1),
                (uint32_t)r.w(2),
                (uint32_t)r.w(3));
    }
    fprintf(f, "\n");
    fflush(f);
}

void logSPU(uint32_t nip, SPUThread* thread) {
    if (!trace)
        return;
    trace_init();
    
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
#endif
)";

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

void init();

#ifdef TRACE
void log(uint32_t nip, PPUThread* thread);
void logSPU(uint32_t nip, SPUThread* thread);
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

std::tuple<std::optional<uint32_t>, uint32_t> outOfPartTargets(
    BasicBlock const& block, std::vector<BasicBlock> const& part, auto analyze)
{
    auto info = analyze(block.start + block.len - 4);
    auto find = [&](uint32_t target) {
        return part.end() !=
               std::find_if(part.begin(), part.end(), [&](auto& block) {
                   return block.start == target;
               });
    };
    std::optional<uint32_t> target;
    uint32_t fallthrough = block.start + block.len;
    if ((info.flow && !info.passthrough) || find(fallthrough)) {
        fallthrough = 0;
    }
    if (info.flow && info.target && !find(*info.target)) {
        target = *info.target;
    }
    return {target, fallthrough};
}

std::set<uint32_t> collectInitialPPULeads(uint32_t vaBase, ELFLoader& elf, uint32_t imageBase) {
    std::set<uint32_t> leads;
    if (!imageBase) {
        fdescr epDescr;
        g_state.mm->readMemory(elf.entryPoint(), &epDescr, sizeof(fdescr));
        leads.insert(epDescr.va);
    }
    
    auto [exports, nexports] = elf.exports();
    for (auto i = 0; i < nexports; ++i) {
        auto& e = exports[i];
        auto stubs = reinterpret_cast<big_uint32_t*>(g_state.mm->getMemoryPointer(e.stub_table, 0));
        for (auto i = 0; i < e.functions; ++i) {
            fdescr descr;
            g_state.mm->readMemory(stubs[i], &descr, sizeof(fdescr));
            leads.insert(descr.va);
        }
    }
    
    return leads;
}

std::vector<SegmentInfo> rewritePPU(RewriteCommand const& command, std::ofstream& log) {
    ELFLoader elf;
    RewriterStore rewriterStore;
    elf.load(command.elf);
    uint32_t vaBase, segmentSize;
    elf.map([&](auto va, auto size, auto index) {
        if (index == 0) {
            vaBase = va;
            segmentSize = size;
        }
    }, command.imageBase, {}, &rewriterStore, true);
    
    auto leads = collectInitialPPULeads(vaBase, elf, command.imageBase);
    auto analyzeFunc = [&](uint32_t cia) { return analyze(g_state.mm->load32(cia), cia); };
    
    auto validate = [&](auto cia) {
        try {
            auto instr = endian_reverse(g_state.mm->load32(cia));
            std::string str;
            ppu_dasm<DasmMode::Print>(&instr, cia, &str);
            return true;
        } catch (IllegalInstructionException& e) {
            return false;
        }
    };
    
    auto blocks = discoverBasicBlocks(
        vaBase, segmentSize, leads, log, analyzeFunc, validate, [&](uint32_t cia) {
            auto instr = endian_reverse(g_state.mm->load32(cia));
            std::string name;
            ppu_dasm<DasmMode::Name>(&instr, cia, &name);
            return "ppu_" + name;
        }, {});
    auto parts = partitionBasicBlocks(blocks, analyzeFunc);
    
    for (auto& part : parts) {
        for (auto& block : part) {
            for (auto cia = block.start; cia < block.start + block.len; cia += 4) {
                std::string rewritten, printed;
                big_uint32_t instr = g_state.mm->load32(cia);
                ppu_dasm<DasmMode::Rewrite>(&instr, cia, &rewritten);
                ppu_dasm<DasmMode::Print>(&instr, cia, &printed);
                block.body.push_back(ssnprintf("/*%8x: %-24s*/ log(0x%x, TH); %s;", cia, printed, cia, rewritten));
            }
            auto [target, fallthrough] = outOfPartTargets(block, part, analyzeFunc);
            if (target) {
                block.body.insert(block.body.begin(), "#define SET_NIP SET_NIP_INDIRECT");
                block.body.insert(block.body.begin(), "#undef SET_NIP");
                block.body.insert(block.body.begin(), ssnprintf("// target %x leads out of part", *target));
                block.body.push_back("#undef SET_NIP");
                block.body.push_back("#define SET_NIP SET_NIP_INITIAL");
            }
            if (fallthrough) {
                block.body.insert(block.body.begin(), ssnprintf("// possible fallthrough %x leads out of part", fallthrough));
                block.body.push_back(ssnprintf("SET_NIP_INDIRECT(0x%x);", fallthrough));
            }
        }
    }
    
    std::vector<SegmentInfo> infos;
    for (auto i = 0u; i < parts.size(); i++) {
        SegmentInfo info;
        info.isSpu = false;
        info.suffix = ssnprintf("ppu_bin_%d", i);
        for (auto& block : parts[i]) {
            info.blocks.push_back({block.start, block.len, block.body, block.start});
        }
        infos.push_back(info);
    }
    
    return infos;
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

std::tuple<uint32_t, std::vector<EmbeddedElfInfo>> mapEmbeddedElfs(
    std::string elf, uint32_t imageBase, std::vector<uint8_t>& body)
{
    body = read_all_bytes(elf);
    auto elfs = discoverEmbeddedSpuElfs(body);
    uint32_t vaBase = 0x10000u;
    if (!elfs.empty() && elfs[0].header == 0) {
        assert(elfs.size() == 1);
        // the file itself is an spu elf
        g_state.mm->mark(vaBase, body.size(), false, "elf");
        g_state.mm->writeMemory(vaBase, &body[0], body.size());
    } else {
        ELFLoader elfLoader;
        elfLoader.load(elf);
        uint32_t mappedSize;
        RewriterStore rewriterStore;
        elfLoader.map([&](auto va, auto size, auto index) {
            if (index == 0) {
                vaBase = va;
            }
            mappedSize = va + size;
        }, imageBase, {}, &rewriterStore, true);
        body.resize(mappedSize);
        g_state.mm->mark(vaBase, mappedSize, false, "elf");
        g_state.mm->readMemory(vaBase, &body[0], body.size());
        elfs = discoverEmbeddedSpuElfs(body);
    }
    return {vaBase, elfs};
}

std::vector<BasicBlock> CollectSpuSegmentBasicBlocks(
    std::vector<uint32_t> const& leads,
    Elf32_be_Phdr const& segment,
    std::ofstream& log,
    auto spuElfVaToParentVaInCurrentSegment)
{
    auto segmentLeads = selectSubset(leads, segment.p_vaddr, segment.p_filesz);
    
    auto read = [&](uint32_t cia) {
        auto elfVa = spuElfVaToParentVaInCurrentSegment(cia, segment);
        return g_state.mm->load32(elfVa);
    };
    
    auto validate = [&](auto cia) {
        try {
            auto instr = endian_reverse(read(cia));
            std::string str;
            SPUDasm<DasmMode::Print>(&instr, cia, &str);
            return true;
        } catch (IllegalInstructionException& e) {
            return false;
        }
    };
    
    auto blocks = discoverBasicBlocks(
        segment.p_vaddr, segment.p_filesz, toSet(segmentLeads), log, [&](uint32_t cia) {
            return analyzeSpu(read(cia), cia);
        }, validate, [&](uint32_t cia) {
            auto instr = endian_reverse(read(cia));
            std::string name;
            SPUDasm<DasmMode::Name>(&instr, cia, &name);
            return "spu_" + name;
        }, read, true
    );
    
    return blocks;
}

/*

    ppu_elf:  spu_elf_1:   segment_1_1
                           segment_1_2
                           segment_1_3
              spu_elf_2:   segment_2_1
              spu_elf_3:   segment_3_1
              
    segments are elf sections, that can overlap

*/
std::vector<SegmentInfo> rewriteSPU(RewriteCommand const& command, std::ofstream& log) {
    std::vector<uint8_t> body;
    auto [vaBase, elfs] = mapEmbeddedElfs(command.elf, command.imageBase, body);
    
    std::vector<SegmentInfo> infos;
    
    int totalSegmentNumber = 0;
    for (auto& elf : elfs) {
        std::vector<Elf32_be_Phdr> copySegments;
        
        auto elfSegment = (Elf32_be_Phdr*)&body[elf.startOffset + elf.header->e_phoff];
        for (auto i = 0u; i < elf.header->e_phnum; ++i) {
            if (elfSegment[i].p_type != SYS_SPU_SEGMENT_TYPE_COPY)
                continue;
            if ((elfSegment[i].p_flags & PF_X) == 0)
                continue;
            copySegments.push_back(elfSegment[i]);
        }
        
        auto spuElfVaToParentVaInCurrentSegment = [&](uint32_t va, Elf32_be_Phdr const& segment) {
            assert(subset<uint32_t>(va, 4, segment.p_vaddr, segment.p_filesz));
            auto spuElfOffset = va - segment.p_vaddr + segment.p_offset;
            return vaBase + elf.startOffset + spuElfOffset;
        };
        
        std::vector<uint32_t> leads;
        // entry point might not lead to code for some segments so ignore it competely
                
        for (auto& segment : copySegments) {
            auto blocks =
                CollectSpuSegmentBasicBlocks(leads,
                                             segment,
                                             log,
                                             spuElfVaToParentVaInCurrentSegment);
            
            if (blocks.empty())
                continue;
                
            if (elf.isJob) {
                // a spurs job has its EP pointed to a special stub that looks like a series of instructions
                //     42855402        0: ila r2,0x10aa8
                //     435c1082        4: ila r2,0x2b821
                //     42f8ab02        8: ila r2,0x1f156
                //     42f41b82        c: ila r2,0x1e837
                //     44012850       10: xori r80,r80,4
                //     32000080       14: br 18
                //     44012850       18: xori r80,r80,4
                //     32000280       1c: br 30
                //     ascii "bin2"
                // this stub consists of up to three basic blocks and must never be overwritten
                for (auto offset : {0u, 4u, 8u, 0xcu, 0x10u, 0x14u, 0x18u, 0x1cu}) {
                    auto it = std::find_if(begin(blocks), end(blocks), [&](auto& block) {
                        return block.start == offset;
                    });
                    if (it != end(blocks)) {
                        blocks.erase(it);
                    }
                }
            }
                
            auto analyzeFunc = [&](uint32_t cia) { 
                auto elfVa = spuElfVaToParentVaInCurrentSegment(cia, segment);
                return analyzeSpu(g_state.mm->load32(elfVa), cia);
            };
            
            totalSegmentNumber++;
            auto parts = partitionBasicBlocks(blocks, analyzeFunc, 300);
            int partNum = 0;
            for (auto& part : parts) {
                SegmentInfo info;
                for (auto& block : part) {
                    if (elf.isJob) { // all jobs are position independent
                        block.body.push_back(ssnprintf(
                            "if (!pic_offset_set) { pic_offset = bb_va - 0x%x; pic_offset_set = true; }",
                            block.start));
                    }
                    for (auto cia = block.start; cia < block.start + block.len; cia += 4) {
                        std::string rewritten, printed;
                        big_uint32_t instr = g_state.mm->load32(spuElfVaToParentVaInCurrentSegment(cia, segment));
                        SPUDasm<DasmMode::Rewrite>(&instr, cia, &rewritten);
                        SPUDasm<DasmMode::Print>(&instr, cia, &printed);
                        auto line = ssnprintf("/* %08x %8x: %-24s*/ logSPU(0x%x, th); %s;", instr, cia, printed, cia, rewritten);
                        block.body.push_back(line);
                    }
                    
                    auto [target, fallthrough] = outOfPartTargets(block, part, analyzeFunc);
                    if (target) {
                        block.body.insert(begin(block.body), "#define SPU_SET_NIP SPU_SET_NIP_INDIRECT_PIC");
                        block.body.insert(begin(block.body), "#undef SPU_SET_NIP");
                        block.body.insert(begin(block.body), ssnprintf("// target %x leads out of part", *target));
                        block.body.push_back("#undef SPU_SET_NIP");
                        block.body.push_back("#define SPU_SET_NIP SPU_SET_NIP_INITIAL");
                    }
                    if (fallthrough) {
                        block.body.insert(block.body.begin(), ssnprintf("// possible fallthrough %x leads out of part", fallthrough));
                        block.body.push_back(ssnprintf("SPU_SET_NIP_INDIRECT_PIC(0x%x);", fallthrough));
                    }
                    
                    info.blocks.push_back({spuElfVaToParentVaInCurrentSegment(block.start, segment),
                                        block.len,
                                        block.body,
                                        block.start});
                }
                
                info.isSpu = true;
                info.suffix = ssnprintf("spu_%x_%x_%x_part_%d_%s",
                                        elf.startOffset,
                                        segment.p_vaddr,
                                        segment.p_vaddr + segment.p_filesz,
                                        partNum,
                                        elf.isJob ? "_SPURS_JOB" : "");
                infos.push_back(info);
                partNum++;
            }
        }
    }
    
    return infos;
}

std::string entrySignature(auto& segment) {
    return ssnprintf("void entryPoint_%s(%sThread* thread, int label%s)",
                     segment.suffix,
                     segment.isSpu ? "SPU" : "PPU",
                     segment.isSpu ? ", uint32_t bb_va" : "");
}

void printHeader(std::string name, std::vector<SegmentInfo> const& segments) {
    std::ofstream f(name);
    assert(f.is_open());
    
    f << header;
    
    for (auto& segment : segments) {
        f << entrySignature(segment) << ";\n";
    }
    
    for (auto& segment : segments) {
        f << ssnprintf("extern std::vector<RewrittenBlock> segment_%s_blocks;\n", segment.suffix);
    }
    
    for (auto& segment : segments) {
        f << ssnprintf("extern RewrittenSegment segment_%s;\n", segment.suffix);
    }
}

void printMain(std::string name, std::vector<SegmentInfo> const& segments) {
    std::ofstream f(name);
    assert(f.is_open());

    std::string text = mainText;
    replace_first(text, "{headerName}", basename(name));
    f << text, 
    
    f << "extern \"C\" {\n";
    f << "    RewrittenSegmentsInfo info {\n";
    f << "        {\n";
    for (auto& segment : segments) {
        f << ssnprintf("            segment_%s,\n", segment.suffix);
    }
    f << "        }, \n";
    f << "        init\n";
    f << "    };\n";
    f << "};\n\n";
}

void printSegment(std::string baseName, std::string name, SegmentInfo const& segment) {
    std::ofstream f(name);
    assert(f.is_open());
    
    f << ssnprintf("#include \"%s.h\"\n\n", baseName);
    
    f << ssnprintf("std::vector<RewrittenBlock> segment_%s_blocks {\n", segment.suffix);
    for (auto& block : segment.blocks) {
        f << ssnprintf("    { 0x%x, 0x%x },\n", block.start, block.len);
    }
    f << "};\n";
    
    f << ssnprintf("RewrittenSegment segment_%s {\n", segment.suffix);
    auto entryName = ssnprintf("entryPoint_%s", segment.suffix);
    auto ppuEntry = segment.isSpu ? "nullptr" : entryName;
    auto spuEntry = segment.isSpu ? entryName : "nullptr";
    auto blocks = ssnprintf("&segment_%s_blocks", segment.suffix);
    f << ssnprintf("    %s, %s, \"%s\", %s\n",
                   ppuEntry,
                   spuEntry,
                   segment.suffix,
                   blocks);
    f << "};\n";
    
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

void printNinja(std::string name, std::vector<SegmentInfo> const& segments) {
    NinjaScript script;
    script.variable("opt", static_debug ? "-O0 -ggdb -DNDEBUG" : "-O3 -ggdb -DNDEBUG");
    script.variable("trace", "");
    script.rule("compile", compileRule());
    script.rule("link", linkRule());
    std::string objects;
    auto baseName = basename(name);
    for (auto i = 0u; i < segments.size(); ++i) {
        auto in = ssnprintf("%s.segment_%d.cpp", baseName, i);
        auto out = ssnprintf("%s.segment_%d.o", baseName, i);
        script.statement("compile", in, out, {});
        objects += out + " ";
    }
    auto inMain = ssnprintf("%s.cpp", baseName);
    auto outMain = ssnprintf("%s.o", baseName);
    script.statement("compile", inMain, outMain, {});
    objects += outMain;
    
    auto out = ssnprintf("%s.x86.so", baseName);
    script.statement("link", objects, out, {});
    
    std::ofstream f(name);
    assert(f.is_open());
    f << script.dump();
}

void HandleRewrite(RewriteCommand const& command) {
    MainMemory mm;
    if (!g_state.mm) {
        g_state.mm = &mm;
    }
    InternalMemoryManager memalloc(EmuInternalArea, EmuInternalAreaSize, "internal alloc");
    if (!g_state.memalloc) {
        g_state.memalloc = &memalloc;
    }
    
    std::ofstream log("/tmp/" + path(command.elf).filename().string() + "_rewriter_log");
    assert(log.is_open());
    
    std::vector<SegmentInfo> segmentInfos;
    if (command.isSpu) {
        segmentInfos = rewriteSPU(command, log);
    } else {
        segmentInfos = rewritePPU(command, log);
    }
    
    auto headerName = command.cpp + ".h";
    auto ninjaName = command.cpp + ".ninja";
    auto mainName = command.cpp + ".cpp";
    
    printHeader(headerName, segmentInfos);
    printMain(mainName, segmentInfos);
    printNinja(ninjaName, segmentInfos);
    for (auto i = 0u; i < segmentInfos.size(); ++i) {
        auto segmentName = ssnprintf("%s.segment_%d.cpp", command.cpp, i);
        printSegment(basename(headerName), segmentName, segmentInfos[i]);
    }
}
