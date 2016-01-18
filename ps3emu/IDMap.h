#pragma once

#include <map>
#include <assert.h>
#include <memory>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

template <typename ID, typename T>
class IDMap {
    ID _maxId = 1;
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
        auto it = _map.find(id);
        assert(it != end(_map));
        return it->second;
    }
};

template <typename ID, typename T>
class ThreadSafeIDMap {
    IDMap<ID, std::shared_ptr<T>> _map;
    boost::mutex _m;
public:
    ID create(std::shared_ptr<T> t) {
        boost::lock_guard<boost::mutex> lock(_m);
        return _map.create(std::move(t));
    }
    
    void destroy(ID id) {
        boost::lock_guard<boost::mutex> lock(_m);
        return _map.destroy(id);
    }
    
    std::shared_ptr<T> get(ID id) {
        boost::lock_guard<boost::mutex> lock(_m);
        return _map.get(id);
    }
};