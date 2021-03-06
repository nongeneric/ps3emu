#include "sceNp.h"

#include "../utils.h"
#include "../log.h"
#include <vector>

int32_t sceNpBasicSetPresence(ps3_uintptr_t data, size_t size, MainMemory* mm) {
    std::vector<char> buf(size);
    mm->readMemory(data, &buf[0], size);
    INFO(libs) << sformat("sceNpBasicSetPresence({}, {})", size, print_hex(&buf[0], buf.size()));
    return CELL_OK;
}

#define SCE_NP_ERROR_OFFLINE 0x8002aa0c

int32_t sceNpManagerGetContentRatingFlag(big_int32_t* isRestricted, big_int32_t* age) {
    INFO(libs) << __FUNCTION__;
    return SCE_NP_ERROR_OFFLINE;
}

int32_t sceNpBasicRegisterHandler(const SceNpCommunicationId* context,
                                  ps3_uintptr_t handler, 
                                  ps3_uintptr_t arg)
{
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

int32_t sceNpDrmIsAvailable(const SceNpDrmKey* k_licensee, const char* path) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

// this function is equivalent to sceNpDrmIsAvailable, because the emu completes the check immediately
int32_t sceNpDrmIsAvailable2(const SceNpDrmKey* k_licensee, const char* path) {
    INFO(libs) << sformat("sceNpDrmIsAvailable2(..., {})", path);
    return CELL_OK;
}

int32_t sceNpManagerGetNpId(SceNpId* npId) {
    INFO(libs) << __FUNCTION__;
    return SCE_NP_ERROR_OFFLINE;
}

int32_t sceNpInit(uint32_t poolsize, ps3_uintptr_t poolptr) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

int32_t sceNpDrmVerifyUpgradeLicense2(cstring_ptr_t content_id) {
    INFO(libs) << sformat("sceNpDrmVerifyUpgradeLicense2({})", content_id.str);
    return CELL_OK;
}

int32_t sceNpManagerRegisterCallback(uint32_t callback, uint32_t arg) {
    WARNING(libs) << "sceNpManagerRegisterCallback: not implemented";
    return CELL_OK;
}

int32_t sceNpBasicRegisterContextSensitiveHandler(
    const SceNpCommunicationId* context, uint32_t handler, uint32_t arg) {
    WARNING(libs) << "sceNpBasicRegisterContextSensitiveHandler: not implemented";
    return CELL_OK;
}

#define SCE_NP_MANAGER_STATUS_OFFLINE (-1)

int32_t sceNpManagerGetStatus(big_int32_t* status) {
    *status = SCE_NP_MANAGER_STATUS_OFFLINE;
    return CELL_OK;
}
