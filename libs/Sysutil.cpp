#include "Sysutil.h"
#include "assert.h"

int32_t cellSysutilRegisterCallback(int32_t slot, ps3_uintptr_t callback, ps3_uintptr_t userdata) {
    // TODO: implement
    return CELL_OK;
}

int32_t cellSysutilCheckCallback() {
    // TODO: implement
    return CELL_OK;
}

emu_void_t cellGcmSetDebugOutputLevel(int32_t level) {
    assert(level == 0 || level == 1 || level == 2);
    return emu_void;
}


