#pragma once

#include <map>
#include <set>
#include <assert.h>
#include <memory>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/optional.hpp>

template <typename ID, typename T, int InitialID = 1>
class IDMap {
    ID _maxId = InitialID;
    std::map<ID, T> _map;
    std::set<ID> _free;

public:
    ID create(T&& t) {
        ID id;
        if (!_free.empty()) {
            auto it = begin(_free);
            id = *it;
            _free.erase(it);
        } else {
            id = _maxId++;
        }
        _map.insert(std::make_pair(id, std::move(t)));
        return id;
    }
    
    void destroy(ID id) {
        auto it = _map.find(id);
        assert(it != end(_map));
        _map.erase(it);
        _free.insert(id);
    }
    
    T& get(ID id) {
        auto v = try_get(id);
        assert(!!v);
        return v.value();
    }
    
    boost::optional<T&> try_get(ID id) {
        auto it = _map.find(id);
        if (it == end(_map))
            return boost::none;
        return it->second;
    }
    
    std::map<ID, T>& map() {
        return _map;
    }
};

template <typename ID, typename T, int InitialID = 1>
class ThreadSafeIDMap {
    IDMap<ID, T, InitialID> _map;
    boost::mutex _m;
public:
    ID create(T t) {
        boost::lock_guard<boost::mutex> lock(_m);
        return _map.create(std::move(t));
    }
    
    void destroy(ID id) {
        boost::lock_guard<boost::mutex> lock(_m);
        return _map.destroy(id);
    }
    
    T get(ID id) {
        boost::lock_guard<boost::mutex> lock(_m);
        return _map.get(id);
    }
    
    boost::optional<T&> try_get(ID id) {
        boost::lock_guard<boost::mutex> lock(_m);
        return _map.try_get(id);
    }   
    
    std::map<ID, T> map() {
        boost::lock_guard<boost::mutex> lock(_m);
        return _map.map();
    }
    
    ID search(std::function<bool(T&)> predicate) {
        boost::lock_guard<boost::mutex> lock(_m);
        for (auto& pair : _map.map()) {
            if (predicate(pair.second))
                return pair.first;
        }
        return 0;
    }
};
