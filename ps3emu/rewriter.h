#pragma once

#include <stdint.h>
#include <functional>
#include <vector>

class PPUThread;
class SPUThread;

struct RewrittenBlock {
    uint32_t va;
    uint32_t len;
};

struct RewrittenSegment {
    void (*ppuEntryPoint)(PPUThread*, int);
    void (*spuEntryPoint)(SPUThread*, int);
    const char* description;
    const std::vector<RewrittenBlock>* blocks;
};

struct RewrittenSegmentsInfo {
    std::vector<RewrittenSegment> segments;
    std::function<void()> init;
};
