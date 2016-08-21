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
#include "libs/libpngdec.h"
#include "libs/libl10n.h"
#include "libs/audio/configuration.h"
#include "libs/audio/libaudio.h"
#include "ppu/CallbackThread.h"
#include "state.h"
#include "log.h"
#include <openssl/sha.h>
#include <boost/type_traits.hpp>
#include <memory>
#include <algorithm>
#include <vector>

namespace boost {
    namespace detail {
        template<typename R, typename T1, typename T2, typename T3, typename T4,
                 typename T5, typename T6, typename T7, typename T8, typename T9,
                 typename T10, typename T11, typename T12>
        struct function_traits_helper<R (*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)>
        {
            BOOST_STATIC_CONSTANT(unsigned, arity = 12);
            typedef R result_type;
            typedef T1 arg1_type;
            typedef T2 arg2_type;
            typedef T3 arg3_type;
            typedef T4 arg4_type;
            typedef T5 arg5_type;
            typedef T6 arg6_type;
            typedef T7 arg7_type;
            typedef T8 arg8_type;
            typedef T9 arg9_type;
            typedef T10 arg10_type;
            typedef T11 arg11_type;
            typedef T12 arg12_type;
        };
        template<typename R, typename T1, typename T2, typename T3, typename T4,
                 typename T5, typename T6, typename T7, typename T8, typename T9,
                 typename T10, typename T11, typename T12, typename T13, typename T14>
        struct function_traits_helper<R (*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)>
        {
            BOOST_STATIC_CONSTANT(unsigned, arity = 14);
            typedef R result_type;
            typedef T1 arg1_type;
            typedef T2 arg2_type;
            typedef T3 arg3_type;
            typedef T4 arg4_type;
            typedef T5 arg5_type;
            typedef T6 arg6_type;
            typedef T7 arg7_type;
            typedef T8 arg8_type;
            typedef T9 arg9_type;
            typedef T10 arg10_type;
            typedef T11 arg11_type;
            typedef T12 arg12_type;
            typedef T13 arg13_type;
            typedef T14 arg14_type;
        };
    }
}

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
};

template <int ArgN>
struct get_arg<ArgN, PPUThread*> {
    inline PPUThread* value(PPUThread* thread) {
        return thread;
    }
};

template <int ArgN>
struct get_arg<ArgN, Process*> {
    inline Process* value(PPUThread* thread) {
        return g_state.proc;
    }
};

template <int ArgN>
struct get_arg<ArgN, MainMemory*> {
    inline MainMemory* value(PPUThread* thread) {
        return g_state.mm;
    }
};

template <int ArgN>
struct get_arg<ArgN, cstring_ptr_t> {
    inline cstring_ptr_t value(PPUThread* thread) {
        cstring_ptr_t cstr;
        readString(g_state.mm, thread->getGPR(3 + ArgN), cstr.str);
        return cstr;
    }
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
    inline ~get_arg() {
        if (_va == 0 || boost::is_const<T>::value)
            return;
        g_state.mm->writeMemory(_va, &_t, sizeof(elem_type));
    }
};

#define ARG(n, f) get_arg<n - 1, \
    typename boost::function_traits< \
        typename boost::remove_pointer<decltype(&f)>::type >::arg##n##_type >() \
            .value(thread)

#define ARG_VOID_PTR(n, lenArg, f) get_arg<n - 1, void*>().value(thread, lenArg)

#define STUB_14(f) void nstub_##f(PPUThread* thread) { \
    thread->setGPR(3, f(ARG(1, f), ARG(2, f), ARG(3, f), ARG(4, f), \
                     ARG(5, f), ARG(6, f), ARG(7, f), ARG(8, f), \
                     ARG(9, f), ARG(10, f), ARG(11, f), ARG(12, f), \
                     ARG(13, f), ARG(14, f))); \
}

#define STUB_12(f) void nstub_##f(PPUThread* thread) { \
    thread->setGPR(3, f(ARG(1, f), ARG(2, f), ARG(3, f), ARG(4, f), \
                     ARG(5, f), ARG(6, f), ARG(7, f), ARG(8, f), \
                     ARG(9, f), ARG(10, f), ARG(11, f), ARG(12, f))); \
}

#define STUB_8(f) void nstub_##f(PPUThread* thread) { \
    thread->setGPR(3, f(ARG(1, f), ARG(2, f), ARG(3, f), ARG(4, f), ARG(5, f), ARG(6, f), ARG(7, f), ARG(8, f))); \
}

#define STUB_7(f) void nstub_##f(PPUThread* thread) { \
    thread->setGPR(3, f(ARG(1, f), ARG(2, f), ARG(3, f), ARG(4, f), ARG(5, f), ARG(6, f), ARG(7, f))); \
}

#define STUB_6(f) void nstub_##f(PPUThread* thread) { \
    thread->setGPR(3, f(ARG(1, f), ARG(2, f), ARG(3, f), ARG(4, f), ARG(5, f), ARG(6, f))); \
}

#define STUB_5(f) void nstub_##f(PPUThread* thread) { \
    thread->setGPR(3, f(ARG(1, f), ARG(2, f), ARG(3, f), ARG(4, f), ARG(5, f))); \
}

#define STUB_4(f) void nstub_##f(PPUThread* thread) { \
    thread->setGPR(3, f(ARG(1, f), ARG(2, f), ARG(3, f), ARG(4, f))); \
}

#define STUB_3(f) void nstub_##f(PPUThread* thread) { \
    thread->setGPR(3, f(ARG(1, f), ARG(2, f), ARG(3, f))); \
}

#define STUB_2(f) void nstub_##f(PPUThread* thread) { \
    thread->setGPR(3, f(ARG(1, f), ARG(2, f))); \
}

#define STUB_1(f) void nstub_##f(PPUThread* thread) { \
    thread->setGPR(3, f(ARG(1, f))); \
}

#define STUB_0(f) void nstub_##f(PPUThread* thread) { \
    thread->setGPR(3, f()); \
}

#define STUB_sys_tty_write(f) void nstub_##f(PPUThread* thread) { \
    auto va = thread->getGPR(3 + 1); \
    auto len = thread->getGPR(3 + 2); \
    std::unique_ptr<char[]> buf(new char[len]); \
    g_state.mm->readMemory(va, buf.get(), len); \
    thread->setGPR(3, f(ARG(1, f), buf.get(), len, ARG(4, f))); \
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

STUB_2(defaultContextCallback);
STUB_4(sys_lwcond_create);
STUB_1(sys_lwcond_destroy);
STUB_2(sys_lwcond_wait);
STUB_1(sys_lwcond_signal);
STUB_1(sys_lwcond_signal_all);
STUB_3(sys_cond_create);
STUB_1(sys_cond_destroy);
STUB_2(sys_cond_wait);
STUB_1(sys_cond_signal);
STUB_1(sys_cond_signal_all);
STUB_3(sys_lwmutex_create);
STUB_1(sys_lwmutex_destroy);
STUB_2(sys_lwmutex_lock);
STUB_1(sys_lwmutex_trylock);
STUB_1(sys_lwmutex_unlock);
STUB_1(sys_time_get_system_time);
STUB_0(_sys_process_atexitspawn);
STUB_0(_sys_process_at_Exitspawn);
STUB_1(sys_memory_get_user_memory_size);
STUB_2(sys_dbg_set_mask_to_ppu_exception_handler);
STUB_sys_tty_write(sys_tty_write);
STUB_1(sys_prx_exitspawn_with_level);
STUB_4(sys_prx_load_module);
STUB_4(sys_prx_start_module);
STUB_4(sys_prx_stop_module);
STUB_3(sys_prx_unload_module);
STUB_4(sys_memory_allocate);
STUB_2(sys_memory_free);
STUB_4(cellVideoOutConfigure);
STUB_3(cellVideoOutGetState);
STUB_2(cellVideoOutGetResolution);
STUB_5(_cellGcmInitBody);
STUB_1(cellGcmSetFlipMode);
STUB_1(cellGcmGetConfiguration);
STUB_2(cellGcmAddressToOffset);
STUB_6(cellGcmSetDisplayBuffer);
STUB_0(cellGcmGetControlRegister);
STUB_1(cellGcmGetLabelAddress);
STUB_1(sys_timer_usleep);
STUB_1(cellGcmGetFlipStatus);
STUB_1(cellGcmResetFlipStatus);
STUB_3(_cellGcmSetFlipCommand);
STUB_2(cellGcmIoOffsetToAddress);
STUB_1(cellGcmGetTiledPitchSize);
STUB_7(sys_fs_open_proxy);
STUB_8(cellGcmSetTileInfo);
STUB_4(_cellGcmSetFlipWithWaitLabel);
STUB_1(cellGcmBindTile);
STUB_1(cellGcmUnbindTile);
STUB_12(cellGcmSetZcull);
STUB_12(cellGcmBindZcull);
STUB_1(cellGcmUnbindZcull);
STUB_4(cellGcmMapMainMemory);
STUB_4(sys_event_queue_create);
STUB_3(sys_event_port_create);
STUB_2(sys_event_port_connect_local);
STUB_1(sys_event_port_disconnect);
STUB_2(sys_event_queue_destroy);
STUB_4(sys_event_queue_receive);
STUB_5(sys_event_queue_tryreceive);
STUB_4(sys_event_port_send);
STUB_1(sys_event_queue_drain);
STUB_1(sys_event_port_destroy);
STUB_1(cellGcmSetFlipHandler);
STUB_1(cellPadInit);
STUB_1(cellPadClearBuf);
STUB_2(cellPadSetPortSetting);
STUB_1(cellKbInit);
STUB_2(cellKbSetCodeType);
STUB_1(cellMouseInit);
STUB_1(sys_time_get_timebase_frequency);
STUB_1(cellGcmSetDefaultCommandBuffer);
STUB_3(cellSysutilRegisterCallback);
STUB_1(cellSysutilUnregisterCallback);
STUB_0(cellSysutilCheckCallback);
STUB_1(cellPadGetInfo2);
STUB_1(cellKbGetInfo);
STUB_1(cellMouseGetInfo);
STUB_2(sys_ppu_thread_get_stack_information);
STUB_1(cellGcmSetDebugOutputLevel);
STUB_2(sys_mutex_create);
STUB_1(sys_mutex_destroy);
STUB_2(sys_mutex_lock);
STUB_1(sys_mutex_trylock);
STUB_1(sys_mutex_unlock);
STUB_5(_sys_heap_create_heap);
STUB_1(cellSysmoduleLoadModule);
STUB_1(cellSysmoduleUnloadModule);
STUB_1(cellSysmoduleIsLoaded);
STUB_0(cellSysmoduleInitialize);
STUB_3(cellFsStat_proxy);
STUB_6(cellFsOpen_proxy);
STUB_4(cellFsLseek);
STUB_1(cellFsClose);
STUB_3(cellFsFstat);
STUB_5(cellFsRead);
STUB_5(cellFsWrite);
STUB_2(sys_time_get_current_time);
STUB_2(sys_time_get_timezone);
STUB_4(sys_semaphore_create);
STUB_1(sys_semaphore_destroy);
STUB_2(sys_semaphore_wait);
STUB_1(sys_semaphore_trywait);
STUB_2(sys_semaphore_post);
STUB_8(sys_ppu_thread_create);
STUB_3(sys_ppu_thread_join);
STUB_2(sys_ppu_thread_exit);
STUB_3(sys_ppu_thread_set_priority);
STUB_3(sys_ppu_thread_get_priority);
STUB_1(sys_process_exit);
STUB_5(cellGameBootCheck);
STUB_4(cellGameDataCheck);
STUB_2(cellGamePatchCheck);
STUB_3(cellGameContentPermit);
STUB_4(cellGameGetParamString);
STUB_2(cellSysutilGetSystemParamInt);
STUB_4(cellSysutilGetSystemParamString);
STUB_2(sceNp2Init);
STUB_2(sceNpInit);
STUB_0(sceNp2Term);
STUB_3(sceNpBasicSetPresence);
STUB_2(sceNpManagerGetContentRatingFlag);
STUB_3(sceNpBasicRegisterHandler);
STUB_3(sceNpDrmIsAvailable2_proxy);
STUB_3(sceNpDrmIsAvailable_proxy);
STUB_1(sceNpManagerGetNpId);
STUB_2(sys_rwlock_create);
STUB_1(sys_rwlock_destroy);
STUB_2(sys_rwlock_rlock);
STUB_1(sys_rwlock_tryrlock);
STUB_1(sys_rwlock_runlock);
STUB_2(sys_rwlock_wlock);
STUB_1(sys_rwlock_trywlock);
STUB_1(sys_rwlock_wunlock);
STUB_2(cellSysCacheMount);
STUB_1(cellSysCacheClear);
STUB_3(cellFsMkdir_proxy);
STUB_4(cellFsGetFreeSize_proxy);
STUB_1(cellFsFsync);
STUB_2(cellFsUnlink_proxy);
STUB_3(cellFsOpendir_proxy);
STUB_3(cellFsReaddir);
STUB_1(cellFsClosedir);
STUB_4(cellFsGetDirectoryEntries);
STUB_3(sys_ppu_thread_once);
STUB_0(_sys_spu_printf_initialize);
STUB_0(_sys_spu_printf_finalize);
STUB_3(cellSpursAttributeSetNamePrefix);
STUB_1(cellSpursAttributeEnableSpuPrintfIfAvailable);
STUB_2(cellSpursAttributeSetSpuThreadGroupType);
STUB_3(cellSpursInitializeWithAttribute);
STUB_3(cellSpursInitializeWithAttribute2);
STUB_7(_cellSpursAttributeInitialize);
STUB_1(cellSpursFinalize);
STUB_2(cellSpursJobChainAttributeSetName);
STUB_3(cellSpursCreateJobChainWithAttribute);
STUB_14(_cellSpursJobChainAttributeInitialize);
STUB_1(cellSpursJoinJobChain);
STUB_1(cellSpursRunJobChain);
STUB_4(sys_spu_thread_read_ls);
STUB_2(sys_spu_initialize);
STUB_4(sys_spu_image_import);
STUB_1(sys_spu_image_close);
STUB_5(sys_spu_thread_group_create);
STUB_7(sys_spu_thread_initialize);
STUB_2(sys_spu_thread_group_start);
STUB_4(sys_spu_thread_group_join);
STUB_2(sys_spu_thread_group_destroy);
STUB_3(sys_spu_thread_get_exit_status);
STUB_1(sceNpDrmVerifyUpgradeLicense2);
STUB_3(sys_raw_spu_create);
STUB_3(sys_spu_image_open);
STUB_3(sys_raw_spu_image_load);
STUB_4(sys_raw_spu_load);
STUB_4(sys_raw_spu_create_interrupt_tag);
STUB_5(sys_interrupt_thread_establish);
STUB_3(sys_raw_spu_set_int_mask);
STUB_3(sys_raw_spu_get_int_stat);
STUB_2(sys_raw_spu_read_puint_mb);
STUB_3(sys_raw_spu_set_int_stat);
STUB_0(sys_interrupt_thread_eoi);
STUB_2(sys_raw_spu_destroy);
STUB_1(sys_ppu_thread_yield);
STUB_1(cellGcmSetGraphicsHandler);
STUB_0(cellGcmGetMaxIoMapSize);
STUB_1(cellGcmGetOffsetTable);
STUB_4(cellGcmMapEaIoAddress);
STUB_5(cellGcmMapEaIoAddressWithFlags);
STUB_1(cellGcmUnmapEaIoAddress);
STUB_1(cellGcmUnmapIoAddress);
STUB_1(callbackThreadQueueWait);
STUB_2(cellGcmSetVBlankHandler);
STUB_4(sys_fs_lseek);
STUB_3(sys_fs_fstat);
STUB_5(sys_fs_read);
STUB_1(sys_fs_close);
STUB_3(cellGcmGetReportDataLocation);
STUB_1(_sys_strlen);
STUB_3(_sys_heap_malloc);
STUB_0(cellPadEnd);
STUB_2(cellPadGetData);
STUB_0(cellKbEnd);
STUB_0(cellMouseEnd);
STUB_4(cellVideoOutGetResolutionAvailability);
STUB_3(cellPngDecCreate);
STUB_6(cellPngDecDecodeData);
STUB_2(cellPngDecClose);
STUB_1(cellPngDecDestroy);
STUB_3(cellPngDecReadHeader);
STUB_5(cellPngDecOpen);
STUB_4(cellPngDecSetParameter);
STUB_1(cellGcmInitDefaultFifoMode);
STUB_0(cellGcmGetTileInfo);
STUB_0(cellGcmGetZcullInfo);
STUB_1(cellGcmSetFlipStatus);
STUB_2(cellGcmGetReportDataAddressLocation);
STUB_1(cellGcmGetLastFlipTime);
STUB_2(_sys_heap_delete_heap);
STUB_2(l10n_get_converter);
STUB_6(l10n_convert_str);
STUB_4(cellAudioOutGetSoundAvailability);
STUB_4(cellAudioOutConfigure);
STUB_3(cellAudioOutGetState);
STUB_0(cellAudioInit);
STUB_3(_sys_heap_memalign);
STUB_2(cellAudioPortOpen);
STUB_2(cellAudioGetPortConfig);
STUB_1(cellAudioPortStart);
STUB_1(cellAudioSetNotifyEventQueue);
STUB_6(_cellSpursTasksetAttributeInitialize);
STUB_2(cellSpursTasksetAttributeSetName);
STUB_3(cellSpursCreateTasksetWithAttribute);
STUB_1(cellSpursDestroyTaskset2);
STUB_3(cellSpursCreateTaskset2);
STUB_3(cellSpursJoinTask2);
STUB_2(_cellSpursTasksetAttribute2Initialize);
STUB_8(cellSpursCreateTask2WithBinInfo);
STUB_2(cellSyncMutexInitialize);
STUB_1(cellSyncMutexTryLock);
STUB_1(cellSyncMutexLock);
STUB_1(cellSyncMutexUnlock);
STUB_6(_cellSpursEventFlagInitialize);
STUB_1(ps3call_then);
STUB_2(sys_process_is_spu_lock_line_reservation_address);
STUB_5(sys_spu_thread_connect_event);
STUB_4(sys_spu_thread_bind_queue);
STUB_5(sys_spu_thread_group_connect_event_all_threads);
STUB_0(sys_process_getpid);
STUB_2(sys_process_get_sdk_version);
STUB_3(sys_spu_thread_group_connect_event);
STUB_4(sys_prx_get_module_id_by_name);
STUB_2(_sys_printf);
STUB_1(_sys_process_get_paramsfo);
STUB_1(sys_ppu_thread_start);
STUB_1(EmuInitLoadedPrxModules);
STUB_1(sys_get_process_info);
STUB_2(sys_prx_register_module);
STUB_1(sys_prx_register_library);
STUB_3(sys_ss_access_control_engine);
STUB_6(sys_prx_load_module_list);
STUB_4(sys_mmapper_allocate_shared_memory);
STUB_4(sys_mmapper_allocate_address);
STUB_4(sys_mmapper_search_and_map);
STUB_0(emuEmptyModuleStart);
STUB_2(sys_prx_get_module_list);
STUB_3(sys_spu_image_get_info);
STUB_3(sys_spu_image_get_modules);

#define ENTRY(name) { #name, calcFnid(#name), nstub_##name }

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
    { "cellGcmSetTile", calcFnid("cellGcmSetTile"), nstub_cellGcmSetTileInfo },
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
    { "cellFsStat", calcFnid("cellFsStat"), nstub_cellFsStat_proxy },
    { "cellFsOpen", calcFnid("cellFsOpen"), nstub_cellFsOpen_proxy },
    { "cellFsMkdir", calcFnid("cellFsMkdir"), nstub_cellFsMkdir_proxy },
    { "cellFsGetFreeSize", calcFnid("cellFsGetFreeSize"), nstub_cellFsGetFreeSize_proxy },
    { "cellFsUnlink", calcFnid("cellFsUnlink"), nstub_cellFsUnlink_proxy },
    { "cellFsOpendir", calcFnid("cellFsOpendir"), nstub_cellFsOpendir_proxy },
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
    { "sceNpDrmIsAvailable2", calcFnid("sceNpDrmIsAvailable2"), nstub_sceNpDrmIsAvailable2_proxy },
    { "sceNpDrmIsAvailable", calcFnid("sceNpDrmIsAvailable"), nstub_sceNpDrmIsAvailable_proxy },
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
    //INFO(libs) << ssnprintf("  -- scall %d at %08x", index, getNIP());
    switch (index) {
        case 352: nstub_sys_memory_get_user_memory_size(this); break;
        case 403: nstub_sys_tty_write(this); break;
        case 988: nstub_sys_dbg_set_mask_to_ppu_exception_handler(this); break;
        case 348: nstub_sys_memory_allocate(this); break;
        case 349: nstub_sys_memory_free(this); break;
        case 141: nstub_sys_timer_usleep(this); break;
        case 801: nstub_sys_fs_open_proxy(this); break;
        case 128: nstub_sys_event_queue_create(this); break;
        case 129: nstub_sys_event_queue_destroy(this); break;
        case 130: nstub_sys_event_queue_receive(this); break;
        case 131: nstub_sys_event_queue_tryreceive(this); break;
        case 133: nstub_sys_event_queue_drain(this); break;
        case 134: nstub_sys_event_port_create(this); break;
        case 136: nstub_sys_event_port_connect_local(this); break;
        case 138: nstub_sys_event_port_send(this); break;
        case 147: nstub_sys_time_get_timebase_frequency(this); break;
        case 49: nstub_sys_ppu_thread_get_stack_information(this); break;
        case 100: nstub_sys_mutex_create(this); break;
        case 101: nstub_sys_mutex_destroy(this); break;
        case 102: nstub_sys_mutex_lock(this); break;
        case 103: nstub_sys_mutex_trylock(this); break;
        case 104: nstub_sys_mutex_unlock(this); break;
        case 145: nstub_sys_time_get_current_time(this); break;
        case 144: nstub_sys_time_get_timezone(this); break;
        case 90: nstub_sys_semaphore_create(this); break;
        case 91: nstub_sys_semaphore_destroy(this); break;
        case 92: nstub_sys_semaphore_wait(this); break;
        case 93: nstub_sys_semaphore_trywait(this); break;
        case 94: nstub_sys_semaphore_post(this); break;
        case 41: nstub_sys_ppu_thread_exit(this); break;
        case 44: nstub_sys_ppu_thread_join(this); break;
        case 47: nstub_sys_ppu_thread_set_priority(this); break;
        case 48: nstub_sys_ppu_thread_get_priority(this); break;
        case 120: nstub_sys_rwlock_create(this); break;
        case 121: nstub_sys_rwlock_destroy(this); break;
        case 122: nstub_sys_rwlock_rlock(this); break;
        case 123: nstub_sys_rwlock_tryrlock(this); break;
        case 124: nstub_sys_rwlock_runlock(this); break;
        case 125: nstub_sys_rwlock_wlock(this); break;
        case 148: // duplicate
        case 126: nstub_sys_rwlock_trywlock(this); break;
        case 127: nstub_sys_rwlock_wunlock(this); break;
        case 105: nstub_sys_cond_create(this); break;
        case 106: nstub_sys_cond_destroy(this); break;
        case 107: nstub_sys_cond_wait(this); break;
        case 108: nstub_sys_cond_signal(this); break;
        case 109: nstub_sys_cond_signal_all(this); break;
        case 182: nstub_sys_spu_thread_read_ls(this); break;
        case 169: nstub_sys_spu_initialize(this); break;
        case 170: nstub_sys_spu_thread_group_create(this); break;
        case 171: nstub_sys_spu_thread_group_destroy(this); break;
        case 172: nstub_sys_spu_thread_initialize(this); break;
        case 173: nstub_sys_spu_thread_group_start(this); break;
        case 178: nstub_sys_spu_thread_group_join(this); break;
        case 165: nstub_sys_spu_thread_get_exit_status(this); break;
        case 160: nstub_sys_raw_spu_create(this); break;
        case 156: nstub_sys_spu_image_open(this); break;
        case 150: nstub_sys_raw_spu_create_interrupt_tag(this); break;
        case 151: nstub_sys_raw_spu_set_int_mask(this); break;
        case 84: nstub_sys_interrupt_thread_establish(this); break;
        case 154: nstub_sys_raw_spu_get_int_stat(this); break;
        case 155: nstub_sys_spu_image_get_info(this); break;
        case 158: nstub_sys_spu_image_close(this); break;
        case 159: nstub_sys_spu_image_get_modules(this); break;
        case 163: nstub_sys_raw_spu_read_puint_mb(this); break;
        case 153: nstub_sys_raw_spu_set_int_stat(this); break;
        case 88: nstub_sys_interrupt_thread_eoi(this); break;
        case 161: nstub_sys_raw_spu_destroy(this); break;
        case 43: nstub_sys_ppu_thread_yield(this); break;
        case 818: nstub_sys_fs_lseek(this); break;
        case 809: nstub_sys_fs_fstat(this); break;
        case 802: nstub_sys_fs_read(this); break;
        case 804: nstub_sys_fs_close(this); break;
        case 137: nstub_sys_event_port_disconnect(this); break;
        case 135: nstub_sys_event_port_destroy(this); break;
        case 14: nstub_sys_process_is_spu_lock_line_reservation_address(this); break;
        case 191: nstub_sys_spu_thread_connect_event(this); break;
        case 193: nstub_sys_spu_thread_bind_queue(this); break;
        case 251: nstub_sys_spu_thread_group_connect_event_all_threads(this); break;
        case 1: nstub_sys_process_getpid(this); break;
        case 25: nstub_sys_process_get_sdk_version(this); break;
        case 185: nstub_sys_spu_thread_group_connect_event(this); break;
        case 30: nstub__sys_process_get_paramsfo(this); break;
        case 52: nstub_sys_ppu_thread_create(this); break;
        case 53: nstub_sys_ppu_thread_start(this); break;
        case 22: nstub_sys_process_exit(this); break;
        case 462: nstub_sys_get_process_info(this); break;
        case 484: nstub_sys_prx_register_module(this); break;
        case 486: nstub_sys_prx_register_library(this); break;
        case 480: nstub_sys_prx_load_module(this); break;
        case 481: nstub_sys_prx_start_module(this); break;
        case 871: nstub_sys_ss_access_control_engine(this); break;
        case 465: nstub_sys_prx_load_module_list(this); break;
        case 332: nstub_sys_mmapper_allocate_shared_memory(this); break;
        case 330: nstub_sys_mmapper_allocate_address(this); break;
        case 337: nstub_sys_mmapper_search_and_map(this); break;
        case 494: nstub_sys_prx_get_module_list(this); break;
        case 482: nstub_sys_prx_stop_module(this); break;
        case 483: nstub_sys_prx_unload_module(this); break;
        case 496: nstub_sys_prx_get_module_id_by_name(this); break;
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



