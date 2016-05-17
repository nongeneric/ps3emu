#pragma once

#include <map>
#include <assert.h>
#include <memory>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/optional.hpp>

template <typename ID, typename T, int InitialID = 1>
class IDMap {
    ID _maxId = InitialID;
    std::map<ID, T> _map;
public:
    ID create(T&& t) {
        assert(_maxId <= 2000); // TODO: implement id reuse
        _map.insert(std::make_pair(_maxId, std::move(t)));
        return _maxId++;
    }
    
    void destroy(ID id) {
        auto it = _map.find(id);
        assert(it != end(_map));
        _map.erase(it);
    }
    
    T& get(ID id) {
        auto v = try_get(id);
        assert(v);
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
    
    std::map<ID, T>& map() {
        return _map.map();
    }
};