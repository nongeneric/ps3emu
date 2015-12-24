#pragma once

#include <boost/endian/arithmetic.hpp>
#include <atomic>
#include <string>
#include <stdio.h>
#include <inttypes.h>

template <typename T>
struct AdaptType {
    T const& adapt(T const& t) { return t; }
};

template <>
struct AdaptType<std::string> { 
    const char* adapt(std::string const& str) { return str.c_str(); }
};

template <>
struct AdaptType<boost::endian::big_uint64_t> { 
    uint64_t adapt(boost::endian::big_uint64_t str) { return (uint64_t)str; }
};

template <>
struct AdaptType<boost::endian::big_uint32_t> { 
    uint32_t adapt(boost::endian::big_uint32_t str) { return (uint32_t)str; }
};

template <>
struct AdaptType<boost::endian::big_uint16_t> { 
    uint16_t adapt(boost::endian::big_uint16_t str) { return (uint16_t)str; }
};

template <>
struct AdaptType<boost::endian::big_int64_t> { 
    int64_t adapt(boost::endian::big_int64_t str) { return (int64_t)str; }
};

template <>
struct AdaptType<boost::endian::big_int32_t> { 
    int32_t adapt(boost::endian::big_int32_t str) { return (int32_t)str; }
};

template <>
struct AdaptType<boost::endian::big_int16_t> { 
    int16_t adapt(boost::endian::big_int16_t str) { return (int16_t)str; }
};

template <typename... Args>
std::string ssnprintf(const char* f, Args... args) {
    char buf[300];
    snprintf(buf, sizeof buf, f, AdaptType<decltype(args)>().adapt(args)...);
    return std::string(buf);
}

template <typename S, typename D>
union union_cast {
    static_assert(sizeof(S) == sizeof(D), "invalid cast");
    union_cast(S s) : _s(s) { }
    operator D() const {
        return _d;
    }
private:
    S _s;
    D _d;
};

void ums_sleep(uint64_t microseconds);

class spinlock {
    std::atomic<int> _m;
public:
    inline spinlock() : _m(0) { }

    inline void lock() {
        while (_m.exchange(1) == 1) ;
    }
    
    inline void unlock() {
        _m.store(0);
    }
    
    inline void wait() const {
        while (_m.load() == 1) ;
    }
    
    inline bool is_locked() const {
        return _m.load();
    }
};
