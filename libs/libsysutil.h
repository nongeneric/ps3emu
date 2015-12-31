#pragma once

#include "sys_defs.h"
#include "../ps3emu/constants.h"
#include <stdint.h>

class MainMemory;

int32_t cellSysutilRegisterCallback(int32_t slot, ps3_uintptr_t callback, ps3_uintptr_t userdata);
int32_t cellSysutilCheckCallback();
emu_void_t cellGcmSetDebugOutputLevel(int32_t level);

int32_t cellSysutilGetSystemParamInt(int32_t id, big_int32_t *value);
int32_t cellSysutilGetSystemParamString(int32_t id, ps3_uintptr_t buf, uint32_t bufsize, MainMemory* mm);
