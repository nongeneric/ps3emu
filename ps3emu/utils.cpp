#include "utils.h"

#include <unistd.h>
#include <algorithm>

void nap(std::chrono::microseconds duration) {
    if (duration > std::chrono::milliseconds(50)) {
        struct timespec ts;
        auto us = duration.count();
        ts.tv_sec =  us / 1000;
        ts.tv_nsec = (us % 1000) * 1000000;
        nanosleep(&ts, &ts);
        return;
    }

    auto until = std::chrono::high_resolution_clock::now() + duration;
    while (std::chrono::high_resolution_clock::now() < until) {
    }
}

void ums_sleep(uint64_t microseconds) {
    usleep(microseconds);
}

std::string print_hex(const void* buf, int len, bool cArray) {
    std::string res;
    auto typed = reinterpret_cast<const uint8_t*>(buf);
    for (auto it = typed; it != typed + len; ++it) {
        res += sformat(cArray ? "0x{:02x}, " : "{:02X} ", *it);
    }
    return res;
}

std::string pretty_print_hex(const uint8_t* buffer, size_t size, size_t line) {
    std::string res;
    for (size_t i = 0; i < size; i += line) {
        auto left = std::min(line, size - i);
        for (size_t j = 0; j < left; ++j) {
            res += sformat("{:02x} ", buffer[i + j]);
        }
        res += "\n";
    }
    return res;
}