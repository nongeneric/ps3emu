#include "userinfo.h"

static constexpr int g_currentUserId = 1;
#define CELL_USERINFO_ERROR_NOUSER 0x8002c304

int32_t cellUserInfoGetList(uint32_t* listNum,
                            CellUserInfoUserList* listBuf,
                            CellSysutilUserId* currentUserId) {
    *listNum = 1;
    listBuf->userId[0] = g_currentUserId;
    *currentUserId = g_currentUserId;
    return CELL_OK;
}

int32_t cellUserInfoGetStat(CellSysutilUserId id, CellUserInfoUserStat* stat) {
    if (id != 0 && id != g_currentUserId)
        return CELL_USERINFO_ERROR_NOUSER;
    stat->id = g_currentUserId;
    strcpy(stat->name, "emu_user");
    return CELL_OK;
}
