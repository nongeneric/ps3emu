#include "utils.h"

#include <unistd.h>

void ums_sleep(uint64_t microseconds) {
    usleep(microseconds);
}

std::string print_hex(const void* buf, int len, bool cArray) {
    std::string res;
    auto typed = reinterpret_cast<const uint8_t*>(buf);
    for (auto it = typed; it != typed + len; ++it) {
        res += ssnprintf(cArray ? "0x%02x, " : "%02X ", *it);
    }
    return res;
}
