#pragma once

#include <functional>
#include <map>
#include <set>
#include <stdint.h>
#include "../utils.h"
#include <boost/log/trivial.hpp>

template <typename T>
struct CacheItemUpdater {
    uint32_t va;
    uint32_t size;
    std::function<void(T*)> update;
};

template <typename T>
struct ValueInfo {
    std::unique_ptr<T> value;
    CacheItemUpdater<T> updater;
};

template <typename K, typename T, unsigned CacheSize>
class Cache {
    std::map<K, ValueInfo<T>> _store;
    std::set<K> _dirty;
    
    void actualize() {
        for (auto& k : _dirty) {
            auto it = _store.find(k);
            assert(it != end(_store));
            it->second.updater.update(it->second.value.get());
        }
        _dirty.clear();
    }
public:
    T* retrieve(K const& key) {
        auto it = _store.find(key);
        if (it == end(_store))
            return nullptr;
        return it->second.value.get();
    }
    
    void insert(K const& key, T* val, CacheItemUpdater<T> updater) {
        ValueInfo<T> vi { std::unique_ptr<T>(val), updater };
        _store.emplace(key, std::move(vi));
        _dirty.insert(key);
    }
    
    void invalidate(uint32_t va, uint32_t size) {
        BOOST_LOG_TRIVIAL(trace) << ssnprintf("invalidating cache %x, %x", va, size);
        for (auto& p : _store) {
            auto itemVa = p.second.updater.va;
            auto itemSize = p.second.updater.size;
            if (itemVa > va + size || itemVa + itemSize < va)
                continue;
            _dirty.insert(p.first);
        }
    }
    
    template <typename F>
    void watch(F setBreak) {
        actualize();
        for (auto& p : _store) {
            setBreak(p.second.updater.va, p.second.updater.size);
        }
    }
};
