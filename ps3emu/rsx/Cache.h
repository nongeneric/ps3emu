#pragma once

#include <functional>
#include <map>
#include <set>
#include <stdint.h>
#include "../utils.h"
#include <boost/log/trivial.hpp>

template <typename T>
struct SimpleCacheItemUpdater {
    uint32_t va;
    uint32_t size;
    std::function<void(T*)> update;
    std::function<void(T*, std::vector<uint8_t>&)> updateWithBlob;
};

template <typename T, typename U>
struct ValueInfo {
    std::unique_ptr<T> value;
    std::unique_ptr<U> updater;
};

template <typename K, typename T, unsigned CacheSize, typename U = SimpleCacheItemUpdater<T>>
class Cache {
    std::map<K, ValueInfo<T, U>> _store;
    std::set<K> _dirty;
    
    void actualize() {
        for (auto& k : _dirty) {
            auto it = _store.find(k);
            assert(it != end(_store));
            it->second.updater->update(it->second.value.get());
        }
        _dirty.clear();
    }
public:
    T* retrieve(K const& key) {
        return std::get<0>(retrieveWithUpdater(key));
    }
    
    std::tuple<T*, U*> retrieveWithUpdater(K const& key) {
        auto it = _store.find(key);
        if (it == end(_store))
            return std::tuple<T*, U*>();
        return std::make_tuple(it->second.value.get(), it->second.updater.get());
    }
    
    void insert(K const& key, T* val, U* updater) {
        ValueInfo<T, U> vi { std::unique_ptr<T>(val), std::unique_ptr<U>(updater) };
        _store.emplace(key, std::move(vi));
        _dirty.insert(key);
    }
    
    void invalidate(uint32_t va, uint32_t size) {
        BOOST_LOG_TRIVIAL(trace) << ssnprintf("invalidating cache %x, %x", va, size);
        for (auto& p : _store) {
            auto itemVa = p.second.updater->va;
            auto itemSize = p.second.updater->size;
            if (itemVa > va + size || itemVa + itemSize < va)
                continue;
            _dirty.insert(p.first);
        }
    }
    
    template <typename F>
    void watch(F setBreak) {
        actualize();
        for (auto& p : _store) {
            setBreak(p.second.updater->va, p.second.updater->size);
        }
    }
};


