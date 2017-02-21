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
#include <ps3emu/rewriter.h>
#include <ps3emu/log.h>

#include <stdio.h>
#include <functional>
#include <string>
#include <vector>

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

)";

struct SegmentInfo {
    std::string description;
    std::string suffix;
    bool isSpu;
    std::vector<BasicBlock> blocks;
};

struct SubElfInfo {
    uint32_t va;
    uint32_t len;
    const uint8_t* ptr;
    std::vector<uint32_t> entryPoints;
};

class IRewriter {
public:
    virtual void map(std::string elf, uint32_t imageBase) = 0;
    virtual std::string rewrite(uint32_t instr, uint32_t cia) = 0;
    virtual std::string print(uint32_t instr, uint32_t cia) = 0;
    virtual InstructionInfo analyze(uint32_t instr, uint32_t cia) = 0;
    virtual std::vector<SubElfInfo> codeSegments() = 0;
};

class PPURewriter : public IRewriter {
    ELFLoader _elf;
    uint32_t _vaBase;
    uint32_t _mappedSize;
    bool _islib;;
    std::vector<uint8_t> _mapped;
    
public:
    void map(std::string elf, uint32_t imageBase) override {
        RewriterStore rewriterStore;
        _elf.load(elf);
        _elf.map([&](auto va, auto size, auto index) {
            if (index == 0) {
                _vaBase = va;
            }
            _mappedSize = va + size - _vaBase;
        }, imageBase, "", &rewriterStore);
        _mapped.resize(_mappedSize);
        g_state.mm->mark(_vaBase, _mappedSize, true, "elf");
        g_state.mm->readMemory(_vaBase, &_mapped[0], _mapped.size());
        _islib = imageBase != 0;
    }
    
    std::string rewrite(uint32_t instr, uint32_t cia) override {
        std::string res;
        ppu_dasm<DasmMode::Rewrite>(&instr, cia, &res);
        return res;
    }
    
    std::string print(uint32_t instr, uint32_t cia) override {
        std::string res;
        ppu_dasm<DasmMode::Print>(&instr, cia, &res);
        return res;
    }
    
    InstructionInfo analyze(uint32_t instr, uint32_t cia) override {
        return ::analyze(instr, cia);
    }
    
    std::vector<SubElfInfo> codeSegments() override {
        std::vector<uint32_t> eps;
        if (!_islib) {
            auto epDescr = (fdescr*)&_mapped[_elf.entryPoint() - _vaBase];
            eps.push_back(epDescr->va);
        }
        return {{_vaBase, _mappedSize, &_mapped[0], eps}};
    }
};

class SPURewriter : public IRewriter {
    std::vector<SubElfInfo> _subs;
    std::vector<uint8_t> _mapped;
    
public:
    void map(std::string elf, uint32_t imageBase) override {
        auto body = read_all_bytes(elf);
        auto elfs = discoverEmbeddedSpuElfs(body);
        if (!elfs.empty() && elfs[0].start == 0) {
            // the file itself is an spu elf
            _subs.push_back({0, (uint32_t)body.size(), &_mapped[0], {0}});
            return;
        }
        
        ELFLoader elfLoader;
        elfLoader.load(elf);
        uint32_t vaBase, mappedSize;
        RewriterStore rewriterStore;
        elfLoader.map([&](auto va, auto size, auto index) {
            if (index == 0) {
                vaBase = va;
                mappedSize = size;
            }
        }, imageBase, "", &rewriterStore);
        
        body.resize(mappedSize);
        g_state.mm->readMemory(vaBase, &body[0], body.size());
        elfs = discoverEmbeddedSpuElfs(body);
        for (auto& elf : elfs) {
            _subs.push_back({elf.startOffset, 0, &_mapped[elf.startOffset], {0}});
        }
    }
        
    std::string rewrite(uint32_t instr, uint32_t cia) override {
        std::string res;
        SPUDasm<DasmMode::Rewrite>(&instr, cia, &res);
        return res;
    }
    
    std::string print(uint32_t instr, uint32_t cia) override {
        std::string res;
        SPUDasm<DasmMode::Print>(&instr, cia, &res);
        return res;
    }
    
    InstructionInfo analyze(uint32_t instr, uint32_t cia) override {
        return ::analyzeSpu(instr, cia);
    }
    
    std::vector<SubElfInfo> codeSegments() override {
        return _subs;
    }
};

void HandleRewrite(RewriteCommand const& command) {
    MainMemory mm;
    if (!g_state.mm) {
        g_state.mm = &mm;
    }
    
    std::ofstream log("/tmp/" + path(command.elf).filename().string() + "_rewriter_log");
    assert(log.is_open());
    
    auto rewriter = std::unique_ptr<IRewriter>(command.isSpu ? (IRewriter*)new SPURewriter : new PPURewriter);
    rewriter->map(command.elf, command.imageBase);
    auto segments = rewriter->codeSegments();
    
    std::vector<SegmentInfo> segmentInfos;
    
    for (auto& segment : segments) {
        log << ssnprintf("analyzing segment %x-%x\n", segment.va, segment.va + segment.len);
        
        std::stack<uint32_t> leads;
        for (auto ep : segment.entryPoints) {
            leads.push(ep);
        }
        
        for (auto ep : command.entryPoints) {
            leads.push(ep + command.imageBase);
        }
        
        auto load32 = [&](auto ea) {
            return *(big_uint32_t*)&segment.ptr[ea - segment.va];
        };
        
        auto blocks = discoverBasicBlocks(
            segment.va,
            segment.len,
            command.imageBase,
            leads,
            log,
            [&](uint32_t cia) { return rewriter->analyze(load32(cia), cia); });
        
        for (auto& block : blocks) {
            for (auto cia = block.start; cia < block.start + block.len; cia += 4) {
                std::string rewritten, printed;
                auto instr = endian_reverse(load32(cia));
                try {
                    rewritten = rewriter->rewrite(instr, cia);
                    printed = rewriter->print(instr, cia);
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
        
        SegmentInfo info;
        info.blocks = blocks;
        info.isSpu = command.isSpu;
        info.suffix = ssnprintf("%x_%x", segment.va, segment.len);
        segmentInfos.push_back(info);
    }

    std::ofstream f(command.cpp.c_str());
    assert(f.is_open());
    
    f << header;
    
    for (auto& segment : segmentInfos) {
        f << ssnprintf("void entryPoint_%s(%sThread*, int);\n",
                       segment.suffix,
                       segment.isSpu ? "SPU" : "PPU");
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
        for (auto& segment : segmentInfos) {
            auto entryName = ssnprintf("entryPoint_%s", segment.suffix);
            auto ppuEntry = segment.isSpu ? "nullptr" : entryName;
            auto spuEntry = segment.isSpu ? entryName : "nullptr";
            auto blocks = ssnprintf("&segment_%s_blocks", segment.suffix);
            f << ssnprintf("    %s, %s, \"%s\", %s,\n",
                           ppuEntry, spuEntry, segment.suffix, blocks);
        }
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
        f << ssnprintf("void entryPoint_%s(%sThread* thread, int label) {\n",
                       segment.suffix,
                       segment.isSpu ? "SPU" : "PPU");
        f << "    static void* labels[] = {\n";
        for (auto& block : segment.blocks) {
            f << ssnprintf("        &&_0x%xu,\n", block.start);
        }
        f << "    };\n\n";
        f << "    goto *labels[label];\n\n";
        for (auto& block : segment.blocks) {
            f << ssnprintf("    _0x%xu:\n", block.start);
            for (auto& line : block.body) {
                f << ssnprintf("        %s\n", line);
            }
        }
        f << "}\n";
    }
}
