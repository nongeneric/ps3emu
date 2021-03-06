#pragma once

#include <functional>
#include <map>
#include <set>
#include <stdint.h>
#include "../utils.h"
#include "../log.h"
#undef LOG

template <typename T>
struct SimpleCacheItemUpdater {
    uint32_t va;
    uint32_t size;
    std::function<void(T*)> update;
};

template <typename T, typename U>
struct ValueInfo {
    std::unique_ptr<T> value;
    std::unique_ptr<U> updater;
};

template <typename K, typename T, typename U>
struct CacheEntryView {
    K key;
    U* updater;
    T* value;
    bool isDirty;
};

template <typename K, typename T, unsigned CacheSize, typename U = SimpleCacheItemUpdater<T>>
class Cache {
    std::map<K, ValueInfo<T, U>> _store;
    std::set<K> _dirty;
    
public:
    T* retrieve(K const& key) {
        return std::get<0>(retrieveWithUpdater(key));
    }
    
    std::tuple<T*, U*> retrieveWithUpdater(K const& key) {
        auto it = _store.find(key);
        if (it == end(_store))
            return std::tuple<T*, U*>();
        sync(key);
        return std::make_tuple(it->second.value.get(), it->second.updater.get());
    }
    
    void insert(K const& key, T* val, U* updater) {
        ValueInfo<T, U> vi { std::unique_ptr<T>(val), std::unique_ptr<U>(updater) };
        _store.emplace(key, std::move(vi));
        _dirty.insert(key);
    }
    
    void invalidate(uint32_t va, uint32_t size) {
        for (auto& p : _store) {
            if (!intersects(p.second.updater->va, p.second.updater->size, va, size))
                continue;
            INFO(cache) << sformat("invalidating cache entry {:x}, {:x}",
                                     p.second.updater->va,
                                     p.second.updater->size);
            _dirty.insert(p.first);
        }
    }
    
    bool sync(K const& key) {
        auto it = _dirty.find(key);
        if (it != end(_dirty)) {
            auto& info = _store[key];
            info.updater->update(info.value.get());
            _dirty.erase(it);
            return true;
        }
        return false;
    }
    
    void syncAll() {
        for (auto& k : _dirty) {
            auto it = _store.find(k);
            assert(it != end(_store));
            it->second.updater->update(it->second.value.get());
        }
        _dirty.clear();
    }
    
    template <typename F>
    void watch(F action) {
        for (auto& p : _store) {
            if (action(p.second.updater->va, p.second.updater->size)) {
                INFO(cache) << sformat("invalidating cache entry (watch) {:x}, {:x}",
                                         p.second.updater->va,
                                         p.second.updater->size);
                _dirty.insert(p.first);
            }
        }
    }
    
    std::vector<CacheEntryView<K, T, U>> cacheSnapshot() {
        std::vector<CacheEntryView<K, T, U>> res;
        for (auto& entry : _store) {
            CacheEntryView<K, T, U> view;
            view.key = entry.first;
            view.updater = entry.second.updater.get();
            view.value = entry.second.value.get();
            view.isDirty = _dirty.find(view.key) != end(_dirty);
            res.push_back(view);
        }
        return res;
    }
};


