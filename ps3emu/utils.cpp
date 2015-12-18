#include "utils.h"

#include <boost/thread.hpp>

void ums_sleep(uint64_t microseconds) {
    boost::this_thread::sleep_for( boost::chrono::microseconds(microseconds) );
}