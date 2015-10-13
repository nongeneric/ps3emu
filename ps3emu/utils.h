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

template <typename S, typename D>
union union_cast {
    static_assert(sizeof(S) == sizeof(D), "invalid cast");
    union_cast(S s) : _s(s) { }
    operator D() const {
        return _d;
    }
private:
    S _s;
    D _d;
};