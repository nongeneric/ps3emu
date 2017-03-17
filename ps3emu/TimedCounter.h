#pragma once

#include <boost/chrono.hpp>

class TimedCounter {
    unsigned _count;
    unsigned _elapsed;
    boost::chrono::steady_clock::time_point _point;
    boost::chrono::microseconds _duration;
    
public:
    TimedCounter(unsigned msDuration = 1000);
    void report();
    bool hasWrapped();
    unsigned elapsed();
};
