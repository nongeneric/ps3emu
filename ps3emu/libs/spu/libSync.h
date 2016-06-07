#pragma once

#include <stdint.h>

int32_t cellSyncMutexInitialize(uint32_t m);
int32_t cellSyncMutexTryLock(int32_t m);
int32_t cellSyncMutexLock(int32_t m);
int32_t cellSyncMutexUnlock(int32_t m);