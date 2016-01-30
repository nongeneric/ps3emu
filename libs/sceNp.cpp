#include "sceNp.h"

#include "../ps3emu/utils.h"
#include <boost/log/trivial.hpp>
#include <vector>

int32_t sceNpBasicSetPresence(ps3_uintptr_t data, size_t size, MainMemory* mm) {
    std::vector<char> buf(size);
    mm->readMemory(data, &buf[0], size);
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sceNpBasicSetPresence(%d, %s)", size, print_hex(&buf[0], buf.size()));
    return CELL_OK;
}

#define SCE_NP_ERROR_OFFLINE 0x8002aa0c

int32_t sceNpManagerGetContentRatingFlag(big_int32_t* isRestricted, big_int32_t* age) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return SCE_NP_ERROR_OFFLINE;
}

int32_t sceNpBasicRegisterHandler(const SceNpCommunicationId* context,
                                  ps3_uintptr_t handler, 
                                  ps3_uintptr_t arg)
{
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

int32_t sceNpDrmIsAvailable(const SceNpDrmKey* k_licensee, const char* path) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

// this function is equivalent to sceNpDrmIsAvailable, because the emu completes the check immediately
int32_t sceNpDrmIsAvailable2(const SceNpDrmKey* k_licensee, const char* path) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sceNpDrmIsAvailable2(..., %s)", path);
    return CELL_OK;
}

int32_t sceNpManagerGetNpId(SceNpId* npId) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return SCE_NP_ERROR_OFFLINE;
}

int32_t sceNpInit(uint32_t poolsize, ps3_uintptr_t poolptr) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

int32_t sceNpDrmVerifyUpgradeLicense2(cstring_ptr_t content_id) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sceNpDrmVerifyUpgradeLicense2(%s)", content_id);
    return CELL_OK;
}