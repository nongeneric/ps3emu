#include "libnetctl.h"
#include "ps3emu/log.h"
#include "ps3emu/utils.h"

int32_t cellNetCtlInit() {
    return CELL_OK;
}

int32_t cellNetCtlNetStartDialogLoadAsync(
    const CellNetCtlNetStartDialogParam* param) {
    INFO(libs) << ssnprintf("cellNetCtlNetStartDialogLoadAsync(%x)", param->type);
    return CELL_OK;
}
