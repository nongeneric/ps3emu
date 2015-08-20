#include "PPU.h"
#include "../libs/sys.h"
#include <boost/type_traits.hpp>

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

template <int ArgN, class T>
struct get_arg<ArgN, T, typename boost::enable_if< boost::is_pointer<T> >::type> {
    typedef typename boost::remove_pointer<T>::type elem_type;
    elem_type _t;
    uint64_t _va;
    PPU* _ppu;
    inline T value(PPU* ppu) {
        _va = ppu->getGPR(3 + ArgN);
        ppu->readMemory(_va, &_t, sizeof(elem_type));
        _ppu = ppu;
        return &_t; 
    }
    inline ~get_arg() {
        _ppu->writeMemory(_va, &_t, sizeof(elem_type));
    }
};

void nstub_sys_initialize_tls(PPU* ppu) {
    
}

#define ARG(n, f) get_arg<n - 1, \
    typename boost::function_traits< \
        typename boost::remove_pointer<decltype(&f)>::type >::arg##n##_type >() \
            .value(ppu)

#define STUB_2(f) void nstub_##f(PPU* ppu) { \
    ppu->setGPR(3, f(ARG(1, f), ARG(2, f))); \
}

#define STUB_1(f) void nstub_##f(PPU* ppu) { \
    ppu->setGPR(3, f(ARG(1, f))); \
}
            
STUB_2(sys_lwmutex_create);
STUB_1(sys_lwmutex_destroy);
STUB_2(sys_lwmutex_lock);
STUB_1(sys_lwmutex_unlock);

STUB_1(sys_memory_get_user_memory_size);

#define ENTRY(name) { #name, nstub_##name }

NCallEntry ncallTable[] {
    { "", nullptr },
    ENTRY(sys_initialize_tls),
    ENTRY(sys_lwmutex_create),
    ENTRY(sys_lwmutex_destroy),
    ENTRY(sys_lwmutex_lock),
    ENTRY(sys_lwmutex_unlock),
};

void PPU::ncall(uint32_t index) {
    if (index == 0)
        throw std::runtime_error("unknown ncall index");
    ncallTable[index].stub(this);
    setNIP(getLR());
}

void PPU::scall() {
    switch (getGPR(11)) {
        case 352: nstub_sys_memory_get_user_memory_size(this); break;
        default: throw std::runtime_error("unknown syscall");
    }
}

uint32_t PPU::findNCallEntryIndex(std::string name) {
    auto count = sizeof(ncallTable) / sizeof(NCallEntry);
    for (uint32_t i = 0; i < count; ++i) {
        if (ncallTable[i].name == name)
            return i;
    }
    return 0;
}