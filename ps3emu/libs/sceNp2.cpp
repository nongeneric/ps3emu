#include "sceNp2.h"

#include <boost/log/trivial.hpp>

int32_t sceNp2Init(uint32_t poolsize, ps3_uintptr_t poolptr) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

int32_t sceNp2Term() {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}
