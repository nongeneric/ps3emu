#pragma once

#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/list.hpp>
#include <boost/preprocessor/array.hpp>
#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/variadic.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <string>
#include <array>
#include <string.h>
#if DEBUG
#include <assert.h>
#endif

template<typename T>
T enum_cast(unsigned value);

template<typename T>
struct enum_traits { };

template<typename T>
inline std::string to_string(T val) {
    if (!enum_traits<T>::is_flags()) {
        auto i = 0u;
        for (auto v : enum_traits<T>::values()) {
            if (val == v) {
                return enum_traits<T>::names()[i];
            }
            i++;
        }
        return "unset";
    }
    std::string res;
    auto mask = val;
    auto i = 0u;
    for (auto v : enum_traits<T>::values()) {
        if (!!(mask & v)) {
            res += enum_traits<T>::names()[i];
            mask &= ~v;
            if (!!mask && (i < enum_traits<T>::size() - 1)) {
                res += " | ";
            }
        }
        i++;
    }
    return res;
}

template<class T>
T parse_enum(const char* str) {
    int i = 0;
    for (auto v : enum_traits<T>::names()) {
        if (strcmp(v, str) == 0) {
            return enum_traits<T>::values()[i];
        }
        i++;
    }
    throw std::runtime_error("can't parse enum");
}

template <typename T>
void enum_validate(T val) {
#if DEBUG
    if (enum_traits<T>::is_flags()) {
        for (auto v : enum_traits<T>::values()) {
            val &= ~v;
        }
        assert((unsigned)val == 0);
    } else {
        for (auto v : enum_traits<T>::values()) {
            if (v == val)
                return;
        }
        assert(false);
    }
#endif
}

#define ENUMF_NAME_LIST(r, n, elem) \
    BOOST_PP_STRINGIZE( \
        BOOST_PP_TUPLE_ELEM(0, elem) ) \
    BOOST_PP_COMMA()
    
#define ENUMF_VALUE_LIST(r, name, elem) \
    name :: BOOST_PP_TUPLE_ELEM(0, elem) BOOST_PP_COMMA()
    
#define ENUMF_NAME_VALUE_PAIR_LIST(r, data, elem) \
    BOOST_PP_TUPLE_ELEM(0, elem) \
    = \
    BOOST_PP_TUPLE_ELEM(1, elem) \
    BOOST_PP_COMMA()

#define ENUM_IMPL(enum_name, isFlags, ...) \
    enum class enum_name : unsigned long long { \
        BOOST_PP_LIST_FOR_EACH(ENUMF_NAME_VALUE_PAIR_LIST, _, \
            BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__) ) \
    }; \
    inline enum_name operator|(enum_name left, enum_name right) { \
        return (enum_name)((unsigned)left | (unsigned)right); \
    } \
    inline enum_name operator~(enum_name e) { \
        return (enum_name)~(unsigned)e; \
    } \
    inline enum_name operator&(enum_name left, enum_name right) { \
        return (enum_name)((unsigned)left & (unsigned)right); \
    } \
    inline bool operator!(enum_name e) { \
        return !(unsigned)e; \
    } \
    inline void operator&=(enum_name& left, enum_name right) { \
        left = left & right; \
    } \
    inline void operator|=(enum_name& left, enum_name right) { \
        left = left | right; \
    } \
    template<> \
    inline enum_name enum_cast<enum_name>(unsigned value) { \
        enum_validate<enum_name>((enum_name)value); \
        return (enum_name)value; \
    } \
    template<> \
    struct enum_traits<enum_name> { \
        constexpr static auto values() { \
            return std::array<enum_name, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)> { \
                BOOST_PP_LIST_FOR_EACH(ENUMF_VALUE_LIST, enum_name, \
                    BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__) ) \
            }; \
        } \
        constexpr static auto names() { \
            return std::array<const char*, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)> { \
                BOOST_PP_LIST_FOR_EACH(ENUMF_NAME_LIST, _, \
                    BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__) ) \
            }; \
        } \
        constexpr static auto name() { \
            return #enum_name; \
        } \
        constexpr static unsigned size() { return BOOST_PP_VARIADIC_SIZE(__VA_ARGS__); } \
        constexpr static bool is_flags() { return isFlags; } \
    };

#define ENUM(name, ...) ENUM_IMPL(name, false, __VA_ARGS__)
#define ENUMF(name, ...) ENUM_IMPL(name, true, __VA_ARGS__)
