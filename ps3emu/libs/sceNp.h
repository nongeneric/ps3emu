#pragma once

#include "../MainMemory.h"
#include "sys_defs.h"

int32_t sceNpBasicSetPresence(ps3_uintptr_t data, size_t size, MainMemory* mm);
int32_t sceNpManagerGetContentRatingFlag(big_int32_t *isRestricted, big_int32_t *age);

struct SceNpCommunicationId {
    char data[9];
    char term;
    uint8_t num;
    char dummy;
};
static_assert(sizeof(SceNpCommunicationId) == 12, "");

int32_t sceNpBasicRegisterHandler(const SceNpCommunicationId *context,
                                  ps3_uintptr_t handler,
                                  ps3_uintptr_t arg);

struct SceNpDrmKey {
    uint8_t keydata[16];
};
static_assert(sizeof(SceNpDrmKey) == 16, "");

int32_t sceNpDrmIsAvailable(const SceNpDrmKey *k_licensee, const char *path);
int32_t sceNpDrmIsAvailable2(const SceNpDrmKey *k_licensee, const char *path);
int32_t sceNpInit(uint32_t poolsize, ps3_uintptr_t poolptr);

struct SceNpOnlineId {
    char data[16];
    char term;
    char dummy[3];
};

struct SceNpId {
    SceNpOnlineId handle;
    uint8_t opt[8];
    uint8_t reserved[8];
};

int32_t sceNpManagerGetNpId(SceNpId *npId);
int32_t sceNpDrmVerifyUpgradeLicense2(cstring_ptr_t content_id);
int32_t sceNpManagerRegisterCallback(uint32_t callback, uint32_t arg);
int32_t sceNpBasicRegisterContextSensitiveHandler(
    const SceNpCommunicationId* context, uint32_t handler, uint32_t arg);
int32_t sceNpManagerGetStatus(big_int32_t* status);
