#pragma once

#include <map>
#include <assert.h>

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