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
#include "ps3emu/libs/savedata.h"
#include "ps3emu/libs/spurs.h"
#include "ps3emu/ppu/CallbackThread.h"
#include "ps3emu/log.h"
#include <openssl/sha.h>
#include <memory>
#include <algorithm>
#include <vector>

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

CellFsErrno cellFsOpen_proxy(ps3_uintptr_t path, 
                             int flags, 
                             big_int32_t* fd,
                             uint64_t arg, 
                             uint64_t size, 
                             Process* proc)
{
    std::string pathStr;
    readString(g_state.mm, path, pathStr);
    return cellFsOpen(pathStr.c_str(), flags, fd, arg, size, proc);
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

NCallEntry ncallTable[] {
    ENTRY(EmuInitLoadedPrxModules),
    ENTRY(defaultContextCallback),
    ENTRY(sys_time_get_system_time),
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
    ENTRY(_cellGcmSetFlipWithWaitLabel),
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
    { "cellFsOpen", calcFnid("cellFsOpen"), [](PPUThread* th) { wrap(cellFsOpen_proxy, th); } },
    { "cellFsMkdir", calcFnid("cellFsMkdir"), [](PPUThread* th) { wrap(cellFsMkdir_proxy, th); } },
    { "cellFsGetFreeSize", calcFnid("cellFsGetFreeSize"), [](PPUThread* th) { wrap(cellFsGetFreeSize_proxy, th); } },
    { "cellFsUnlink", calcFnid("cellFsUnlink"), [](PPUThread* th) { wrap(cellFsUnlink_proxy, th); } },
    { "cellFsOpendir", calcFnid("cellFsOpendir"), [](PPUThread* th) { wrap(cellFsOpendir_proxy, th); } },
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
    ENTRY(cellAudioOutConfigure),
    ENTRY(cellAudioOutGetState),
    ENTRY(cellAudioInit),
    ENTRY(cellAudioPortOpen),
    ENTRY(cellAudioGetPortConfig),
    ENTRY(cellAudioPortStart),
    ENTRY(cellAudioSetNotifyEventQueue),
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
    ENTRY(cellSaveDataAutoSave2),
    ENTRY(cellSaveDataAutoLoad2),
    ENTRY(emuProxyEnter),
    ENTRY(emuProxyExit),
    ENTRY(emuBranch),
    ENTRY(_cellSpursTaskAttribute2Initialize_proxy),
    ENTRY(cellSpursCreateTaskset2_proxy),
    ENTRY(cellSpursCreateTask2_proxy),
    ENTRY(cellSpursCreateTaskWithAttribute_proxy),
    ENTRY(cellSpursEventFlagAttachLv2EventQueue_proxy),
    ENTRY(_cellSpursEventFlagInitialize_proxy),
    ENTRY(cellSyncQueueInitialize_proxy),
    ENTRY(_cellSpursQueueInitialize_proxy),
    ENTRY(cellSpursEventFlagWait_proxy),
    ENTRY(_cellSpursAttributeInitialize_proxy),
    ENTRY(cellSpursAttributeSetSpuThreadGroupType_proxy),
};

constexpr auto staticTableSize() { return sizeof(ncallTable) / sizeof(NCallEntry); }

static std::vector<NCallEntry> dynamicEntries;

void PPUThread::ncall(uint32_t index) {
    
    if (index >= staticTableSize() + dynamicEntries.size()) {
        auto msg = ssnprintf("unknown ncall index %x", index);
        ERROR(libs) << msg;
        throw std::runtime_error(msg);
    }
    setEMUREG(0, getNIP());
    setNIP(getLR());
    NCallEntry* entry;
    if (index >= staticTableSize()) {
        entry = &dynamicEntries[index - staticTableSize()];
    } else {
        entry = &ncallTable[index];
    }
    //INFO(libs) << ssnprintf("ncall %s", entry->name);
    entry->stub(this);
}

const NCallEntry* findNCallEntry(uint32_t fnid, uint32_t& index, bool assertFound) {
    auto count = staticTableSize();
    for (uint32_t i = 0; i < count; ++i) {
        if (ncallTable[i].fnid == fnid) {
            index = i;
            return &ncallTable[i];
        }
    }
    assert(!assertFound);
    return nullptr;
}

uint32_t addNCallEntry(NCallEntry entry) {
    dynamicEntries.push_back(entry);
    return staticTableSize() + dynamicEntries.size() - 1;
}
