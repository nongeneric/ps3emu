#pragma once

#include "Rewriter.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/stoer_wagner_min_cut.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/range/any_range.hpp>
#include <vector>
#include <memory>

struct VertexPropertiesImpl {
    int confidence = 0;
    bool invalid = false;
    BasicBlock block;
};

struct VertexProperties {
    VertexProperties() {
        p = std::make_shared<VertexPropertiesImpl>();
    }
    std::shared_ptr<VertexPropertiesImpl> p;
};

struct EdgeProperties {
    int weight = 0;
};

using Graph = boost::adjacency_list<boost::vecS,
                                    boost::vecS,
                                    boost::bidirectionalS,
                                    VertexProperties,
                                    EdgeProperties>;

using BasicBlockRange = boost::any_range<BasicBlock, boost::forward_traversal_tag>;
Graph buildGraph(BasicBlockRange blocks, analyze_t analyze);

std::vector<std::vector<BasicBlock>> partitionBasicBlocks(BasicBlockRange blocks,
                                                          analyze_t analyze,
                                                          uint32_t partSize = 800);
