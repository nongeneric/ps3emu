#include "sceNp2.h"

#include "../../ps3emu/log.h"

int32_t sceNp2Init(uint32_t poolsize, ps3_uintptr_t poolptr) {
    LOG << __FUNCTION__;
    return CELL_OK;
}

int32_t sceNp2Term() {
    LOG << __FUNCTION__;
    return CELL_OK;
}
