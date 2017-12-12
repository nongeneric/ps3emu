#pragma once

#include <boost/chrono.hpp>
#include <algorithm>
#include <tuple>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/sum.hpp>
#include <boost/accumulators/statistics/count.hpp>

using counter_clock_t = boost::chrono::steady_clock;
using counter_duration_t = counter_clock_t::duration;
using counter_point_t = counter_clock_t::time_point;

#ifdef LOG_ENABLED
#define TIMED_COUNTER_NOW counter_clock_t::now()
#else
#define TIMED_COUNTER_NOW {}
#endif

class TimedCounter {
    counter_duration_t _window;
    counter_point_t _start;
    int _count = 0;
    counter_duration_t _sum = counter_duration_t(0);
    boost::accumulators::accumulator_set<
        counter_duration_t,
        boost::accumulators::stats<boost::accumulators::tag::count,
                                   boost::accumulators::tag::sum>>
        _acc;
    counter_point_t _lastSumQuery = counter_point_t();
    counter_point_t _lastCountQuery = counter_point_t();
    bool _isOpened = false;

public:
    inline explicit TimedCounter(
        counter_duration_t window = boost::chrono::seconds(1))
        : _window(window) {}

    inline void openRange(counter_point_t now = TIMED_COUNTER_NOW) {
        if (_isOpened)
            return;
        _start = now;
        _isOpened = true;
    }

    inline void closeRange(counter_point_t now = TIMED_COUNTER_NOW) {
        if (!_isOpened)
            return;
        _acc(now - _start);
        _isOpened = false;
    }

    inline std::tuple<counter_duration_t, int> value(counter_point_t now = TIMED_COUNTER_NOW) {
        if (now - _lastSumQuery > _window) {
            _sum = boost::accumulators::sum(_acc);
            _count = boost::accumulators::count(_acc);
            _lastSumQuery = now;
            _acc = {};
        }
        return { _sum, _count };
    }
};

class RangeCloser {
    TimedCounter* _counter;

public:
    RangeCloser(RangeCloser const& other) = delete;
    RangeCloser(RangeCloser&& other) = delete;

    inline RangeCloser& operator=(RangeCloser&& other) {
        _counter = other._counter;
        other._counter = nullptr;
        return *this;
    }

    inline explicit RangeCloser() : _counter(nullptr) { }

    inline explicit RangeCloser(TimedCounter* counter) : _counter(counter) {
        _counter->openRange();
    }

    inline ~RangeCloser() {
        if (_counter) {
            _counter->closeRange();
        }
    }
};
