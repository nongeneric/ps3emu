#include "GraphTools.h"

#include "ps3emu/utils.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/stoer_wagner_min_cut.hpp>
#include <boost/graph/graphviz.hpp>
#include <metis.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <assert.h>

using namespace boost;
using Graph = adjacency_list<vecS,
                             vecS,
                             undirectedS,
                             no_property,
                             property<edge_weight_t, int>>;

Graph buildGraph(std::vector<BasicBlock> const& blocks, analyze_t analyze) {
    Graph g(blocks.size());
    auto weights = get(edge_weight, g);
    auto blockIndex = 0u;
    for (auto& bb : blocks) {
        auto lastVa = bb.start + bb.len - 4;
        auto info = analyze(lastVa);
        std::vector<uint32_t> targets;
        if (info.flow && info.target) {
            targets.push_back(*info.target);
        }
        targets.push_back(lastVa + 4);
        for (auto target : targets) {
            auto it = std::find_if(blocks.begin(), blocks.end(), [&](auto& block) {
                return block.start == target;
            });
            if (it == blocks.end()) {
                assert(target == lastVa + 4);
                continue;
            }
            auto targetIndex = std::distance(blocks.begin(), it);
            auto [edge, res] = add_edge(blockIndex, targetIndex, g);
            (void)res;
            // try not to cut fallthroughs if possible
            weights[edge] = target == lastVa + 4 ? 5 : 1;
        }
        blockIndex++;
    }
    return g;
}

std::tuple<std::vector<idx_t>, std::vector<idx_t>, std::vector<idx_t>> toCSR(Graph& g) {
    std::vector<idx_t> vs(num_vertices(g) + 1), es(2 * num_edges(g)), ews(2 * num_edges(g));
    auto edgeWeightMap = get(edge_weight, g);
    auto edge = 0;
    for (auto [v, vend] = vertices(g); v != vend; ++v) {
        vs.at(*v) = edge;
        for (auto [e, eend] = out_edges(*v, g); e != eend; ++e) {
            es.at(edge) = target(*e, g);
            ews.at(edge) = edgeWeightMap[*e];
            edge++;
        }
    }
    vs.back() = edge;
    return {vs, es, ews};
}

std::vector<std::vector<BasicBlock>> partitionBasicBlocks(
    std::vector<BasicBlock> const& blocks, analyze_t analyze)
{
    auto g = buildGraph(blocks, analyze);
    auto [vs, es, ews] = toCSR(g);
    assert(vs.size() - 1 == num_vertices(g));
    idx_t objval, nvert = num_vertices(g), ncon = 100, nparts = blocks.size() / 800;
    if (nparts <= 1) {
        return {blocks};
    }
    std::vector<idx_t> partNumbers(num_vertices(g));
    auto error = METIS_PartGraphRecursive(&nvert,
                                          &ncon,
                                          &vs[0],
                                          &es[0],
                                          NULL,
                                          NULL,
                                          &ews[0],
                                          &nparts,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &objval,
                                          &partNumbers[0]);
    if (error != METIS_OK)
        throw std::runtime_error("metis error");
    std::vector<std::vector<BasicBlock>> res(nparts);
    for (auto i = 0u; i < num_vertices(g); ++i) {
        res.at(partNumbers.at(i)).push_back(blocks.at(i));
    }
    for (auto& comp : res) {
        std::sort(comp.begin(), comp.end());
    }
    return res;
}
