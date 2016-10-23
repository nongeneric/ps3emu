#pragma once

#include <functional>

class EmuCallbacks {
public:
    std::function<void(const char*, unsigned)> stdout = nullptr;
    std::function<void(const char*, unsigned)> stderr = nullptr;
    std::function<void(const char*, unsigned)> spustdout = nullptr;
};
