#include "libSync.h"

#include "ps3emu/MainMemory.h"
#include "../sys.h"

int32_t cellSyncMutexInitialize(uint32_t m, MainMemory* mm) {
    mm->store<4>(m, 0);
    return CELL_OK;
}

int32_t cellSyncMutexTryLock(int32_t m) {
    assert(false);
    return CELL_OK;
}

int32_t cellSyncMutexLock(int32_t m) {
    assert(false);
    return CELL_OK;
}

int32_t cellSyncMutexUnlock(int32_t m) {
    assert(false);
    return CELL_OK;
}
