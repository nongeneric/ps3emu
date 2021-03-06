#include "ps3tool.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/ppu/ppu_dasm_forms.h"
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
#include "ps3emu/utils/ranges.h"

#include <boost/endian/arithmetic.hpp>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>
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
namespace fs = std::filesystem;

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
#define LOG_FUNC(x) { INFO(libs) << sformat("REWRITER FUNC: {:x}", x); }
#define LOG_RET(lr) { INFO(libs) << sformat("REWRITER RET: {:x}", lr); }
#define LOG_RET_TO(x) { INFO(libs) << sformat("REWRITER RET TO: {:x}", x); }
#define LOG_CALL(from, to) { INFO(libs) << sformat("REWRITER CALL: {:x} -> {:x}", from, to); }
#define LOG_VMENTER(x) { INFO(libs) << sformat("REWRITER VMENTER: {:x}", x); }
#else
#define LOG_FUNC(x)
#define LOG_RET(lr)
#define LOG_RET_TO(x)
#define LOG_CALL(from, to)
#define LOG_VMENTER(x)
#endif

)";

constexpr int g_minExtractableBlockSize = 10;

struct BasicBlockInfo {
    uint32_t start = 0;
    uint32_t len = 0;
    std::vector<std::string> body;
    uint32_t label = 0;
    bool extractable = false;
};

struct SegmentInfo {
    std::string description;
    std::string suffix;
    bool isSpu;
    std::vector<BasicBlockInfo> blocks;
};

template <class F>
std::tuple<std::optional<uint32_t>, uint32_t> outOfPartTargets(
    BasicBlock const& block, std::vector<BasicBlock> const& part, F analyze)
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

struct SimpleInstruction {
    uint32_t cia;
    uint32_t bytes;
};

struct LoadStoreStatementSource {
    uint32_t reg;
    uint32_t size;
};

struct LoadStoreStatement {
    SimpleInstruction simple;
    std::vector<LoadStoreStatementSource> src;
    uint32_t destReg;
    int disp;
    bool load;

    int size() const {
        int res = 0;
        for (auto& s : src) {
            res += s.size;
        }
        return res;
    }
};

typedef boost::variant<
    SimpleInstruction,
    LoadStoreStatement
> BasicBlockStatement;

struct ConcreteBasicBlock {
    std::vector<BasicBlockStatement> statements;
};

class StatementPrinter : public boost::static_visitor<> {
public:
    mutable std::vector<std::string> result;

    void operator()(SimpleInstruction& simple) const {
        std::string rewritten, printed;
        big_uint32_t instr = g_state.mm->load32(simple.cia);
        ppu_dasm<DasmMode::Rewrite>(&instr, simple.cia, &rewritten);
        ppu_dasm<DasmMode::Print>(&instr, simple.cia, &printed);
        result = {sformat("/*{:8x}: {:<24}*/ log(0x{:x}, TH); {};",
                            simple.cia,
                            printed,
                            simple.cia,
                            rewritten)};
    }

    void operator()(LoadStoreStatement& store) const {
        if (store.src.size() == 1) {
            (*this)(store.simple);
            return;
        }
        result.clear();
        result.push_back(sformat("{{ // {} {} ({} bytes)",
                                   store.load ? "LOAD" : "STORE",
                                   store.src.size(),
                                   store.size()));
        result.push_back("    struct __attribute__((packed)) {");
        int i = 0;
        for (auto& s : store.src) {
            result.push_back(sformat("        uint{}_t r{};", s.size * 8, i));
            i++;
        }
        result.push_back("    } t;");
        auto dest = store.destReg == 0 ? "0" : sformat("TH->getGPR({})", store.destReg);
        if (store.load) {
            result.push_back(sformat("    MM->readMemory({} + ({}), &t, {});", dest, store.disp, store.size()));
            i = 0;
            for (auto& s : store.src) {
                result.push_back(sformat(
                    "    TH->setGPR({}, fast_endian_reverse(t.r{}));", s.reg, i));
                i++;
            }
        } else {
            i = 0;
            for (auto& s : store.src) {
                result.push_back(sformat(
                    "    t.r{} = fast_endian_reverse((uint{}_t)TH->getGPR({}));",
                    i,
                    s.size * 8,
                    s.reg));
                i++;
            }
            result.push_back(sformat("    MM->writeMemory({} + ({}), &t, {});", dest, store.disp, store.size()));
        }
        result.push_back("}");
    }
};

ConcreteBasicBlock transformBasicBlock(BasicBlock const& block) {
    ConcreteBasicBlock res;
    for (auto cia = block.start; cia < block.start + block.len; cia += 4) {
        big_uint32_t instr = g_state.mm->load32(cia);
        std::string name;
        ppu_dasm<DasmMode::Name>(&instr, cia, &name);
        SimpleInstruction simple{cia, instr};
        auto size = (name == "STB" || name == "LBZ") ? 1u
                  : (name == "STH" || name == "LHZ") ? 2u
                  : (name == "STW" || name == "LWZ") ? 4u
                  : (name == "STD" || name == "LDZ") ? 8u
                  : 0u;
        if (size) {
            DForm_3 i{instr};
            auto ra = i.RA_u();
            auto rs = i.RS_u();
            auto d = i.D_s();
            auto isLoad = name[0] == 'L';
            LoadStoreStatement st{simple, {{rs, size}}, ra, d, isLoad};
            res.statements.push_back(st);
        } else {
            res.statements.push_back(simple);
        }
    }
    return res;
}

void optimizeAdjacentLoadStores(ConcreteBasicBlock& block, bool load) {
    std::vector<std::tuple<int, int>> seqs;
    auto len = 0, start = 0, cur = 0;
    for (auto& st : block.statements) {
        auto loadStore = boost::get<LoadStoreStatement>(&st);
        if (loadStore && loadStore->load == load) {
            if (start == -1) {
                start = cur;
            }
            len++;
        } else {
            if (len > 1) {
                seqs.push_back({start, len});
            }
            len = 0;
            start = -1;
        }
        cur++;
    }
    for (auto seq = (int)seqs.size() - 1; seq >= 0; --seq) {
        auto [start, len] = seqs[seq];
        std::vector<LoadStoreStatement> sts;
        for (auto i = start; i < start + len; ++i) {
            sts.push_back(boost::get<LoadStoreStatement>(block.statements[i]));
        }
        std::sort(begin(sts), end(sts), [](auto& a, auto& b) {
            return a.disp < b.disp;
        });
        auto sameDest = std::all_of(begin(sts), end(sts), [&](auto& st) {
            return st.destReg == sts[0].destReg;
        });
        bool overlap = false;
        for (auto i = 1u; i < sts.size(); ++i) {
            overlap |= sts[i - 1].disp + sts[i - 1].size() != sts[i].disp;
        }
        if (!sameDest || overlap)
            continue;
        while (sts.size() != 1) {
            auto& a = sts[0];
            auto& b = sts[1];
            for (auto& s : b.src)
                a.src.push_back(s);
            sts.erase(begin(sts) + 1);
        }
        block.statements.erase(begin(block.statements) + start,
                               begin(block.statements) + start + len);
        block.statements.insert(begin(block.statements) + start, sts[0]);
    }
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

    std::vector<SegmentInfo> infos;
    StatementPrinter printer;
    for (auto i = 0u; i < parts.size(); i++) {
        auto& part = parts[i];

        SegmentInfo info;
        info.isSpu = false;
        info.suffix = sformat("ppu_bin_{}", i);

        for (auto& block : part) {
            auto processed = transformBasicBlock(block);
            optimizeAdjacentLoadStores(processed, true);
            optimizeAdjacentLoadStores(processed, false);
            for (auto& st : processed.statements) {
                boost::apply_visitor(printer, st);
                for (auto& line : printer.result)
                    block.body.push_back(line);
            }
            auto extractable = true;
            auto [target, fallthrough] = outOfPartTargets(block, part, analyzeFunc);
            if (target) {
                block.body.insert(block.body.begin(), "#define SET_NIP SET_NIP_INDIRECT");
                block.body.insert(block.body.begin(), "#undef SET_NIP");
                block.body.insert(block.body.begin(), sformat("// target {:x} leads out of part", *target));
                block.body.push_back("#undef SET_NIP");
                block.body.push_back("#define SET_NIP SET_NIP_INITIAL");
                extractable = false;
            }
            if (fallthrough) {
                block.body.insert(block.body.begin(), sformat("// possible fallthrough {:x} leads out of part", fallthrough));
                block.body.push_back(sformat("SET_NIP_INDIRECT(0x{:x});", fallthrough));
                extractable = false;
            }

            info.blocks.push_back({block.start, block.len, block.body, block.start, extractable});
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

template <class F>
std::vector<BasicBlock> CollectSpuSegmentBasicBlocks(
    std::vector<uint32_t> const& leads,
    Elf32_be_Phdr const& segment,
    std::ofstream& log,
    F spuElfVaToParentVaInCurrentSegment)
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

        auto spuElfVaToParentVaInCurrentSegment = [&, vaBase=vaBase](uint32_t va, Elf32_be_Phdr const& segment) {
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
                    bool extractable = true;

                    if (elf.isJob) { // all jobs are position independent
                        block.body.push_back(sformat(
                            "if (!pic_offset_set) {{ pic_offset = bb_va - 0x{:x}; pic_offset_set = true; }}",
                            block.start));
                    }
                    for (auto cia = block.start; cia < block.start + block.len; cia += 4) {
                        std::string rewritten, printed;
                        big_uint32_t instr = g_state.mm->load32(spuElfVaToParentVaInCurrentSegment(cia, segment));
                        SPUDasm<DasmMode::Rewrite>(&instr, cia, &rewritten);
                        SPUDasm<DasmMode::Print>(&instr, cia, &printed);
                        auto line = sformat("/* {:08x} {:8x}: {:<24}*/ logSPU(0x{:x}, th); {};", instr, cia, printed, cia, rewritten);
                        block.body.push_back(line);
                    }

                    auto [target, fallthrough] = outOfPartTargets(block, part, analyzeFunc);
                    if (target) {
                        block.body.insert(begin(block.body), "#define SPU_SET_NIP SPU_SET_NIP_INDIRECT_PIC");
                        block.body.insert(begin(block.body), "#undef SPU_SET_NIP");
                        block.body.insert(begin(block.body), sformat("// target {:x} leads out of part", *target));
                        block.body.push_back("#undef SPU_SET_NIP");
                        block.body.push_back("#define SPU_SET_NIP SPU_SET_NIP_INITIAL");
                        extractable = false;
                    }
                    if (fallthrough) {
                        block.body.insert(block.body.begin(), sformat("// possible fallthrough {:x} leads out of part", fallthrough));
                        block.body.push_back(sformat("SPU_SET_NIP_INDIRECT_PIC(0x{:x});", fallthrough));
                        extractable = false;
                    }

                    info.blocks.push_back({spuElfVaToParentVaInCurrentSegment(block.start, segment),
                                        block.len,
                                        block.body,
                                        block.start,
                                        extractable});
                }

                info.isSpu = true;
                info.suffix = sformat("spu_{:x}_{:x}_{:x}_part_{}_{}",
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

const char* threadTypeName(bool isSpu) {
    return isSpu ? "SPUThread" : "PPUThread";
}

std::string entrySignature(const SegmentInfo& segment) {
    return sformat("void entryPoint_{}({}* thread, int label{})",
                     segment.suffix,
                     threadTypeName(segment.isSpu),
                     segment.isSpu ? ", uint32_t bb_va" : "");
}

void printHeader(std::string name, std::vector<SegmentInfo> const& segments, bool noFexcept) {
    std::ofstream f(name);
    assert(f.is_open());

    if (noFexcept) {
        f << "#define EMU_REWRITER_NOFEXCEPT\n";
    }
    f << header;

    for (auto& segment : segments) {
        f << entrySignature(segment) << ";\n";
    }

    for (auto& segment : segments) {
        f << sformat("extern std::vector<RewrittenBlock> segment_{}_blocks;\n", segment.suffix);
    }

    for (auto& segment : segments) {
        f << sformat("extern RewrittenSegment segment_{};\n", segment.suffix);
    }
}

void printMain(std::string name, std::vector<SegmentInfo> const& segments) {
    std::ofstream f(name);
    assert(f.is_open());

    std::string text = mainText;
    replace_first(text, "{headerName}", fs::path(name).stem().string());
    f << text,

    f << "extern \"C\" {\n";
    f << "    RewrittenSegmentsInfo info {\n";
    f << "        {\n";
    for (auto& segment : segments) {
        f << sformat("            segment_{},\n", segment.suffix);
    }
    f << "        }, \n";
    f << "        init\n";
    f << "    };\n";
    f << "};\n\n";
}

void printSegment(std::string baseName, std::string name, SegmentInfo const& segment) {
    std::ofstream f(name);
    assert(f.is_open());

    f << sformat("#include \"{}.h\"\n\n", baseName);

    f << sformat("std::vector<RewrittenBlock> segment_{}_blocks {{\n", segment.suffix);
    for (auto& block : segment.blocks) {
        f << sformat("    {{ 0x{:x}, 0x{:x} }},\n", block.start, block.len);
    }
    f << "};\n";

    f << sformat("RewrittenSegment segment_{} {{\n", segment.suffix);
    auto entryName = sformat("entryPoint_{}", segment.suffix);
    auto ppuEntry = segment.isSpu ? "nullptr" : entryName;
    auto spuEntry = segment.isSpu ? entryName : "nullptr";
    auto blocks = sformat("&segment_{}_blocks", segment.suffix);
    f << sformat("    {}, {}, \"{}\", {}\n",
                   ppuEntry,
                   spuEntry,
                   segment.suffix,
                   blocks);
    f << "};\n";

    auto shouldExtract = [&] (auto& block) { return block.extractable && block.body.size() >= g_minExtractableBlockSize; };

    for (auto& block : segment.blocks | ranges::views::filter(shouldExtract)) {
        f << sformat(
            "void extracted_block_{}_{:x}({}* thread, uint32_t& pic_offset) {{\n",
            segment.suffix,
            block.label,
            threadTypeName(segment.isSpu));

        for (auto& line : block.body | ranges::views::drop_last(1)) {
            f << sformat("        {}\n", line);
        }
        f << "}\n";
    }

    f << entrySignature(segment) << " {\n";

    if (!segment.blocks.empty()) {
        f << "    static void* labels[] = {\n";
        for (auto& block : segment.blocks) {
            f << sformat("        &&_0x{:x}u,\n", block.label);
        }
        f << "    };\n\n";
        f << "    uint32_t pic_offset = 0;\n";
        f << "    bool pic_offset_set = false;\n";
        f << "    goto *labels[label];\n\n";
    }

    for (auto& block : segment.blocks) {
        f << sformat("    _0x{:x}u:\n", block.label);
        if (shouldExtract(block)) {
            f << sformat("        extracted_block_{}_{:x}(thread, pic_offset);\n",
                           segment.suffix,
                           block.label);
        }
        auto body = shouldExtract(block) ? ranges::any_view(ranges::views::single(block.body.back()))
                                         : ranges::any_view(ranges::views::all(block.body));
        for (auto& line : body) {
            f << sformat("        {}\n", line);
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
    auto baseName = fs::path(name).stem().string();
    for (auto i = 0u; i < segments.size(); ++i) {
        auto in = sformat("{}.segment_{}.cpp", baseName, i);
        auto out = sformat("{}.segment_{}.o", baseName, i);
        script.statement("compile", in, out, {});
        objects += out + " ";
    }
    auto inMain = sformat("{}.cpp", baseName);
    auto outMain = sformat("{}.o", baseName);
    script.statement("compile", inMain, outMain, {});
    objects += outMain;

    auto out = sformat("{}.so", baseName);
    script.statement("link", objects, out, {});

    std::ofstream f(name);
    assert(f.is_open());
    f << script.dump();
}

void HandleRewrite(RewriteCommand const& command) {
    g_state.init();
    MainMemory mm;
    if (!g_state.mm) {
        g_state.mm = &mm;
    }
    InternalMemoryManager memalloc(EmuInternalArea, EmuInternalAreaSize, "internal alloc");
    if (!g_state.memalloc) {
        g_state.memalloc = &memalloc;
    }

    std::ofstream log("/tmp/" + fs::path(command.elf).filename().string() + "_rewriter_log");
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

    printHeader(headerName, segmentInfos, command.noFexcept);
    printMain(mainName, segmentInfos);
    printNinja(ninjaName, segmentInfos);
    for (auto i = 0u; i < segmentInfos.size(); ++i) {
        auto segmentName = sformat("{}.segment_{}.cpp", command.cpp, i);
        printSegment(fs::path(headerName).stem(), segmentName, segmentInfos[i]);
    }
}
