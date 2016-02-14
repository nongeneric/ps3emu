#include "libsysutil.h"
#include "../ps3emu/MainMemory.h"
#include "../ps3emu/ContentManager.h"
#include "assert.h"
#include <boost/log/trivial.hpp>

#define CELL_SYSUTIL_SYSTEMPARAM_ID_LANG                                                        (0x0111)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN                         (0x0112)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME                                            (0x0113)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT                                         (0x0114)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT                                         (0x0115)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE                                            (0x0116)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME                                          (0x0117)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL                         (0x0121)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT       (0x0123)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_INTERNET_BROWSER_START_RESTRICT     (0x0125)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME                            (0x0131)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT         (0x0141)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ                                       (0x0151)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE                                          (0x0152)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE                                       (0x0153)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD      (0x0154)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD       (0x0155)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF                                         (0x0156)
#define CELL_SYSUTIL_SYSTEMPARAM_ID_MAGNETOMETER                                        (0x0157)

int32_t cellSysutilRegisterCallback(int32_t slot, ps3_uintptr_t callback, ps3_uintptr_t userdata) {
    // TODO: implement for handling game termination
    return CELL_OK;
}

int32_t cellSysutilCheckCallback() {
    // TODO: implement for handling game termination
    return CELL_OK;
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
            BOOST_LOG_TRIVIAL(fatal) << ssnprintf("cellSysutilGetSystemParamInt: unknown param requested %d", id);
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
            BOOST_LOG_TRIVIAL(fatal) << ssnprintf("cellSysutilGetSystemParamString: unknown param requested %d", id);
            throw std::runtime_error("unknown param");
        }
    }
    return CELL_OK;
}

#define CELL_SYSCACHE_RET_OK_CLEARED            (0)
#define CELL_SYSCACHE_RET_OK_RELAYED            (1)

int32_t cellSysCacheMount(CellSysCacheParam* param, Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellSysCacheMount(id = %s)", param->cacheId);
    auto cacheDir = proc->contentManager()->cacheDir();
    auto hostCacheDir = proc->contentManager()->toHost(cacheDir.c_str());
    system(ssnprintf("mkdir -p \"%s\"", hostCacheDir).c_str());
    strcpy(param->getCachePath, cacheDir.c_str());
    cellSysCacheClear(proc);
    return CELL_SYSCACHE_RET_OK_CLEARED;
}

int32_t cellSysCacheClear(Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellSysCacheClear(id = %s)");
    auto cacheDir = proc->contentManager()->cacheDir();
    auto hostCacheDir = proc->contentManager()->toHost(cacheDir.c_str());
    system(ssnprintf("rm -rf \"%s\"/*", hostCacheDir).c_str());
    return CELL_OK;
}

emu_void_t sys_ppu_thread_once(big_int32_t* once_ctrl, const fdescr* init, PPUThread* th) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sys_ppu_thread_once(%d, %x)", *once_ctrl, init);
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
