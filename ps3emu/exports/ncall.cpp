#include "exports.h"
#include "ps3emu/Process.h"
#include "ps3emu/utils.h"
#include "ps3emu/libs/sys.h"
#include "ps3emu/libs/Controller.h"
#include "ps3emu/libs/libsysutil.h"
#include "ps3emu/libs/fs.h"
#include "ps3emu/libs/cellGame.h"
#include "ps3emu/libs/sceNp2.h"
#include "ps3emu/libs/sceNp.h"
#include "ps3emu/libs/spu/sysSpu.h"
#include "ps3emu/libs/sync/lwmutex.h"
#include "ps3emu/libs/sync/mutex.h"
#include "ps3emu/libs/sync/lwcond.h"
#include "ps3emu/libs/sync/cond.h"
#include "ps3emu/libs/sync/rwlock.h"
#include "ps3emu/libs/sync/queue.h"
#include "ps3emu/libs/sync/event_flag.h"
#include "ps3emu/libs/audio/configuration.h"
#include "ps3emu/libs/message.h"
#include "ps3emu/libs/trophy.h"
#include "ps3emu/libs/savedata/savedata.h"
#include "ps3emu/libs/libnet.h"
#include "ps3emu/libs/libnetctl.h"
#include "ps3emu/libs/libcamera.h"
#include "ps3emu/libs/libgem.h"
#include "ps3emu/libs/userinfo.h"
#include "ps3emu/libs/graphics/sysutil_sysparam.h"
#include "ps3emu/log.h"
#include <tbb/concurrent_vector.h>
#include <openssl/sha.h>
#include <memory>
#include <algorithm>

uint32_t calcSHA1id(const char* name, const uint8_t* suffix, size_t suffix_len) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, (const uint8_t*)name, strlen(name));
    SHA1_Update(&ctx, suffix, suffix_len);
    union {
        uint8_t b[20];
        uint32_t u32[5];
    } md;
    SHA1_Final(md.b, &ctx);
    return md.u32[0];
}

uint32_t calcEid(const char* name) {
    auto suffix = "0xbc5eba9e042504905b64274994d9c41f";
    return calcSHA1id(name, (const uint8_t*)suffix, strlen(suffix));
}

uint32_t calcFnid(const char* name) {
    static uint8_t suffix[] = {
        0x67, 0x59, 0x65, 0x99, 0x04, 0x25, 0x04, 0x90, 
        0x56, 0x64, 0x27, 0x49, 0x94, 0x89, 0x74, 0x1A
    };
    return calcSHA1id(name, (const uint8_t*)suffix, sizeof(suffix));
}

int32_t sceNpDrmIsAvailable_proxy(const SceNpDrmKey *k_licensee, ps3_uintptr_t path, MainMemory* mm) {
    std::string str;
    readString(mm, path, str);
    return sceNpDrmIsAvailable(k_licensee, str.c_str());
}

int32_t sceNpDrmIsAvailable2_proxy(const SceNpDrmKey *k_licensee, ps3_uintptr_t path, MainMemory* mm) {
    std::string str;
    readString(mm, path, str);
    return sceNpDrmIsAvailable2(k_licensee, str.c_str());
}

#define ENTRY(name) { #name, calcFnid(#name), [](PPUThread* th) { wrap(name, th); } }
#define EMPTY() { "none", calcFnid("none"), [](PPUThread* th) { } }

tbb::concurrent_vector<NCallEntry> ncallTable {
    ENTRY(EmuInitLoadedPrxModules),
    EMPTY(),
    ENTRY(_sys_process_atexitspawn),
    ENTRY(_sys_process_at_Exitspawn),
    ENTRY(sys_prx_exitspawn_with_level),
    EMPTY(),
    ENTRY(cellVideoOutConfigure),
    ENTRY(cellVideoOutGetState),
    ENTRY(cellVideoOutGetResolution),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    ENTRY(cellPadInit),
    ENTRY(cellPadClearBuf),
    ENTRY(cellPadSetPortSetting),
    ENTRY(cellPadEnd),
    ENTRY(cellKbInit),
    ENTRY(cellKbSetCodeType),
    ENTRY(cellMouseInit),
    EMPTY(),
    ENTRY(cellSysutilRegisterCallback),
    ENTRY(cellSysutilUnregisterCallback),
    ENTRY(cellSysutilCheckCallback),
    ENTRY(cellPadGetInfo2),
    ENTRY(cellKbGetInfo),
    ENTRY(cellMouseGetInfo),
    EMPTY(),
    ENTRY(cellSysmoduleLoadModule),
    ENTRY(cellSysmoduleUnloadModule),
    ENTRY(cellSysmoduleIsLoaded),
    ENTRY(cellSysmoduleInitialize),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    ENTRY(sys_ppu_thread_join),
    ENTRY(sys_ppu_thread_exit),
    ENTRY(cellGameBootCheck),
    ENTRY(cellGameDataCheck),
    ENTRY(cellGamePatchCheck),
    ENTRY(cellGameContentPermit),
    ENTRY(cellGameGetParamString),
    ENTRY(cellSysutilGetSystemParamInt),
    ENTRY(cellSysutilGetSystemParamString),
    ENTRY(sceNp2Init),
    ENTRY(sceNpInit),
    ENTRY(sceNp2Term),
    ENTRY(sceNpBasicSetPresence),
    ENTRY(sceNpBasicRegisterContextSensitiveHandler),
    ENTRY(sceNpManagerGetContentRatingFlag),
    ENTRY(sceNpBasicRegisterHandler),
    { "sceNpDrmIsAvailable2", calcFnid("sceNpDrmIsAvailable2"), [](PPUThread* th) { wrap(sceNpDrmIsAvailable2_proxy, th); } },
    { "sceNpDrmIsAvailable", calcFnid("sceNpDrmIsAvailable"), [](PPUThread* th) { wrap(sceNpDrmIsAvailable_proxy, th); } },
    ENTRY(sceNpDrmVerifyUpgradeLicense2),
    ENTRY(sceNpManagerGetNpId),
    ENTRY(cellSysCacheMount),
    ENTRY(cellSysCacheClear),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    ENTRY(sys_ppu_thread_once),
    ENTRY(_sys_spu_printf_initialize),
    ENTRY(_sys_spu_printf_finalize),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    ENTRY(cellPadGetData),
    ENTRY(cellKbEnd),
    ENTRY(cellMouseEnd),
    ENTRY(cellVideoOutGetResolutionAvailability),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    ENTRY(cellAudioOutGetSoundAvailability),
    ENTRY(cellAudioOutGetSoundAvailability2),
    ENTRY(cellAudioOutConfigure),
    ENTRY(cellAudioOutGetState),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    ENTRY(sys_event_port_disconnect),
    ENTRY(ps3call_then),
    ENTRY(_sys_printf),
    ENTRY(emuEmptyModuleStart),
    ENTRY(sys_lwmutex_create),
    ENTRY(sys_lwmutex_destroy),
    ENTRY(sys_lwmutex_lock),
    ENTRY(sys_lwmutex_trylock),
    ENTRY(sys_lwmutex_unlock),
    ENTRY(sys_lwcond_create),
    ENTRY(sys_lwcond_destroy),
    ENTRY(sys_lwcond_wait),
    ENTRY(sys_lwcond_signal),
    ENTRY(sys_lwcond_signal_all),
    ENTRY(cellSpursInitializeWithAttribute),
    ENTRY(cellSpursInitializeWithAttribute2),
    ENTRY(cellSpursFinalize),
    ENTRY(cellPadSetActDirect),
    ENTRY(cellMsgDialogOpen2),
    ENTRY(cellMsgDialogProgressBarSetMsg),
    ENTRY(cellMsgDialogClose),
    ENTRY(cellMsgDialogProgressBarInc),
    ENTRY(cellMsgDialogAbort),
    ENTRY(sceNpTrophyInit),
    ENTRY(sceNpTrophyTerm),
    ENTRY(sceNpTrophyCreateContext),
    ENTRY(sceNpTrophyDestroyContext),
    ENTRY(sceNpTrophyCreateHandle),
    ENTRY(sceNpTrophyRegisterContext),
    ENTRY(sceNpTrophyUnlockTrophy),
    ENTRY(sceNpTrophyGetGameInfo),
    ENTRY(sceNpTrophyGetTrophyInfo),
    ENTRY(sceNpTrophyGetGameIcon),
    ENTRY(sceNpTrophyGetTrophyIcon),
    ENTRY(sceNpTrophyGetGameProgress),
    ENTRY(sceNpTrophyGetRequiredDiskSpace),
    ENTRY(sceNpTrophyGetTrophyUnlockState),
    EMPTY(),
    EMPTY(),
    ENTRY(cellSaveDataAutoSave2),
    ENTRY(cellSaveDataAutoLoad2),
    ENTRY(cellSaveDataEnableOverlay),
    ENTRY(cellSaveDataListLoad2),
    ENTRY(cellAudioOutSetCopyControl),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    ENTRY(sys_net_initialize_network_ex),
    ENTRY(cellNetCtlInit),
    ENTRY(cellNetCtlNetStartDialogLoadAsync),
    ENTRY(sceNpManagerRegisterCallback),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    ENTRY(cellAudioOutGetNumberOfDevice),
    ENTRY(sceNpManagerGetStatus),
    ENTRY(cellVideoOutGetNumberOfDevice),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    EMPTY(),
    ENTRY(cellKbSetReadMode),
    ENTRY(_sys_memcpy),
    ENTRY(_sys_memset),
    EMPTY(),
    EMPTY(),
    ENTRY(cellCameraInit),
    ENTRY(cellGemGetMemorySize),
    ENTRY(cellGemInit),
    ENTRY(ps3call_tests), // don't move
    ENTRY(slicing_tests), // don't move
    ENTRY(cellUserInfoGetList),
    ENTRY(cellUserInfoGetStat),
    ENTRY(sceNpMatching2Init),
    ENTRY(cellNetCtlAddHandler),
    ENTRY(cellNetCtlGetState),
    ENTRY(sceNpBasicGetMatchingInvitationEntryCount),
    ENTRY(sceNpScoreInit),
    ENTRY(sceNpManagerGetOnlineName),
    ENTRY(sceNpManagerGetAvatarUrl),
    ENTRY(cellGameDataCheckCreate2),
    ENTRY(sys_net_free_thread_context),
    ENTRY(cellKbRead),
    ENTRY(cellMouseGetData),
    ENTRY(cellAudioOutGetConfiguration),
    ENTRY(cellGameGetParamInt),
    ENTRY(sceNpTrophyDestroyHandle),
    ENTRY(cellGameGetSizeKB),
};

void PPUThread::ncall(uint32_t index) {
    if (index >= ncallTable.size()) {
        auto msg = ssnprintf("unknown ncall index %x", index);
        ERROR(libs) << msg;
        throw std::runtime_error(msg);
    }
    setEMUREG(0, getNIP());
    setNIP(getLR());
    auto entry = &ncallTable[index];
    //INFO(libs) << ssnprintf("ncall %s", entry->name);
    entry->stub(this);
}

const NCallEntry* findNCallEntry(uint32_t fnid, uint32_t& index, bool assertFound) {
    auto end = ncallTable.end();
    auto it = std::find_if(ncallTable.begin(), end, [=](auto& entry) {
        return entry.fnid == fnid;
    });
    if (it == end)
        return nullptr;
    index = std::distance(ncallTable.begin(), it);
#if DEBUG
    auto next = std::find_if(it + 1, end, [=](auto& entry) {
        return entry.fnid == fnid;
    });
    assert(next == end);
#endif
    return &(*it);
}

const NCallEntry* findNCallEntryByIndex(uint32_t index) {
    if (index >= ncallTable.size())
        return nullptr;
    return &ncallTable[index];
}

uint32_t addNCallEntry(NCallEntry entry) {
    auto it = ncallTable.push_back(entry);
    return std::distance(ncallTable.begin(), it);
}
