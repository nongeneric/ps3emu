#pragma once

#include "sys_defs.h"

typedef big_uint32_t CellSysutilUserId;

typedef enum {
    CELL_USERINFO_USER_MAX = 16,
    CELL_USERINFO_TITLE_SIZE = 256,
    CELL_USERINFO_USERNAME_SIZE = 64
} CellUserInfoParamSize;

typedef struct {
    CellSysutilUserId userId[CELL_USERINFO_USER_MAX];
} CellUserInfoUserList;

typedef struct {
    CellSysutilUserId id;
    char name[CELL_USERINFO_USERNAME_SIZE];
} CellUserInfoUserStat;

int32_t cellUserInfoGetList(uint32_t* listNum,
                            CellUserInfoUserList* listBuf,
                            CellSysutilUserId* currentUserId);

int32_t cellUserInfoGetStat(CellSysutilUserId id, CellUserInfoUserStat* stat);
