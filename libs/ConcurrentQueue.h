#pragma once

#include <queue>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <boost/thread.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

enum class QueueReceivingOrder {
    Fifo, Priority
};

template <typename T>
class ConcurrentQueue {
    struct WaitingThread {
        boost::thread::id id;
        unsigned priority;
    };
    
    std::queue<T> _queue;
    QueueReceivingOrder _order;
    boost::mutex _mutex;
    boost::condition_variable _cv;
    std::vector<WaitingThread> _waiting;
    ConcurrentQueue(ConcurrentQueue&) = delete;
public:
    ConcurrentQueue(QueueReceivingOrder order, int capacity) {
        assert(1 <= capacity && capacity < 128);
    }
    
    void send(T const& t) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        _queue.push(t);
        _cv.notify_all();
    }
    
    T receive(unsigned priority) {
        auto id = boost::thread::get_id();
        boost::unique_lock<boost::mutex> lock(_mutex);
        _waiting.emplace_back(id, priority);
        for (;;) {
            _cv.wait(lock);
            if (_queue.empty())
                continue;
            auto thisThread = std::find_if(begin(_waiting), end(_waiting), [=](auto& t) {
                return t.id == id;
            })
            assert(thisThread != end(_waiting));
            bool handled = false;
            if (_order == QueueReceivingOrder::Fifo) {
                if (thisThread == begin(_waiting)) {
                    handled = true;
                }
                // there are threads that have been waiting longer
                // skip this turn
            } else { // priority
                auto worthyThread = std::find_if(begin(_waiting), end(_waiting), [=](auto& t) {
                    return t.priority > priority;
                });
                if (worthyThread != end(_waiting)) {
                    handled = true;
                }
                // there are threads with the higher priority
                // skip this turn
            }
            if (handled) {
                auto res = _queue.front();
                _queue.pop();
                _waiting.erase(thisThread);
                if (!_waiting.empty()) {
                    // other threads could have already taken their turn
                    // and decided to skip it because of this thread having
                    // the higher priority, so other threads need to try again
                    _cv.notify_all();
                }
                return res;
            }
        }
    }
    
    void tryReceive(T* arr, size_t size, size_t* number) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        if (!_waiting.empty()) {
            // after all the waiting threads have finished,
            // it is possible the queue will still have items
            // in that case returning zero should be safe
            *number = 0;
            return;
        }
        size = std::min(size, _queue.size());
        for (int i = 0; i < size; ++i) {
            *arr = _queue.front();
            arr++;
            _queue.pop();
        }
        *number = size;
    }
    
    void drain() {
        boost::lock_guard<boost::mutex> lock(_mutex);
        while (!_queue.empty()) _queue.pop();
    }
};