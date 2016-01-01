#include "Process.h"
#include "utils.h"
#include "../libs/sys.h"
#include "../libs/Controller.h"
#include "../libs/libsysutil.h"
#include "../libs/graphics/gcm.h"
#include "../libs/fs.h"
#include "../libs/cellGame.h"
#include "../libs/sceNp2.h"
#include "../libs/sceNp.h"
#include "../libs/sync/lwmutex.h"
#include "../libs/sync/mutex.h"
#include "../libs/sync/lwcond.h"
#include <boost/log/trivial.hpp>
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
            
    }
}

using namespace emu::Gcm;

uint32_t calcFnid(const char* name) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    static uint8_t suffix[] = {
        0x67, 0x59, 0x65, 0x99, 0x04, 0x25, 0x04, 0x90, 
        0x56, 0x64, 0x27, 0x49, 0x94, 0x89, 0x74, 0x1A
    };
    SHA1_Update(&ctx, (const uint8_t*)name, strlen(name));
    SHA1_Update(&ctx, suffix, sizeof(suffix));
    union {
        uint8_t b[20];
        uint32_t u32[5];
    } md;
    SHA1_Final(md.b, &ctx);
    return md.u32[0];
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
        return thread->proc();
    }
};

template <int ArgN>
struct get_arg<ArgN, MainMemory*> {
    inline MainMemory* value(PPUThread* thread) {
        return thread->mm();
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
        thread->mm()->readMemory(_va, &_t, sizeof(elem_type));
        _thread = thread;
        return &_t; 
    }
    inline ~get_arg() {
        if (_va == 0 || boost::is_const<T>::value)
            return;
        _thread->mm()->writeMemory(_va, &_t, sizeof(elem_type));
    }
};

#define ARG(n, f) get_arg<n - 1, \
    typename boost::function_traits< \
        typename boost::remove_pointer<decltype(&f)>::type >::arg##n##_type >() \
            .value(thread)

#define ARG_VOID_PTR(n, lenArg, f) get_arg<n - 1, void*>().value(thread, lenArg)

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
    thread->mm()->readMemory(va, buf.get(), len); \
    thread->setGPR(3, f(ARG(1, f), buf.get(), len, ARG(4, f))); \
}

void readString(MainMemory* mm, uint32_t va, std::string& str) {
    constexpr size_t chunk = 16;
    str.resize(0);
    auto pos = 0u;
    do {
        str.resize(str.size() + chunk);
        mm->readMemory(va + pos, &str[0] + pos, chunk);
        pos += chunk;
    } while (std::find(begin(str), end(str), 0) == end(str));
}

uint32_t sys_fs_open_proxy(uint32_t path,
                           uint32_t flags,
                           big_uint32_t *fd,
                           uint64_t mode,
                           uint32_t arg,
                           uint64_t size,
                           PPUThread* thread)
{
    std::string pathStr;
    readString(thread->mm(), path, pathStr);
    std::vector<uint8_t> argVec(size + 1);
    if (arg) {
        thread->mm()->readMemory(arg, &argVec[0], size);
    } else {
        argVec[0] = 0;
    }
    return sys_fs_open_impl(pathStr.c_str(), flags, fd, mode, &argVec[0], size);
}

CellFsErrno cellFsStat_proxy(ps3_uintptr_t path, CellFsStat* sb, Process* proc) {
    std::string pathStr;
    readString(proc->mm(), path, pathStr);
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
    readString(proc->mm(), path, pathStr);
    return cellFsOpen(pathStr.c_str(), flags, fd, arg, size, proc);
}

int32_t sys_ppu_thread_create_proxy(
    sys_ppu_thread_t* thread_id,
    ps3_uintptr_t entry,
    uint64_t arg,
    uint32_t prio,
    uint32_t stacksize,
    uint64_t flags,
    uint32_t threadname,
    Process* proc)
{
    std::string namestr;
    readString(proc->mm(), threadname, namestr);
    return sys_ppu_thread_create(thread_id, entry, arg, prio, stacksize, flags, namestr.c_str(), proc);
}

int32_t sceNpDrmIsAvailable2_proxy(const SceNpDrmKey *k_licensee, ps3_uintptr_t path, MainMemory* mm) {
    std::string str;
    readString(mm, path, str);
    return sceNpDrmIsAvailable2(k_licensee, str.c_str());
}

STUB_2(defaultContextCallback);
STUB_3(sys_lwcond_create);
STUB_1(sys_lwcond_destroy);
STUB_2(sys_lwcond_wait);
STUB_1(sys_lwcond_signal);
STUB_1(sys_lwcond_signal_all);
STUB_2(sys_lwmutex_create);
STUB_1(sys_lwmutex_destroy);
STUB_2(sys_lwmutex_lock);
STUB_1(sys_lwmutex_trylock);
STUB_1(sys_lwmutex_unlock);
STUB_1(sys_time_get_system_time);
STUB_0(_sys_process_atexitspawn);
STUB_0(_sys_process_at_Exitspawn);
STUB_1(sys_ppu_thread_get_id);
STUB_1(sys_memory_get_user_memory_size);
STUB_2(sys_dbg_set_mask_to_ppu_exception_handler);
STUB_sys_tty_write(sys_tty_write);
STUB_1(sys_prx_exitspawn_with_level);
STUB_4(sys_memory_allocate);
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
STUB_2(_cellGcmSetFlipCommand);
STUB_2(cellGcmIoOffsetToAddress);
STUB_1(cellGcmGetTiledPitchSize);
STUB_7(sys_fs_open_proxy);
STUB_8(cellGcmSetTileInfo);
STUB_4(_cellGcmSetFlipWithWaitLabel);
STUB_1(cellGcmBindTile);
STUB_1(cellGcmUnbindTile);
STUB_12(cellGcmBindZcull);
STUB_4(cellGcmMapMainMemory);
STUB_4(sys_event_queue_create);
STUB_3(sys_event_port_create);
STUB_2(sys_event_port_connect_local);
STUB_1(cellGcmSetFlipHandler);
STUB_1(cellPadInit);
STUB_1(cellKbInit);
STUB_2(cellKbSetCodeType);
STUB_1(cellMouseInit);
STUB_1(sys_time_get_timebase_frequency);
STUB_1(cellGcmSetDefaultCommandBuffer);
STUB_3(cellSysutilRegisterCallback);
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
STUB_4(_sys_heap_create_heap);
STUB_1(cellSysmoduleLoadModule);
STUB_1(cellSysmoduleUnloadModule);
STUB_1(cellSysmoduleIsLoaded);
STUB_0(cellSysmoduleInitialize);
STUB_3(cellFsStat_proxy);
STUB_6(cellFsOpen_proxy);
STUB_4(cellFsLseek);
STUB_1(cellFsClose);
STUB_5(cellFsRead);
STUB_2(sys_time_get_current_time);
STUB_2(sys_time_get_timezone);
STUB_4(sys_semaphore_create);
STUB_1(sys_semaphore_destroy);
STUB_2(sys_semaphore_wait);
STUB_1(sys_semaphore_trywait);
STUB_2(sys_semaphore_post);
STUB_8(sys_ppu_thread_create_proxy);
STUB_3(sys_ppu_thread_join);
STUB_2(sys_ppu_thread_exit);
STUB_4(sys_initialize_tls);
STUB_1(sys_process_exit);
STUB_4(cellGameBootCheck);
STUB_2(cellGamePatchCheck);
STUB_3(cellGameContentPermit);
STUB_4(cellGameGetParamString);
STUB_2(cellSysutilGetSystemParamInt);
STUB_4(cellSysutilGetSystemParamString);
STUB_2(sceNp2Init);
STUB_0(sceNp2Term);
STUB_3(sceNpBasicSetPresence);
STUB_2(sceNpManagerGetContentRatingFlag);
STUB_3(sceNpBasicRegisterHandler);
STUB_3(sceNpDrmIsAvailable2_proxy);
STUB_1(sceNpManagerGetNpId);

#define ENTRY(name) { #name, calcFnid(#name), nstub_##name }

NCallEntry ncallTable[] {
    ENTRY(defaultContextCallback),
    ENTRY(sys_initialize_tls),
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
    ENTRY(sys_time_get_system_time),
    ENTRY(_sys_process_atexitspawn),
    ENTRY(_sys_process_at_Exitspawn),
    ENTRY(sys_ppu_thread_get_id),
    ENTRY(sys_prx_exitspawn_with_level),
    ENTRY(sys_process_exit),
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
    ENTRY(_cellGcmSetFlipWithWaitLabel),
    ENTRY(cellGcmBindTile),
    ENTRY(cellGcmUnbindTile),
    ENTRY(cellGcmBindZcull),
    ENTRY(cellGcmMapMainMemory),
    ENTRY(cellGcmSetFlipHandler),
    ENTRY(cellPadInit),
    ENTRY(cellKbInit),
    ENTRY(cellKbSetCodeType),
    ENTRY(cellMouseInit),
    ENTRY(cellGcmSetDefaultCommandBuffer),
    ENTRY(cellSysutilRegisterCallback),
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
    ENTRY(cellFsLseek),
    ENTRY(cellFsClose),
    ENTRY(cellFsRead),
    { "sys_ppu_thread_create", calcFnid("sys_ppu_thread_create"), nstub_sys_ppu_thread_create_proxy },
    ENTRY(sys_ppu_thread_join),
    ENTRY(sys_ppu_thread_exit),
    ENTRY(cellGameBootCheck),
    ENTRY(cellGamePatchCheck),
    ENTRY(cellGameContentPermit),
    ENTRY(cellGameGetParamString),
    ENTRY(cellSysutilGetSystemParamInt),
    ENTRY(cellSysutilGetSystemParamString),
    ENTRY(sceNp2Init),
    ENTRY(sceNp2Term),
    ENTRY(sceNpBasicSetPresence),
    ENTRY(sceNpManagerGetContentRatingFlag),
    ENTRY(sceNpBasicRegisterHandler),
    { "sceNpDrmIsAvailable2", calcFnid("sceNpDrmIsAvailable2"), nstub_sceNpDrmIsAvailable2_proxy },
    ENTRY(sceNpManagerGetNpId),
};

void PPUThread::ncall(uint32_t index) {
    if (index >= sizeof(ncallTable) / sizeof(NCallEntry))
        throw std::runtime_error(ssnprintf("unknown ncall index %x", index));
    ncallTable[index].stub(this);
    setNIP(getLR());
}

void PPUThread::scall() {
    auto index = getGPR(11);
    switch (index) {
        case 352: nstub_sys_memory_get_user_memory_size(this); break;
        case 403: nstub_sys_tty_write(this); break;
        case 988: nstub_sys_dbg_set_mask_to_ppu_exception_handler(this); break;
        case 348: nstub_sys_memory_allocate(this); break;
        case 141: nstub_sys_timer_usleep(this); break;
        case 801: nstub_sys_fs_open_proxy(this); break;
        case 128: nstub_sys_event_queue_create(this); break;
        case 134: nstub_sys_event_port_create(this); break;
        case 136: nstub_sys_event_port_connect_local(this); break;
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