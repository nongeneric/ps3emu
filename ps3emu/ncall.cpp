#include "PPU.h"
#include "utils.h"
#include "../libs/sys.h"
#include "../libs/graphics/gcm.h"
#include <boost/type_traits.hpp>
#include <memory>

using namespace emu::Gcm;

struct NCallEntry {
    const char* name;
    void (*stub)(PPU*);
};

template <int ArgN, class T, class Enable = void>
struct get_arg {
    inline T value(PPU* ppu) {
        return (T)ppu->getGPR(3 + ArgN);
    }
};

template <int ArgN>
struct get_arg<ArgN, PPU*> {
    inline PPU* value(PPU* ppu) {
        return ppu;
    }
};

template <int ArgN, class T>
struct get_arg<ArgN, T, typename boost::enable_if< boost::is_pointer<T> >::type> {
    typedef typename boost::remove_pointer<T>::type elem_type;
    elem_type _t;
    uint64_t _va;
    PPU* _ppu;
    inline T value(PPU* ppu) {
        _va = (ps3_uintptr_t)ppu->getGPR(3 + ArgN);
        if (_va == 0)
            return nullptr;
        ppu->readMemory(_va, &_t, sizeof(elem_type));
        _ppu = ppu;
        return &_t; 
    }
    inline ~get_arg() {
        if (_va == 0)
            return;
        _ppu->writeMemory(_va, &_t, sizeof(elem_type));
    }
};

void nstub_sys_initialize_tls(PPU* ppu) {
    
}

void nstub_sys_process_exit(PPU* ppu) {
    throw ProcessFinishedException();
}

#define ARG(n, f) get_arg<n - 1, \
    typename boost::function_traits< \
        typename boost::remove_pointer<decltype(&f)>::type >::arg##n##_type >() \
            .value(ppu)

#define ARG_VOID_PTR(n, lenArg, f) get_arg<n - 1, void*>().value(ppu, lenArg)

#define STUB_5(f) void nstub_##f(PPU* ppu) { \
    ppu->setGPR(3, f(ARG(1, f), ARG(2, f), ARG(3, f), ARG(4, f), ARG(5, f))); \
}

#define STUB_4(f) void nstub_##f(PPU* ppu) { \
    ppu->setGPR(3, f(ARG(1, f), ARG(2, f), ARG(3, f), ARG(4, f))); \
}

#define STUB_3(f) void nstub_##f(PPU* ppu) { \
    ppu->setGPR(3, f(ARG(1, f), ARG(2, f), ARG(3, f))); \
}

#define STUB_2(f) void nstub_##f(PPU* ppu) { \
    ppu->setGPR(3, f(ARG(1, f), ARG(2, f))); \
}

#define STUB_1(f) void nstub_##f(PPU* ppu) { \
    ppu->setGPR(3, f(ARG(1, f))); \
}

#define STUB_0(f) void nstub_##f(PPU* ppu) { \
    ppu->setGPR(3, f()); \
}

#define STUB_sys_tty_write(f) void nstub_##f(PPU* ppu) { \
    auto va = ppu->getGPR(3 + 1); \
    auto len = ppu->getGPR(3 + 2); \
    std::unique_ptr<char[]> buf(new char[len]); \
    ppu->readMemory(va, buf.get(), len); \
    ppu->setGPR(3, f(ARG(1, f), buf.get(), len, ARG(4, f))); \
}

STUB_2(sys_lwmutex_create);
STUB_1(sys_lwmutex_destroy);
STUB_2(sys_lwmutex_lock);
STUB_1(sys_lwmutex_unlock);
STUB_0(sys_time_get_system_time);
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
STUB_5(cellGcmSetDisplayBuffer);
STUB_0(cellGcmGetControlRegister);
STUB_1(cellGcmGetLabelAddress);
STUB_1(sys_timer_usleep);
STUB_1(cellGcmGetFlipStatus);
STUB_1(cellGcmResetFlipStatus);
STUB_1(_cellGcmSetFlipCommand);

#define ENTRY(name) { #name, nstub_##name }

NCallEntry ncallTable[] {
    { "", nullptr },
    ENTRY(sys_initialize_tls),
    ENTRY(sys_lwmutex_create),
    ENTRY(sys_lwmutex_destroy),
    ENTRY(sys_lwmutex_lock),
    ENTRY(sys_lwmutex_unlock),
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
};

void PPU::ncall(uint32_t index) {
    if (index >= sizeof(ncallTable) / sizeof(NCallEntry))
        throw std::runtime_error(ssnprintf("unknown ncall index %d", index));
    ncallTable[index].stub(this);
    setNIP(getLR());
}

void PPU::scall() {
    auto index = getGPR(11);
    switch (index) {
        case 352: nstub_sys_memory_get_user_memory_size(this); break;
        case 403: nstub_sys_tty_write(this); break;
        case 988: nstub_sys_dbg_set_mask_to_ppu_exception_handler(this); break;
        case 348: nstub_sys_memory_allocate(this); break;
        case 141: nstub_sys_timer_usleep(this); break;
        default: throw std::runtime_error(ssnprintf("unknown syscall %d", index));
    }
}

uint32_t PPU::findNCallEntryIndex(std::string name) {
    auto count = sizeof(ncallTable) / sizeof(NCallEntry);
    for (uint32_t i = 0; i < count; ++i) {
        if (ncallTable[i].name == name) {
            return i;
        }
    }
    return 0;
}