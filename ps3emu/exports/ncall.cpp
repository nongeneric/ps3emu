#include "exports.h"
#include "ps3emu/Process.h"
#include "ps3emu/utils.h"
#include "ps3emu/libs/sys.h"
#include "ps3emu/libs/Controller.h"
#include "ps3emu/libs/libsysutil.h"
#include "ps3emu/libs/graphics/gcm.h"
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
#include "ps3emu/libs/audio/libaudio.h"
#include "ps3emu/libs/message.h"
#include "ps3emu/libs/trophy.h"
#include "ps3emu/libs/savedata/savedata.h"
#include "ps3emu/libs/libnet.h"
#include "ps3emu/libs/libnetctl.h"
#include "ps3emu/libs/libcamera.h"
#include "ps3emu/libs/libgem.h"
#include "ps3emu/ppu/CallbackThread.h"
#include "ps3emu/log.h"
#include <tbb/concurrent_vector.h>
#include <openssl/sha.h>
#include <memory>
#include <algorithm>

using namespace emu::Gcm;


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

CellFsErrno cellFsStat_proxy(ps3_uintptr_t path, CellFsStat* sb, Process* proc) {
    std::string pathStr;
    readString(g_state.mm, path, pathStr);
    return cellFsStat(pathStr.c_str(), sb, proc);
}

CellFsErrno cellFsMkdir_proxy(ps3_uintptr_t path,
                              uint32_t mode,
                              Process* proc)
{
    std::string pathStr;
    readString(g_state.mm, path, pathStr);
    return cellFsMkdir(pathStr.c_str(), mode, proc);
}

CellFsErrno cellFsUnlink_proxy(ps3_uintptr_t path,
                               Process* proc)
{
    std::string pathStr;
    readString(g_state.mm, path, pathStr);
    return cellFsUnlink(pathStr.c_str(), proc);
}

CellFsErrno cellFsGetFreeSize_proxy(ps3_uintptr_t directory_path,
                              big_uint32_t* block_size,
                              big_uint64_t* free_block_count,
                              Process* proc)
{
    std::string pathStr;
    readString(g_state.mm, directory_path, pathStr);
    return cellFsGetFreeSize(pathStr.c_str(), block_size, free_block_count, proc);
}

CellFsErrno cellFsOpendir_proxy(ps3_uintptr_t path, big_int32_t *fd, Process* proc) {
    std::string pathStr;
    readString(g_state.mm, path, pathStr);
    return cellFsOpendir(pathStr.c_str(), fd, proc);
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

tbb::concurrent_vector<NCallEntry> ncallTable {
    ENTRY(EmuInitLoadedPrxModules),
    ENTRY(defaultContextCallback),
    ENTRY(_sys_process_atexitspawn),
    ENTRY(_sys_process_at_Exitspawn),
    ENTRY(sys_prx_exitspawn_with_level),
    ENTRY(_cellGcmInitBody),
    ENTRY(cellVideoOutConfigure),
    ENTRY(cellVideoOutGetState),
    ENTRY(cellVideoOutGetResolution),
    ENTRY(cellGcmSetFlipMode),
    ENTRY(cellGcmGetConfiguration),
    ENTRY(cellGcmAddressToOffset),
    ENTRY(cellGcmSetDisplayBuffer),
    ENTRY(cellGcmGetControlRegister),
    ENTRY(cellGcmGetLabelAddress),
    ENTRY(cellGcmGetFlipStatus),
    ENTRY(cellGcmResetFlipStatus),
    ENTRY(_cellGcmSetFlipCommand),
    ENTRY(cellGcmIoOffsetToAddress),
    ENTRY(cellGcmGetTiledPitchSize),
    ENTRY(cellGcmSetTileInfo),
    { "cellGcmSetTile", calcFnid("cellGcmSetTile"), [](PPUThread* th) { wrap(cellGcmSetTileInfo, th); } },
    ENTRY(cellGcmBindTile),
    ENTRY(cellGcmUnbindTile),
    ENTRY(cellGcmSetZcull),
    ENTRY(cellGcmBindZcull),
    ENTRY(cellGcmUnbindZcull),
    ENTRY(cellGcmMapMainMemory),
    ENTRY(cellGcmSetFlipHandler),
    ENTRY(cellPadInit),
    ENTRY(cellPadClearBuf),
    ENTRY(cellPadSetPortSetting),
    ENTRY(cellPadEnd),
    ENTRY(cellKbInit),
    ENTRY(cellKbSetCodeType),
    ENTRY(cellMouseInit),
    ENTRY(cellGcmSetDefaultCommandBuffer),
    ENTRY(cellSysutilRegisterCallback),
    ENTRY(cellSysutilUnregisterCallback),
    ENTRY(cellSysutilCheckCallback),
    ENTRY(cellPadGetInfo2),
    ENTRY(cellKbGetInfo),
    ENTRY(cellMouseGetInfo),
    ENTRY(cellGcmSetDebugOutputLevel),
    ENTRY(cellSysmoduleLoadModule),
    ENTRY(cellSysmoduleUnloadModule),
    ENTRY(cellSysmoduleIsLoaded),
    ENTRY(cellSysmoduleInitialize),
    { "cellFsStat", calcFnid("cellFsStat"), [](PPUThread* th) { wrap(cellFsStat_proxy, th); } },
    { "cellFsMkdir", calcFnid("cellFsMkdir"), [](PPUThread* th) { wrap(cellFsMkdir_proxy, th); } },
    { "cellFsGetFreeSize", calcFnid("cellFsGetFreeSize"), [](PPUThread* th) { wrap(cellFsGetFreeSize_proxy, th); } },
    { "cellFsUnlink", calcFnid("cellFsUnlink"), [](PPUThread* th) { wrap(cellFsUnlink_proxy, th); } },
    { "cellFsOpendir", calcFnid("cellFsOpendir"), [](PPUThread* th) { wrap(cellFsOpendir_proxy, th); } },
    ENTRY(cellFsOpen),
    ENTRY(cellFsSdataOpen),
    ENTRY(cellFsLseek),
    ENTRY(cellFsClose),
    ENTRY(cellFsFstat),
    ENTRY(cellFsRead),
    ENTRY(cellFsWrite),
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
    ENTRY(cellFsFsync),
    ENTRY(cellFsReaddir),
    ENTRY(cellFsClosedir),
    ENTRY(cellFsGetDirectoryEntries),
    ENTRY(sys_ppu_thread_once),
    ENTRY(_sys_spu_printf_initialize),
    ENTRY(_sys_spu_printf_finalize),
    ENTRY(cellGcmSetGraphicsHandler),
    ENTRY(cellGcmGetMaxIoMapSize),
    ENTRY(cellGcmGetOffsetTable),
    ENTRY(cellGcmMapEaIoAddress),
    ENTRY(cellGcmMapEaIoAddressWithFlags),
    ENTRY(cellGcmUnmapEaIoAddress),
    ENTRY(cellGcmUnmapIoAddress),
    ENTRY(callbackThreadQueueWait),
    ENTRY(cellGcmSetVBlankHandler),
    ENTRY(cellGcmGetReportDataLocation),
    ENTRY(cellPadGetData),
    ENTRY(cellKbEnd),
    ENTRY(cellMouseEnd),
    ENTRY(cellVideoOutGetResolutionAvailability),
    ENTRY(cellGcmInitDefaultFifoMode),
    ENTRY(cellGcmGetTileInfo),
    ENTRY(cellGcmGetZcullInfo),
    ENTRY(cellGcmSetFlipStatus),
    ENTRY(cellGcmGetReportDataAddressLocation),
    ENTRY(cellGcmGetLastFlipTime),
    ENTRY(cellAudioOutGetSoundAvailability),
    ENTRY(cellAudioOutGetSoundAvailability2),
    ENTRY(cellAudioOutConfigure),
    ENTRY(cellAudioOutGetState),
    ENTRY(cellAudioInit),
    ENTRY(cellAudioPortOpen),
    ENTRY(cellAudioGetPortConfig),
    ENTRY(cellAudioPortStart),
    ENTRY(cellAudioSetNotifyEventQueue),
    ENTRY(cellAudioSetNotifyEventQueueEx),
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
    ENTRY(cellGcmGetTimeStamp),
    ENTRY(cellGcmGetReport),
    ENTRY(cellSaveDataAutoSave2),
    ENTRY(cellSaveDataAutoLoad2),
    ENTRY(cellSaveDataEnableOverlay),
    ENTRY(cellSaveDataListLoad2),
    ENTRY(cellAudioOutSetCopyControl),
    ENTRY(cellAudioCreateNotifyEventQueue),
    ENTRY(cellAudioGetPortBlockTag),
    ENTRY(cellAudioGetPortTimestamp),
    ENTRY(cellAudioAddData),
    ENTRY(cellGcmSetUserHandler),
    ENTRY(_cellGcmSetFlipCommandWithWaitLabel),
    ENTRY(cellGcmSetSecondVFrequency),
    ENTRY(sys_net_initialize_network_ex),
    ENTRY(cellNetCtlInit),
    ENTRY(cellNetCtlNetStartDialogLoadAsync),
    ENTRY(sceNpManagerRegisterCallback),
    ENTRY(cellAudioPortStop),
    ENTRY(cellAudioPortClose),
    ENTRY(cellAudioSetPortLevel),
    ENTRY(cellAudioOutGetNumberOfDevice),
    ENTRY(sceNpManagerGetStatus),
    ENTRY(cellVideoOutGetNumberOfDevice),
    ENTRY(cellAudioAdd2chData),
    ENTRY(cellAudioAdd6chData),
    ENTRY(cellGcmGetDefaultCommandWordSize),
    ENTRY(cellGcmGetDefaultSegmentWordSize),
    ENTRY(cellKbSetReadMode),
    ENTRY(_sys_memcpy),
    ENTRY(_sys_memset),
    ENTRY(cellFsReadWithOffset),
    ENTRY(cellFsWriteWithOffset),
    ENTRY(cellCameraInit),
    ENTRY(cellGemGetMemorySize),
    ENTRY(cellGemInit),
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
    auto it = std::find_if(ncallTable.begin(), ncallTable.end(), [=](auto& entry) {
        return entry.fnid == fnid;
    });
    if (it == ncallTable.end())
        return nullptr;
    index = std::distance(ncallTable.begin(), it);
    auto next = std::find_if(it + 1, ncallTable.end(), [=](auto& entry) {
        return entry.fnid == fnid;
    });
    (void)next;
    assert(next == ncallTable.end());
    return &(*it);
}

const NCallEntry* findNCallEntryByIndex(uint32_t index) {
    if (index >= ncallTable.size())
        return nullptr;
    return &ncallTable[index];
}

uint32_t addNCallEntry(NCallEntry entry) {
    ncallTable.push_back(entry);
    return ncallTable.size() - 1;
}
