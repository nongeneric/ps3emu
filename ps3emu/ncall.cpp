#include "Process.h"
#include "utils.h"
#include "libs/sys.h"
#include "libs/Controller.h"
#include "libs/libsysutil.h"
#include "libs/graphics/gcm.h"
#include "libs/fs.h"
#include "libs/cellGame.h"
#include "libs/sceNp2.h"
#include "libs/sceNp.h"
#include "libs/spu/sysSpu.h"
#include "libs/spu/cellSpurs.h"
#include "libs/spu/libSync.h"
#include "libs/heap.h"
#include "libs/sync/lwmutex.h"
#include "libs/sync/mutex.h"
#include "libs/sync/lwcond.h"
#include "libs/sync/cond.h"
#include "libs/sync/rwlock.h"
#include "libs/sync/queue.h"
#include "libs/sync/event_flag.h"
#include "libs/libpngdec.h"
#include "libs/libl10n.h"
#include "libs/audio/configuration.h"
#include "libs/audio/libaudio.h"
#include "ppu/CallbackThread.h"
#include "state.h"
#include "log.h"
#include <openssl/sha.h>
#include <boost/hana.hpp>
#include <memory>
#include <algorithm>
#include <vector>

namespace hana = boost::hana;
using namespace hana::literals;
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

template <int ArgN, class T, class Enable = void>
struct get_arg {
    inline T value(PPUThread* thread) {
        return (T)thread->getGPR(3 + ArgN);
    }
    inline void destroy() {}
};

template <int ArgN>
struct get_arg<ArgN, PPUThread*> {
    inline PPUThread* value(PPUThread* thread) {
        return thread;
    }
    inline void destroy() {}
};

template <int ArgN>
struct get_arg<ArgN, Process*> {
    inline Process* value(PPUThread* thread) {
        return g_state.proc;
    }
    inline void destroy() {}
};

template <int ArgN>
struct get_arg<ArgN, MainMemory*> {
    inline MainMemory* value(PPUThread* thread) {
        return g_state.mm;
    }
    inline void destroy() {}
};

template <int ArgN>
struct get_arg<ArgN, cstring_ptr_t> {
    inline cstring_ptr_t value(PPUThread* thread) {
        cstring_ptr_t cstr;
        readString(g_state.mm, thread->getGPR(3 + ArgN), cstr.str);
        return cstr;
    }
    inline void destroy() {}
};

template <int ArgN, class T>
struct get_arg<ArgN, T, typename boost::enable_if< boost::is_pointer<T> >::type> {
    typedef typename boost::remove_const< typename boost::remove_pointer<T>::type >::type elem_type;
    elem_type _t;
    uint64_t _va;
    PPUThread* _thread;
    inline T value(PPUThread* thread) {
        _va = (ps3_uintptr_t)thread->getGPR(3 + ArgN);
        if (_va == 0)
            return nullptr;
        g_state.mm->readMemory(_va, &_t, sizeof(elem_type));
        _thread = thread;
        return &_t; 
    }
    inline void destroy() {
        if (_va == 0 || boost::is_const<T>::value)
            return;
        g_state.mm->writeMemory(_va, &_t, sizeof(elem_type));
    }
};

template <typename T, typename N>
constexpr auto make_n_tuple(T t, N n) {
    return hana::eval_if(n == 0_c,
        [&](auto _) { return t; },
        [&](auto _) { return hana::append(make_n_tuple(t, n - 1_c), n - 1_c); }
    );
}

template <typename R, typename... Args>
constexpr auto make_type_tuple(R (*)(Args...)) {
    return hana::make_tuple(hana::type_c<Args>...);
}

template <typename F>
auto wrap(F f, PPUThread* th) {
    auto types = make_type_tuple(f);
    auto ns = make_n_tuple(hana::make_tuple(), hana::int_c<hana::length(types)>);
    auto pairs = hana::zip(ns, types);
    auto holders = hana::transform(pairs, [&](auto t) {
        return get_arg<t[0_c].value, typename decltype(+t[1_c])::type>();
    });
    auto values = hana::transform(holders, [&](auto& h) {
        return h.value(th);
    });
    th->setGPR(3, hana::unpack(values, f));
    hana::for_each(holders, [&](auto& h) {
        h.destroy();
    });
}

uint32_t sys_fs_open_proxy(uint32_t path,
                           uint32_t flags,
                           big_int32_t *fd,
                           uint64_t mode,
                           uint32_t arg,
                           uint64_t size,
                           PPUThread* thread)
{
    std::string pathStr;
    readString(g_state.mm, path, pathStr);
    std::vector<uint8_t> argVec(size + 1);
    if (arg) {
        g_state.mm->readMemory(arg, &argVec[0], size);
    } else {
        argVec[0] = 0;
    }
    return sys_fs_open(pathStr.c_str(), flags, fd, mode, &argVec[0], size, g_state.proc);
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
    ENTRY(_sys_heap_create_heap),
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
    ENTRY(cellSpursAttributeSetNamePrefix),
    ENTRY(cellSpursAttributeEnableSpuPrintfIfAvailable),
    ENTRY(cellSpursAttributeSetSpuThreadGroupType),
    ENTRY(cellSpursInitializeWithAttribute),
    ENTRY(cellSpursInitializeWithAttribute2),
    ENTRY(_cellSpursAttributeInitialize),
    ENTRY(cellSpursFinalize),
    ENTRY(cellSpursJobChainAttributeSetName),
    ENTRY(cellSpursCreateJobChainWithAttribute),
    ENTRY(_cellSpursJobChainAttributeInitialize),
    ENTRY(cellSpursJoinJobChain),
    ENTRY(cellSpursRunJobChain),
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
    ENTRY(_sys_strlen),
    ENTRY(_sys_heap_malloc),
    ENTRY(cellPadGetData),
    ENTRY(cellKbEnd),
    ENTRY(cellMouseEnd),
    ENTRY(cellVideoOutGetResolutionAvailability),
    ENTRY(cellPngDecCreate),
    ENTRY(cellPngDecDecodeData),
    ENTRY(cellPngDecClose),
    ENTRY(cellPngDecDestroy),
    ENTRY(cellPngDecReadHeader),
    ENTRY(cellPngDecOpen),
    ENTRY(cellPngDecSetParameter),
    ENTRY(cellGcmInitDefaultFifoMode),
    ENTRY(cellGcmGetTileInfo),
    ENTRY(cellGcmGetZcullInfo),
    ENTRY(cellGcmSetFlipStatus),
    ENTRY(cellGcmGetReportDataAddressLocation),
    ENTRY(cellGcmGetLastFlipTime),
    ENTRY(_sys_heap_delete_heap),
    ENTRY(l10n_get_converter),
    ENTRY(l10n_convert_str),
    ENTRY(cellAudioOutGetSoundAvailability),
    ENTRY(cellAudioOutConfigure),
    ENTRY(cellAudioOutGetState),
    ENTRY(cellAudioInit),
    ENTRY(_sys_heap_memalign),
    ENTRY(cellAudioPortOpen),
    ENTRY(cellAudioGetPortConfig),
    ENTRY(cellAudioPortStart),
    ENTRY(cellAudioSetNotifyEventQueue),
    ENTRY(_cellSpursTasksetAttributeInitialize),
    ENTRY(cellSpursTasksetAttributeSetName),
    ENTRY(cellSpursCreateTasksetWithAttribute),
    ENTRY(cellSpursDestroyTaskset2),
    ENTRY(cellSpursCreateTaskset2),
    ENTRY(cellSpursJoinTask2),
    ENTRY(_cellSpursTasksetAttribute2Initialize),
    ENTRY(cellSpursCreateTask2WithBinInfo),
    ENTRY(sys_event_port_disconnect),
    ENTRY(cellSyncMutexInitialize),
    ENTRY(cellSyncMutexTryLock),
    ENTRY(cellSyncMutexLock),
    ENTRY(cellSyncMutexUnlock),
    ENTRY(_cellSpursEventFlagInitialize),
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
};

void PPUThread::ncall(uint32_t index) {
    if (index >= sizeof(ncallTable) / sizeof(NCallEntry))
        throw std::runtime_error(ssnprintf("unknown ncall index %x", index));
    setNIP(getLR());
    ncallTable[index].stub(this);
}

void PPUThread::scall() {
    auto index = getGPR(11);
    // INFO(libs) << ssnprintf("  -- scall %d at %08x", index, getNIP());
    switch (index) {
        case 352: wrap(sys_memory_get_user_memory_size, this); break;
        case 403: wrap(sys_tty_write, this); break;
        case 988: wrap(sys_dbg_set_mask_to_ppu_exception_handler, this); break;
        case 348: wrap(sys_memory_allocate, this); break;
        case 349: wrap(sys_memory_free, this); break;
        case 141: wrap(sys_timer_usleep, this); break;
        case 801: wrap(sys_fs_open_proxy, this); break;
        case 128: wrap(sys_event_queue_create, this); break;
        case 129: wrap(sys_event_queue_destroy, this); break;
        case 130: wrap(sys_event_queue_receive, this); break;
        case 131: wrap(sys_event_queue_tryreceive, this); break;
        case 133: wrap(sys_event_queue_drain, this); break;
        case 134: wrap(sys_event_port_create, this); break;
        case 136: wrap(sys_event_port_connect_local, this); break;
        case 138: wrap(sys_event_port_send, this); break;
        case 147: wrap(sys_time_get_timebase_frequency, this); break;
        case 49: wrap(sys_ppu_thread_get_stack_information, this); break;
        case 100: wrap(sys_mutex_create, this); break;
        case 101: wrap(sys_mutex_destroy, this); break;
        case 102: wrap(sys_mutex_lock, this); break;
        case 103: wrap(sys_mutex_trylock, this); break;
        case 104: wrap(sys_mutex_unlock, this); break;
        case 145: wrap(sys_time_get_current_time, this); break;
        case 144: wrap(sys_time_get_timezone, this); break;
        case 90: wrap(sys_semaphore_create, this); break;
        case 91: wrap(sys_semaphore_destroy, this); break;
        case 92: wrap(sys_semaphore_wait, this); break;
        case 93: wrap(sys_semaphore_trywait, this); break;
        case 94: wrap(sys_semaphore_post, this); break;
        case 41: wrap(sys_ppu_thread_exit, this); break;
        case 44: wrap(sys_ppu_thread_join, this); break;
        case 47: wrap(sys_ppu_thread_set_priority, this); break;
        case 48: wrap(sys_ppu_thread_get_priority, this); break;
        case 120: wrap(sys_rwlock_create, this); break;
        case 121: wrap(sys_rwlock_destroy, this); break;
        case 122: wrap(sys_rwlock_rlock, this); break;
        case 123: wrap(sys_rwlock_tryrlock, this); break;
        case 124: wrap(sys_rwlock_runlock, this); break;
        case 125: wrap(sys_rwlock_wlock, this); break;
        case 148: // duplicate
        case 126: wrap(sys_rwlock_trywlock, this); break;
        case 127: wrap(sys_rwlock_wunlock, this); break;
        case 105: wrap(sys_cond_create, this); break;
        case 106: wrap(sys_cond_destroy, this); break;
        case 107: wrap(sys_cond_wait, this); break;
        case 108: wrap(sys_cond_signal, this); break;
        case 109: wrap(sys_cond_signal_all, this); break;
        case 182: wrap(sys_spu_thread_read_ls, this); break;
        case 169: wrap(sys_spu_initialize, this); break;
        case 170: wrap(sys_spu_thread_group_create, this); break;
        case 171: wrap(sys_spu_thread_group_destroy, this); break;
        case 172: wrap(sys_spu_thread_initialize, this); break;
        case 173: wrap(sys_spu_thread_group_start, this); break;
        case 178: wrap(sys_spu_thread_group_join, this); break;
        case 165: wrap(sys_spu_thread_get_exit_status, this); break;
        case 160: wrap(sys_raw_spu_create, this); break;
        case 156: wrap(sys_spu_image_open, this); break;
        case 150: wrap(sys_raw_spu_create_interrupt_tag, this); break;
        case 151: wrap(sys_raw_spu_set_int_mask, this); break;
        case 84: wrap(sys_interrupt_thread_establish, this); break;
        case 154: wrap(sys_raw_spu_get_int_stat, this); break;
        case 155: wrap(sys_spu_image_get_info, this); break;
        case 158: wrap(sys_spu_image_close, this); break;
        case 159: wrap(sys_spu_image_get_modules, this); break;
        case 163: wrap(sys_raw_spu_read_puint_mb, this); break;
        case 153: wrap(sys_raw_spu_set_int_stat, this); break;
        case 88: wrap(sys_interrupt_thread_eoi, this); break;
        case 161: wrap(sys_raw_spu_destroy, this); break;
        case 43: wrap(sys_ppu_thread_yield, this); break;
        case 818: wrap(sys_fs_lseek, this); break;
        case 809: wrap(sys_fs_fstat, this); break;
        case 802: wrap(sys_fs_read, this); break;
        case 804: wrap(sys_fs_close, this); break;
        case 137: wrap(sys_event_port_disconnect, this); break;
        case 135: wrap(sys_event_port_destroy, this); break;
        case 14: wrap(sys_process_is_spu_lock_line_reservation_address, this); break;
        case 191: wrap(sys_spu_thread_connect_event, this); break;
        case 193: wrap(sys_spu_thread_bind_queue, this); break;
        case 251: wrap(sys_spu_thread_group_connect_event_all_threads, this); break;
        case 1: wrap(sys_process_getpid, this); break;
        case 25: wrap(sys_process_get_sdk_version, this); break;
        case 185: wrap(sys_spu_thread_group_connect_event, this); break;
        case 186: wrap(sys_spu_thread_group_disconnect_event, this); break;
        case 30: wrap(_sys_process_get_paramsfo, this); break;
        case 52: wrap(sys_ppu_thread_create, this); break;
        case 53: wrap(sys_ppu_thread_start, this); break;
        case 22: wrap(sys_process_exit, this); break;
        case 462: wrap(sys_get_process_info, this); break;
        case 484: wrap(sys_prx_register_module, this); break;
        case 486: wrap(sys_prx_register_library, this); break;
        case 480: wrap(sys_prx_load_module, this); break;
        case 481: wrap(sys_prx_start_module, this); break;
        case 871: wrap(sys_ss_access_control_engine, this); break;
        case 465: wrap(sys_prx_load_module_list, this); break;
        case 332: wrap(sys_mmapper_allocate_shared_memory, this); break;
        case 330: wrap(sys_mmapper_allocate_address, this); break;
        case 337: wrap(sys_mmapper_search_and_map, this); break;
        case 494: wrap(sys_prx_get_module_list, this); break;
        case 482: wrap(sys_prx_stop_module, this); break;
        case 483: wrap(sys_prx_unload_module, this); break;
        case 496: wrap(sys_prx_get_module_id_by_name, this); break;
        case 82: wrap(sys_event_flag_create, this); break;
        case 83: wrap(sys_event_flag_destroy, this); break;
        case 85: wrap(sys_event_flag_wait, this); break;
        case 87: wrap(sys_event_flag_set, this); break;
        case 118: wrap(sys_event_flag_clear, this); break;
        case 139: wrap(sys_event_flag_get, this); break;
        case 190: wrap(sys_spu_thread_write_spu_mb, this); break;
        case 252: wrap(sys_spu_thread_group_disconnect_event_all_threads, this); break;
        default: throw std::runtime_error(ssnprintf("unknown syscall %d", index));
    }
}

const NCallEntry* findNCallEntry(uint32_t fnid, uint32_t& index) {
    auto count = sizeof(ncallTable) / sizeof(NCallEntry);
    for (uint32_t i = 0; i < count; ++i) {
        if (ncallTable[i].fnid == fnid) {
            index = i;
            return &ncallTable[i];
        }
    }
    return nullptr;
}



