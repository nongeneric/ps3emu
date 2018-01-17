#pragma once

#include "ps3emu/state.h"
#include "ps3emu/ppu/PPUThread.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/libs/sys_defs.h"

#include <boost/hana.hpp>
#include <boost/context/all.hpp>

namespace hana = boost::hana;
using namespace hana::literals;

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
    inline Process* value(PPUThread*) {
        return g_state.proc;
    }
    inline void destroy() {}
};

template <int ArgN>
struct get_arg<ArgN, boost::context::continuation*> {
    inline boost::context::continuation* value(PPUThread*) {
        return nullptr;
    }
    inline void destroy() {}
};

template <int ArgN>
struct get_arg<ArgN, MainMemory*> {
    inline MainMemory* value(PPUThread*) {
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
constexpr auto make_type_tuple(std::function<R(Args...)> f) {
    return hana::make_tuple(hana::type_c<Args>...);
}

template <typename R, typename... Args>
constexpr auto make_type_tuple(R (*)(Args...)) {
    return hana::make_tuple(hana::type_c<Args>...);
}

constexpr bool containsContinuationArgument(auto types) {
    auto argCount = hana::int_c<hana::length(types)>;
    if constexpr(argCount.value > 0) {
        if constexpr(types[argCount - 1_c] == hana::type_c<boost::context::continuation*>) {
            return true;
        }
    }
    return false;
}

template <typename F>
auto wrap(F f, PPUThread* th) {
    auto types = make_type_tuple(f);
    auto argCount = hana::int_c<hana::length(types)>;
    auto ns = make_n_tuple(hana::make_tuple(), argCount);
    auto pairs = hana::zip(ns, types);
    auto holders = hana::transform(pairs, [](auto t) {
        return get_arg<t[0_c].value, typename decltype(+t[1_c])::type>();
    });

    if constexpr(containsContinuationArgument(types)) {
        boost::context::continuation source =
            boost::context::callcc([holders, &f, th](boost::context::continuation&& sink) mutable {
                auto values = hana::transform(holders, [&](auto& h) {
                    return h.value(th);
                });
                auto updatedValues = hana::append(hana::drop_back(values), &sink);
                th->setGPR(3, hana::unpack(updatedValues, f));
                hana::for_each(holders, [](auto& h) {
                    h.destroy();
                });
                return std::move(sink);
            });
        if (source) { // ps3call has been called by the coroutine
            assert(!th->_ps3calls.empty());
            th->_pscallContinuation.push(std::move(source));
        }
    } else {
        auto values = hana::transform(holders, [&](auto& h) {
            return h.value(th);
        });
        th->setGPR(3, hana::unpack(values, f));
        hana::for_each(holders, [&](auto& h) {
            h.destroy();
        });
    }
}
