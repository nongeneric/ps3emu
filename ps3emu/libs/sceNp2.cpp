#include "sceNp2.h"

#include "../../ps3emu/log.h"

int32_t sceNp2Init(uint32_t poolsize, ps3_uintptr_t poolptr) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

int32_t sceNp2Term() {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

int32_t sceNpMatching2Init(size_t stackSize, int priority) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

int32_t cellNetCtlAddHandler(uint32_t handler, uint32_t arg, big_int32_t* hid) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

#define CELL_NET_CTL_STATE_Disconnected 0

int32_t cellNetCtlGetState(big_int32_t* state) {
    INFO(libs) << __FUNCTION__;
    *state = CELL_NET_CTL_STATE_Disconnected;
    return CELL_OK;
}

int32_t sceNpBasicGetMatchingInvitationEntryCount(big_uint32_t* count) {
    *count = 0;
    return CELL_OK;
}

int32_t sceNpScoreInit() {
    return CELL_OK;
}

int32_t sceNpManagerGetOnlineName(SceNpOnlineName* onlineName) {
    return SCE_NP_ERROR_OFFLINE;
}

int32_t sceNpManagerGetAvatarUrl(SceNpAvatarUrl* avatarUrl) {
    return SCE_NP_ERROR_OFFLINE;
}
