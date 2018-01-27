#pragma once

#include "sys_defs.h"

int32_t sceNp2Init(uint32_t poolsize, ps3_uintptr_t poolptr);
int32_t sceNp2Term();

int32_t sceNpMatching2Init(size_t stackSize, int priority);
int32_t cellNetCtlAddHandler(uint32_t handler, uint32_t arg, big_int32_t* hid);
int32_t cellNetCtlGetState(big_int32_t* state);
int32_t sceNpBasicGetMatchingInvitationEntryCount(big_uint32_t* count);
int32_t sceNpScoreInit();

#define SCE_NET_NP_ONLINENAME_MAX_LENGTH 48
typedef struct SceNpOnlineName {
    char data[SCE_NET_NP_ONLINENAME_MAX_LENGTH];
    char term;
    char padding[3];
} SceNpOnlineName;

int32_t sceNpManagerGetOnlineName(SceNpOnlineName* onlineName);

#define SCE_NET_NP_AVATAR_URL_MAX_LENGTH 127
#define SCE_NP_ERROR_OFFLINE 0x8002aa0c

typedef struct SceNpAvatarUrl {
    char data[SCE_NET_NP_AVATAR_URL_MAX_LENGTH];
    char term;
} SceNpAvatarUrl;

int32_t sceNpManagerGetAvatarUrl(SceNpAvatarUrl* avatarUrl);
