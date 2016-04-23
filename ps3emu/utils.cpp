#include "utils.h"

#include <boost/thread.hpp>

void ums_sleep(uint64_t microseconds) {
    boost::this_thread::sleep_for( boost::chrono::microseconds(microseconds) );
}

std::string print_hex(const void* buf, int len, bool cArray) {
    std::string res;
    auto typed = reinterpret_cast<const uint8_t*>(buf);
    for (auto it = typed; it != typed + len; ++it) {
        res += ssnprintf(cArray ? "0x%02x, " : "%02X ", *it);
    }
    return res;
}