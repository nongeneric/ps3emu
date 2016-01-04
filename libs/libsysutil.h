#pragma once

#include "sys_defs.h"
#include "../ps3emu/Process.h"

class MainMemory;

int32_t cellSysutilRegisterCallback(int32_t slot, ps3_uintptr_t callback, ps3_uintptr_t userdata);
int32_t cellSysutilCheckCallback();
emu_void_t cellGcmSetDebugOutputLevel(int32_t level);

int32_t cellSysutilGetSystemParamInt(int32_t id, big_int32_t *value);
int32_t cellSysutilGetSystemParamString(int32_t id, ps3_uintptr_t buf, uint32_t bufsize, MainMemory* mm);

enum CellSysCacheParamSize {
    CELL_SYSCACHE_ID_SIZE           = 32,
    CELL_SYSCACHE_PATH_MAX          = 1055
};

struct CellSysCacheParam {
    char cacheId[CELL_SYSCACHE_ID_SIZE];
    char getCachePath[CELL_SYSCACHE_PATH_MAX];
    uint32_t reserved;
};

int32_t cellSysCacheMount(CellSysCacheParam* param, Process* proc);
int32_t cellSysCacheClear(Process* proc);
