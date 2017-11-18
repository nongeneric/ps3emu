#pragma once

#include <tuple>

template <typename... Args>
class GLProxy {
    std::tuple<Args...> _state;
    void (*_f)(Args...);

public:
    GLProxy(void (*f)(Args...)) : _f(f) { }

    void operator()(Args... args) {
        auto t = std::make_tuple(args...);
        if (t != _state) {
            _f(args...);
            _state = t;
        }
    }
};
