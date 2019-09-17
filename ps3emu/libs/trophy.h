#pragma once

#include "sys.h"
#include <optional>
#include <array>
#include <boost/context/continuation.hpp>

typedef big_int32_t SceNpTrophyId;
typedef big_uint32_t SceNpTrophyContext;
typedef big_uint32_t SceNpTrophyHandle;
typedef big_uint32_t SceNpTrophyStatus;
typedef big_uint32_t SceNpTrophyGrade;

#define SCE_NP_TROPHY_TITLE_MAX_SIZE (128)
#define SCE_NP_TROPHY_GAME_DESCR_MAX_SIZE (1024)
#define SCE_NP_TROPHY_NAME_MAX_SIZE (128)
#define SCE_NP_TROPHY_DESCR_MAX_SIZE (1024)

struct SceNpTrophyGameDetails {
    big_uint32_t numTrophies;
    big_uint32_t numPlatinum;
    big_uint32_t numGold;
    big_uint32_t numSilver;
    big_uint32_t numBronze;
    char title[SCE_NP_TROPHY_TITLE_MAX_SIZE];
    char description[SCE_NP_TROPHY_GAME_DESCR_MAX_SIZE];
    uint8_t reserved[4];
};
static_assert(sizeof(SceNpTrophyGameDetails) == 1176, "");

struct SceNpTrophyGameData {
    big_uint32_t unlockedTrophies;
    big_uint32_t unlockedPlatinum;
    big_uint32_t unlockedGold;
    big_uint32_t unlockedSilver;
    big_uint32_t unlockedBronze;
};
static_assert(sizeof(SceNpTrophyGameData) == 20, "");

struct SceNpTrophyDetails {
    SceNpTrophyId trophyId;
    SceNpTrophyGrade trophyGrade;
    char name[SCE_NP_TROPHY_NAME_MAX_SIZE];
    char description[SCE_NP_TROPHY_DESCR_MAX_SIZE];
    uint8_t hidden;
    uint8_t reserved[3];
};
static_assert(sizeof(SceNpTrophyDetails) == 1164, "");

struct SceNpTrophyData {
    big_uint64_t timestamp;
    SceNpTrophyId trophyId;
    uint8_t unlocked;
    uint8_t reserved[3];
};
static_assert(sizeof(SceNpTrophyData) == 16, "");

typedef big_uint32_t SceNpTrophyFlagMask;
#define SCE_NP_TROPHY_FLAG_SETSIZE (128)
#define SCE_NP_TROPHY_FLAG_BITS_SHIFT (5)

int32_t sceNpTrophyInit(uint32_t pool,
                        uint32_t poolSize,
                        uint32_t containerId,
                        uint64_t options);

int32_t sceNpTrophyTerm(
);

int32_t sceNpTrophyCreateContext(SceNpTrophyContext* context,
                                 uint32_t commId,
                                 uint32_t commSign,
                                 uint64_t options);

int32_t sceNpTrophyDestroyContext(SceNpTrophyContext context);

int32_t sceNpTrophyCreateHandle(SceNpTrophyHandle* handle);

int32_t sceNpTrophyRegisterContext(SceNpTrophyContext context,
                                   SceNpTrophyHandle handle,
                                   uint32_t callback,
                                   uint32_t arg,
                                   uint64_t options,
                                   PPUThread* th,
                                   boost::context::continuation* sink);

int32_t sceNpTrophyUnlockTrophy(SceNpTrophyContext context,
                                SceNpTrophyHandle handle,
                                SceNpTrophyId trophyId,
                                SceNpTrophyId* platinumId);

int32_t sceNpTrophyGetGameInfo(SceNpTrophyContext context,
                               SceNpTrophyHandle handle,
                               SceNpTrophyGameDetails* details,
                               SceNpTrophyGameData* data);

int32_t sceNpTrophyGetTrophyUnlockState(SceNpTrophyContext context,
                                        SceNpTrophyHandle handle,
                                        std::array<uint8_t, 16>* flags,
                                        big_int32_t* count);

int32_t sceNpTrophyGetTrophyInfo(SceNpTrophyContext context,
                                 SceNpTrophyHandle handle,
                                 SceNpTrophyId trophyId,
                                 SceNpTrophyDetails* details,
                                 SceNpTrophyData* data);

int32_t sceNpTrophyGetGameIcon(SceNpTrophyContext context,
                               SceNpTrophyHandle handle,
                               uint32_t buffer,
                               big_int32_t* size);

int32_t sceNpTrophyGetTrophyIcon(SceNpTrophyContext context,
                                 SceNpTrophyHandle handle,
                                 SceNpTrophyId trophyId,
                                 big_int32_t buffer,
                                 big_int32_t* size);

int32_t sceNpTrophyGetGameProgress(SceNpTrophyContext context,
                                   SceNpTrophyHandle handle,
                                   big_int32_t* percentage);


int32_t sceNpTrophyGetRequiredDiskSpace(SceNpTrophyContext context,
                                        SceNpTrophyHandle handle,
                                        big_uint64_t* reqspace,
                                        uint64_t options);
int32_t sceNpTrophyDestroyHandle(SceNpTrophyHandle handle);
