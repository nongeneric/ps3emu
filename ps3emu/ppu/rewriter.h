#pragma once

#include <stdint.h>
#include <functional>

class PPUThread;

struct RewrittenBlock {
    uint32_t va;
    uint32_t len;
};

struct RewrittenBlocks {
    void (*entryPoint)(PPUThread*, int);
    RewrittenBlock* blocks;
    unsigned count;
    std::function<void()> init;
};
