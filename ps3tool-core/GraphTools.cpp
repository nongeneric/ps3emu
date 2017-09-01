#include "GraphTools.h"

#include "ps3emu/utils.h"
#include <metis.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <assert.h>

using namespace boost;

Graph buildGraph(BasicBlockRange blocks, analyze_t analyze) {
    Graph g;
    for (auto& bb : blocks) {
        auto v = add_vertex(g);
        g[v].p->block = bb;
    }
    std::map<uint32_t, Graph::vertex_descriptor> startMap;
    for (auto [v, vend] = vertices(g); v != vend; ++v) {
        startMap[g[*v].p->block.start] = *v;
    }
    auto weights = get(&EdgeProperties::weight, g);
    for (auto [v, vend] = vertices(g); v != vend; ++v) {
        auto lastVa = g[*v].p->block.start + g[*v].p->block.len - 4;
        auto info = analyze(lastVa);
        std::vector<uint32_t> targets;
        if (info.flow) {
            if (info.target) {
                targets.push_back(*info.target);
            }
            if (info.passthrough) {
                targets.push_back(lastVa + 4);
            }
        }
        for (auto target : targets) {
            auto tv = startMap.find(target);
            if (tv == startMap.end()) {
                // target might point out of the current segment in SPU
                // in this case the branch is treated as indirect and
                // will be handled when rewriting basic blocks
                continue;
            }
            auto [edge, res] = add_edge(*v, tv->second, g);
            (void)res;
            // try not to cut fallthroughs if possible
            weights[edge] = target == lastVa + 4 ? 5 : 1;
        }
    }
    return g;
}

std::tuple<std::vector<idx_t>, std::vector<idx_t>, std::vector<idx_t>> toCSR(Graph& g) {
    std::vector<idx_t> vs(num_vertices(g) + 1), es(2 * num_edges(g)), ews(2 * num_edges(g));
    auto edgeWeightMap = get(&EdgeProperties::weight, g);
    auto edge = 0;
    for (auto [v, vend] = vertices(g); v != vend; ++v) {
        vs.at(*v) = edge;
        for (auto [e, eend] = out_edges(*v, g); e != eend; ++e) {
            es.at(edge) = target(*e, g);
            ews.at(edge) = edgeWeightMap[*e];
            edge++;
        }
        for (auto [e, eend] = in_edges(*v, g); e != eend; ++e) {
            es.at(edge) = source(*e, g);
            ews.at(edge) = edgeWeightMap[*e];
            edge++;
        }
    }
    vs.back() = edge;
    return {vs, es, ews};
}

std::vector<std::vector<BasicBlock>> partitionBasicBlocks(BasicBlockRange blocks,
                                                          analyze_t analyze,
                                                          uint32_t partSize)
{
    auto g = buildGraph(blocks, analyze);
    auto [vs, es, ews] = toCSR(g);
    assert(vs.size() - 1 == num_vertices(g));
    idx_t objval, nvert = num_vertices(g), ncon = 100, nparts = distance(blocks) / partSize;
    if (nparts <= 1) {
        return {std::vector<BasicBlock>(begin(blocks), end(blocks))};
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
    int i = 0;
    for (auto [v, vend] = vertices(g); v != vend; ++v, ++i) {
        res.at(partNumbers.at(i)).push_back(g[*v].p->block);
    }
    for (auto& comp : res) {
        std::sort(comp.begin(), comp.end());
    }
    return res;
}
