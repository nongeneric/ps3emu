#include "libnetctl.h"
#include "ps3emu/log.h"
#include "ps3emu/utils.h"

#define CELL_NET_CTL_ERROR_NOT_TERMINATED 0x80130102

int32_t cellNetCtlInit() {
    return CELL_NET_CTL_ERROR_NOT_TERMINATED;
}

int32_t cellNetCtlNetStartDialogLoadAsync(
    const CellNetCtlNetStartDialogParam* param) {
    INFO(libs) << sformat("cellNetCtlNetStartDialogLoadAsync({:x})", param->type);
    return CELL_OK;
}

emu_void_t cellNetCtlTerm() {
    return emu_void;
}
