#pragma once

#include "sys_defs.h"

struct CellNetCtlNetStartDialogParam {
    big_uint32_t size;
    big_int32_t type;
    big_uint32_t cid;
};

int32_t cellNetCtlInit();
int32_t cellNetCtlNetStartDialogLoadAsync(
    const CellNetCtlNetStartDialogParam* param);
