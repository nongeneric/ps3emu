#pragma once

#include <string>
#include <stdio.h>
#include <inttypes.h>

template <typename... Args>
std::string ssnprintf(const char* f, Args... args) {
    char buf[300];
    snprintf(buf, sizeof buf, f, args...);
    return std::string(buf);
}