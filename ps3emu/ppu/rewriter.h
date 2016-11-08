#pragma once

#include <stdint.h>

struct RewrittenFunction {
    const char* name;
    uint32_t va;
};

struct RewrittenFunctions {
    RewrittenFunction* functions;
    unsigned len;
};
