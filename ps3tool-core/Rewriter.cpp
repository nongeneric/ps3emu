#include "Rewriter.h"

#include "ps3emu/utils.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/spu/SPUDasm.h"
#include "ps3tool-core/GraphTools.h"
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/vector_property_map.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/transpose_graph.hpp>
#include <optional>
#include <set>
#include <algorithm>
#include <vector>

class CodeMap {
    std::vector<bool> _map;
    uint32_t _start;
    
public:
    CodeMap(uint32_t start, uint32_t len) : _map(len / 4), _start(start) { }
    
    bool get(uint32_t va) const {
        assert(inrange(va));
        auto i = (va - _start) / 4u;
        return _map[i];
    }
    
    void mark(uint32_t va) {
        assert(inrange(va));
        auto i = (va - _start) / 4u;
        _map[i] = true;
    }
    
    bool inrange(uint32_t va) const {
        assert(va % 4 == 0);
        auto i = (va - _start) / 4u;
        return 0u <= i && i < _map.size();
    }
};

CodeMap DiscoverBreaks(uint32_t start, uint32_t len, analyze_t analyze) {
    CodeMap map(start, len);
    for (auto cia = start; cia != start + len; cia += 4) {
        auto info = analyze(cia);
        if (info.target && map.inrange(*info.target)) {
            map.mark(*info.target);
        }
        if (info.flow && map.inrange(cia + 4)) {
            map.mark(cia + 4);
        }
    }
    return map;
}

std::set<BasicBlock> MakeBasicBlocks(uint32_t start, uint32_t len, CodeMap const& breaks) {
    std::set<BasicBlock> blocks;
    BasicBlock block;
    block.start = start;
    for (auto cia = start; cia < start + len; cia += 4) {
        if (breaks.get(cia) && block.len) {
            blocks.insert(block);
            block = BasicBlock();
            block.start = cia;
        }
        block.len += 4;
    }
    if (block.len) {
        blocks.insert(block);
    }
    return blocks;
}

void SplitBlocks(std::set<BasicBlock>& blocks, uint32_t at) {
    for (auto it = begin(blocks); it != end(blocks); ++it) {
        if (intersects(it->start, it->len, at, 4u)) {
            if (it->start == at)
                return;
            BasicBlock left, right;
            left.start = it->start;
            left.len = at - left.start;
            right.start = at;
            right.len = it->len - left.len;
            blocks.erase(it);
            blocks.insert(left);
            blocks.insert(right);
            return;
        }
    }
    WARNING(libs) << ssnprintf("splitting at %x is out of range, add this address to branches\n", at);
}

bool containsIllegalInstruction(BasicBlock const& block, validate_t validate) {
    for (auto cia = block.start; cia != block.start + block.len; cia += 4) {
        if (!validate(cia))
            return true;
    }
    return false;
}

template <typename G>
class CustomDSFVisitor : public boost::default_dfs_visitor {
    std::function<void(typename G::vertex_descriptor)> _discover;
public:
    CustomDSFVisitor(std::function<void(typename G::vertex_descriptor)> discover)
        : _discover(discover) {}
    void discover_vertex(auto v, G const&) {
        _discover(v);
    }
};

template <typename G>
Graph removeInvalidVertices(G& graph) {
    std::function<bool(typename G::vertex_descriptor)> vfilter = [&](auto v) {
        return !graph[v].p->invalid;
    };
    std::function<bool(typename G::edge_descriptor)> efilter = [&](auto) {
        return true;
    };
    auto filtered = boost::filtered_graph(graph, efilter, vfilter);
    Graph r;
    boost::copy_graph(filtered, r);
    return r;
}

Graph removeVertices(Graph& graph, auto removeIf) {
    auto transposed = boost::make_reverse_graph(graph);
    CustomDSFVisitor<decltype(transposed)> visitor([&](auto v) { transposed[v].p->invalid = true; });
    std::vector<boost::default_color_type> colors(num_vertices(transposed));
    for (auto [v, vend] = vertices(transposed); v != vend; ++v) {
        if (removeIf(transposed[*v].p->block, transposed, *v)) {
            for (auto& color : colors) {
                color = boost::white_color;
            }
            boost::iterator_property_map colorMap(begin(colors), get(boost::vertex_index, transposed));
            depth_first_visit(transposed, *v, visitor, colorMap);
        }
    }
    return removeInvalidVertices(graph);
}

template<typename C>
C graphToBlocks(auto const& graph) {
    C blocks;
    for (auto [v, vend] = vertices(graph); v != vend; ++v) {
        blocks.insert(end(blocks), graph[*v].p->block);
    }
    return blocks;
}

Graph removeLoadStoreTargets(Graph& graph, analyze_t analyze, name_t name) {
    std::set<uint32_t> targets;
    for (auto [v, vend] = vertices(graph); v != vend; ++v) {
        auto& block = graph[*v].p->block;
        for (auto ip = block.start; ip <  block.start + block.len; ip += 4u) {
            auto n = name(ip);
            if (n == "spu_lqa" || n == "spu_stqa" || n == "spu_lqr" || n == "spu_stqr") {
                auto info = analyze(ip);
                targets.insert(info.inputs[0].imm);
            }
        }
    }
    return removeVertices(graph, [&](auto& block, auto& g, auto v) {
        for (auto target : targets) {
            if (intersects(block.start, block.len, target, 4u))
                return true;
        }
        return false;
    });
}

Graph removeLonelyStops(Graph& graph, name_t name) {
    for (auto [v, vend] = vertices(graph); v != vend; ++v) {
        auto& block = graph[*v].p->block;
        if (block.len != 4)
            continue;
        auto n = name(block.start);
        if (n == "spu_stop" || n == "spu_stopd") {
            graph[*v].p->invalid = true;
        }
    }
    return removeInvalidVertices(graph);
}

std::vector<BasicBlock> discoverBasicBlocks(
    uint32_t start,
    uint32_t length,
    std::set<uint32_t> leads,
    std::ostream& log,
    analyze_t analyze,
    validate_t validate,
    std::optional<name_t> name,
    std::optional<read_t> read,
    bool confidence)
{
    for (auto& lead : leads) {
        if (!subset(lead, 4u, start, length))
            throw std::runtime_error("a lead is outside the segment");
    }
    
    auto breaks = DiscoverBreaks(start, length, analyze);
    for (auto lead : leads) {
        breaks.mark(lead);
    }
    
    auto blocks = MakeBasicBlocks(start, length, breaks);
    auto graph = buildGraph(blocks, analyze);
    graph = removeVertices(graph, [&](auto& block, auto& g, auto v) {
        return containsIllegalInstruction(block, validate);
    });
    
    if (confidence) {
        blocks = graphToBlocks<std::set<BasicBlock>>(graph);
        std::vector<std::tuple<uint32_t, uint32_t>> jumpTables;
        std::vector<uint32_t> newBreaks;
        for (auto& block : blocks) {
            auto breaks1 =
                discoverIndirectLocations(block.start, block.len, analyze, *name);
            std::copy(begin(breaks1),
                        end(breaks1),
                        std::inserter(newBreaks, begin(newBreaks)));
            auto[table, len] = discoverJumpTable(
                block.start, block.len, start, length, analyze, *name, *read);
            if (len) {
                jumpTables.push_back({table, len});
                newBreaks.push_back(table + len);
                for (auto i = table; i < table + len; i += 4) {
                    newBreaks.push_back((*read)(i));
                }
            }
        }
        for (auto b : newBreaks) {
            auto aligned = ::align(b, 4u);
            if (breaks.inrange(aligned)) {
                SplitBlocks(blocks, aligned);
            }
        }
        graph = buildGraph(blocks, analyze);
        graph = removeVertices(graph, [&](auto& block, auto& g, auto v) {
            for (auto[table, len] : jumpTables) {
                if (intersects(block.start, block.len, table, len))
                    return true;
            }
            return false;
        });
        graph = removeLoadStoreTargets(graph, analyze, *name);
        graph = removeLonelyStops(graph, *name);
    }
    auto blockVec = graphToBlocks<std::vector<BasicBlock>>(graph);
    
    std::sort(begin(blockVec), end(blockVec));
    return blockVec;
}

uint32_t vaToOffset(const Elf32_be_Ehdr* header, uint32_t va) {
    auto phs = (Elf32_be_Phdr*)((char*)header + header->e_phoff);
    for (auto i = 0; i < header->e_phnum; ++i) {
        auto vaStart = phs[i].p_vaddr; 
        auto vaEnd = vaStart + phs[i].p_memsz;
        if (vaStart <= va && va < vaEnd) {
            if (va - vaStart > phs[i].p_filesz)
                throw std::runtime_error("out of range");
            return va - vaStart + phs[i].p_offset;
        }
    }
    throw std::runtime_error("out of range");
}

std::vector<EmbeddedElfInfo> discoverEmbeddedSpuElfs(std::vector<uint8_t> const& elf) {
    std::vector<uint8_t> magic { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3 };
    std::vector<EmbeddedElfInfo> elfs;
    auto it = begin(elf);
    for (;;) {
        it = std::search(it, end(elf), begin(magic), end(magic));
        if (it == end(elf))
            break;
        auto elfit = it++;
        auto header = (const Elf32_be_Ehdr*)&*elfit;
        if (header->e_ident[EI_CLASS] != ELFCLASS32)
            continue;
        if (sizeof(Elf32_be_Ehdr) != header->e_ehsize)
            continue;
        if (sizeof(Elf32_be_Phdr) != header->e_phentsize)
            continue;
        if (sizeof(Elf32_be_Shdr) != header->e_shentsize)
            continue;
        uint32_t end = 0;
        auto phs = (Elf32_be_Phdr*)((char*)header + header->e_phoff);
        for (auto i = 0; i < header->e_phnum; ++i) {
            end = std::max(end, phs[i].p_offset + phs[i].p_filesz);
        }
        if (header->e_shnum) {
            auto shs = (Elf32_be_Shdr*)((char*)header + header->e_shoff);
            for (auto i = 0; i < header->e_shnum; ++i) {
                end = std::max(end, shs[i].sh_addr + shs[i].sh_size);
            }
        }
        bool isJob = false;
        if (header->e_entry == 0x10) {
            auto ep = (big_uint32_t*)((char*)header + vaToOffset(header, header->e_entry));
            isJob = ep[0] == 0x44012850 && ep[1] == 0x32000080 && ep[2] == 0x44012850 && ep[4] == 0x62696E32;
        }
        elfs.push_back({(uint32_t)std::distance(begin(elf), elfit), end, isJob, header});
    }
    return elfs;
}

std::set<uint32_t> discoverIndirectLocations(uint32_t start,
                                             uint32_t len,
                                             analyze_t analyze,
                                             name_t name) {
    assert(len >= 4);
    std::set<OperandInfo> marked;
    std::set<uint32_t> labels;
    for (auto ip = start + len - 4; ip > start; ip -= 4) {
        auto info = analyze(ip);
        if (info.flow && !info.target && info.extInfo) {
            marked.insert(info.inputs[0]);
        } else {
            auto n = name(ip);
            if (n == "spu_selb") {
                auto it = marked.find(info.outputs[0]);
                if (it != end(marked)) {
                    marked.insert(info.inputs[0]);
                    marked.insert(info.inputs[1]);
                }
            } else if (n == "spu_ila") {
                auto it = marked.find(info.outputs[0]);
                if (it != end(marked)) {
                    assert(info.inputs[0].type == OperandType::imm);
                    if (info.inputs[0].imm % 4 == 0) {
                        labels.insert(info.inputs[0].imm);
                    }
                }
            }
        }
    }
    return labels;
}

std::tuple<uint32_t, uint32_t> discoverJumpTable(uint32_t start,
                                                 uint32_t len,
                                                 uint32_t segmentStart,
                                                 uint32_t segmentLen,
                                                 analyze_t analyze,
                                                 name_t name,
                                                 read_t read) {
    uint32_t table = 0, tlen = 0;
    for (auto ip = start; ip < start + len; ip += 4) {
        auto n = name(ip);
        if (n == "spu_ila") {
            auto info = analyze(ip);
            assert(info.extInfo);
            assert(info.inputs[0].type == OperandType::imm);
            table = info.inputs[0].imm;
            while (subset(table + tlen, 4u, segmentStart, segmentLen)) {
                auto offset = read(table + tlen);
                if (offset % 4u != 0u)
                    break;
                if (!subset(offset, 4u, segmentStart, segmentLen))
                    break;
                tlen += 4;
            }
            if (tlen)
                break;
        }
    }
    return {table, tlen};
}
