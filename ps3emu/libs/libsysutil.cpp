#include "libsysutil.h"
#include "ps3emu/rsx/Rsx.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/ContentManager.h"
#include "ps3emu/libs/message.h"
#include "ps3emu/libs/trophy.h"
#include <tbb/concurrent_queue.h>
#include <future>
#include "assert.h"
#include <memory>
#include "../log.h"
#include "../state.h"

#define CELL_SYSUTIL_SYSTEMPARAM_ID_LANG (0x0111)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN (0x0112)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME (0x0113)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT (0x0114)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT (0x0115)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE (0x0116)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME (0x0117)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL (0x0121)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT (0x0123)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_INTERNET_BROWSER_START_RESTRICT (0x0125)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME (0x0131)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT (0x0141)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ (0x0151)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE (0x0152)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE (0x0153)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD (0x0154)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD (0x0155)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF (0x0156)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_MAGNETOMETER (0x0157)

namespace {
    struct CallbackInfo {
        uint32_t va;
        std::vector<uint64_t> args;
        std::promise<uint64_t> promise;
    };

    tbb::concurrent_queue<std::shared_ptr<CallbackInfo>> callbackQueue;
}

int32_t cellSysutilRegisterCallback(int32_t slot, ps3_uintptr_t callback, ps3_uintptr_t userdata) {
    // TODO: implement for handling game termination
    WARNING(libs) << sformat("NOT IMPLEMENTED: cellSysutilRegisterCallback({})", slot);
    return CELL_OK;
}

int64_t cellSysutilCheckCallback(boost::context::continuation* sink) {
    std::shared_ptr<CallbackInfo> info;
    while (callbackQueue.try_pop(info)) {
        fdescr descr;
        g_state.mm->readMemory(info->va, &descr, sizeof(descr));
        auto res = g_state.th->ps3call(descr, &info->args[0], info->args.size(), sink);
        info->promise.set_value(res);
    }
    return CELL_OK;
}

uint64_t emuCallback(uint32_t va, std::vector<uint64_t> const& args, bool wait) {
    auto info = std::make_shared<CallbackInfo>();
    info->va = va;
    info->args = args;
    auto future = info->promise.get_future();
    callbackQueue.push(info);
    if (wait)
        return future.get();
    return 0;
}

emu_void_t cellGcmSetDebugOutputLevel(int32_t level) {
    assert(level == 0 || level == 1 || level == 2);
    return emu_void;
}

int32_t cellSysutilGetSystemParamInt(int32_t id, big_int32_t* value) {
    switch (id) {
        case CELL_SYSUTIL_SYSTEMPARAM_ID_LANG: *value = 1; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN: *value = 1; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT: *value = 1; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT: *value = 1; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE: *value = 180; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME: *value = 0; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL: *value = 9; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT: *value = 0; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_INTERNET_BROWSER_START_RESTRICT: *value = 0; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT: *value = 0; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ: *value = 4; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE: *value = 1; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE: *value = 9; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD: *value = 0; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD: *value = 1; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF: *value = 0; break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_MAGNETOMETER: *value = 0; break;
        default: {
            INFO(libs) << sformat("cellSysutilGetSystemParamInt: unknown param requested {}", id);
            throw std::runtime_error("unknown param");
        }
    }
    return CELL_OK;
}

int32_t cellSysutilGetSystemParamString(int32_t id, ps3_uintptr_t buf, uint32_t bufsize, MainMemory* mm) {
    std::string nickname("PS3-EMU");
    switch (id) {
        case CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME:
            mm->writeMemory(buf, nickname.c_str(), std::min<uint32_t>(nickname.size() + 1, bufsize));
            break;
        case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME:
            mm->writeMemory(buf, nickname.c_str(), std::min<uint32_t>(nickname.size() + 1, bufsize));
            break;
        default: {
            INFO(libs) << sformat("cellSysutilGetSystemParamString: unknown param requested {}", id);
            throw std::runtime_error("unknown param");
        }
    }
    return CELL_OK;
}

#define CELL_SYSCACHE_RET_OK_CLEARED            (0)
#define CELL_SYSCACHE_RET_OK_RELAYED            (1)

int32_t cellSysCacheMount(CellSysCacheParam* param, Process* proc) {
    INFO(libs) << sformat("cellSysCacheMount(id = {})", param->cacheId);
    auto cacheDir = g_state.content->cacheDir();
    auto hostCacheDir = g_state.content->toHost(cacheDir.c_str());
    system(sformat("mkdir -p \"{}\"", hostCacheDir).c_str());
    strcpy(param->getCachePath, cacheDir.c_str());
    cellSysCacheClear(proc);
    return CELL_SYSCACHE_RET_OK_CLEARED;
}

int32_t cellSysCacheClear(Process* proc) {
    auto cacheDir = g_state.content->cacheDir();
    auto hostCacheDir = g_state.content->toHost(cacheDir.c_str());
    INFO(libs) << sformat("cellSysCacheClear {}", hostCacheDir);
    system(sformat("rm -rf \"{}\"/*", hostCacheDir).c_str());
    return CELL_OK;
}

emu_void_t sys_ppu_thread_once(big_int32_t* once_ctrl, const fdescr* init, PPUThread* th) {
    INFO(libs) << sformat("sys_ppu_thread_once({}, [{:x}, {:x}])", *once_ctrl, init->va, init->tocBase);
    if (*once_ctrl == 0) {
        th->setNIP(init->va);
        th->setGPR(2, init->tocBase);
    }
    *once_ctrl = 1;
    return emu_void;
}

int32_t _sys_spu_printf_initialize() {
    return CELL_OK;
}

int32_t _sys_spu_printf_finalize() {
    return CELL_OK;
}

int32_t cellVideoOutGetResolutionAvailability(uint32_t videoOut,
                                              uint32_t resolutionId,
                                              uint32_t aspect,
                                              uint32_t option) {
    assert(option == 0);
    return videoOut == CELL_VIDEO_OUT_PRIMARY &&
           resolutionId == CELL_VIDEO_OUT_RESOLUTION_720 &&
           (aspect == CELL_VIDEO_OUT_ASPECT_AUTO ||
            aspect == CELL_VIDEO_OUT_ASPECT_16_9);
}

int32_t cellSysutilUnregisterCallback(int32_t slot) {
    return CELL_OK;
}
