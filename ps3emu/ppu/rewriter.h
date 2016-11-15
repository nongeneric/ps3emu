#pragma once

#include <stdint.h>
#include <functional>

class PPUThread;

struct RewrittenFunction {
    const char* name;
    uint32_t va;
    void (*ptr)(PPUThread*);
};

struct RewrittenFunctions {
    RewrittenFunction* functions;
    unsigned len;
    std::function<void()> init;
};
