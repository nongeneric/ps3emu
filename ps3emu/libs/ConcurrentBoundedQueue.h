#pragma once

#include <queue>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

template <typename T>
class ConcurrentBoundedQueue {
    boost::mutex _mutex;
    boost::condition_variable _sizeChanged;
    unsigned _capacity;
    std::queue<T> _queue;
    
public:
    ConcurrentBoundedQueue(unsigned capacity) : _capacity(capacity) {}
    
    void enqueue(T value) {
        boost::unique_lock<boost::mutex> lock(_mutex);
        while (_queue.size() == _capacity) {
            _sizeChanged.wait(lock);
        }
        _queue.push(value);
        _sizeChanged.notify_all();
    }
    
    T dequeue() {
        boost::unique_lock<boost::mutex> lock(_mutex);
        while (_queue.empty()) {
            _sizeChanged.wait(lock);
        }
        auto value = _queue.front();
        _queue.pop();
        _sizeChanged.notify_all();
        return value;
    }
    
    unsigned size() {
        boost::unique_lock<boost::mutex> lock(_mutex);
        return _queue.size();
    }
};
