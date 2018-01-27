#pragma once

#include <functional>

class IEmuCallbacks {
public:
    virtual ~IEmuCallbacks() = default;
    virtual void stdout(const char* str, int len) = 0;
    virtual void stderr(const char* str, int len) = 0;
    virtual void spustdout(const char* str, int len) = 0;
};
