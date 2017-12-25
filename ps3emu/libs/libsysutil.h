#pragma once

#include "sys_defs.h"
#include "../Process.h"
#include "../ELFLoader.h"
#include <boost/context/all.hpp>

class MainMemory;

int32_t cellSysutilRegisterCallback(int32_t slot, ps3_uintptr_t callback, ps3_uintptr_t userdata);
int32_t cellSysutilUnregisterCallback(int32_t slot);
int64_t cellSysutilCheckCallback(boost::context::continuation* sink);
emu_void_t cellGcmSetDebugOutputLevel(int32_t level);
int32_t cellVideoOutGetResolutionAvailability(uint32_t videoOut,
                                              uint32_t resolutionId,
                                              uint32_t aspect,
                                              uint32_t option);

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
emu_void_t sys_ppu_thread_once(big_int32_t* once_ctrl, const fdescr* init, PPUThread* th);
int32_t _sys_spu_printf_initialize();
int32_t _sys_spu_printf_finalize();

uint64_t emuCallback(uint32_t va, std::vector<uint64_t> const& args, bool wait);
