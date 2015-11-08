#pragma once

#include "sys_defs.h"
#include "../ps3emu/constants.h"
#include <stdint.h>

//TODO replace all int by int32_t for ncall functions

int32_t cellSysutilRegisterCallback(int32_t slot, ps3_uintptr_t callback, ps3_uintptr_t userdata);
int32_t cellSysutilCheckCallback();
