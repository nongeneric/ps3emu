#pragma once

#include <queue>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <boost/thread.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

template <template<class> class Q, typename T>
class ConcurrentQueue {
    
    struct WaitingThread {
        boost::thread::id id;
        unsigned priority;
        bool operator<(WaitingThread const& other) const {
            return priority < other.priority;
        }
    };
    
    std::queue<T> _values;
    boost::mutex _mutex;
    boost::condition_variable _cv;
    Q<WaitingThread> _waiting;
    ConcurrentQueue(ConcurrentQueue&) = delete;
    
public:
    ConcurrentQueue() = default;
    
    void send(T const& t) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        _values.push(t);
        _cv.notify_all();
    }
    
    T receive(unsigned priority) {
        auto id = boost::this_thread::get_id();
        boost::unique_lock<boost::mutex> lock(_mutex);
        _waiting.push({id, priority});
        while (_values.empty() || _waiting.top().id != id)
            _cv.wait(lock);
        _waiting.pop();
        auto val = _values.front();
        _values.pop();
        _cv.notify_all();
        return val;
    }
    
    void tryReceive(T* arr, size_t size, size_t* number) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        *number = std::min(size, _values.size());
        for (auto i = 0u; i < *number; ++i) {
            *arr++ = _values.front();
            _values.pop();
        }
    }
    
    void drain() {
        boost::lock_guard<boost::mutex> lock(_mutex);
        while (!_values.empty())
            _values.pop();
    }
    
    unsigned size() {
        boost::lock_guard<boost::mutex> lock(_mutex);
        return _values.size();
    }
};

template <typename T>
using PriorityQueue = std::priority_queue<T, std::vector<T>, std::less<T>>;

template <typename T>
class Queue : public std::queue<T, std::deque<T>> {
public:
    T const& top() {
        return std::queue<T, std::deque<T>>::front();
    }
};

template <typename T>
using ConcurrentPriorityQueue = ConcurrentQueue<PriorityQueue, T>;

template <typename T>
using ConcurrentFifoQueue = ConcurrentQueue<Queue, T>;