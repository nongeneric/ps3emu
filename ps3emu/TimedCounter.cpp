#include "TimedCounter.h"

TimedCounter::TimedCounter(unsigned msDuration) {
    _duration = boost::chrono::milliseconds(msDuration);
}

void TimedCounter::report() {
    _count++;
}

bool TimedCounter::hasWrapped() {
    auto now = boost::chrono::steady_clock::now();
    if (now - _point > _duration) {
        _elapsed = _count;
        _count = 0;
        _point = now;
        return true;
    }
    return false;
}

unsigned int TimedCounter::elapsed() {
    return _elapsed;
}
