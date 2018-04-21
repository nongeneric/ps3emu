#include "AudioAttributes.h"

AudioAttributes::AudioAttributes(uint8_t* mem) : _offset(1), _layout((Layout*)mem) {
    const auto baseTag = 101u;
    _tag = baseTag - fixedLatency;
    _layout->tagShift = 1;
    for (auto i = 0u; i < maxPorts; ++i) {
        _layout->tags[i] = baseTag;
    }
}

void AudioAttributes::setReadIndex(unsigned port, unsigned index) {
    port -= portHwBase;
    _layout->readIndex[port + 1].val = index;
    _layout->tags[port + 1] += 2;
}

unsigned AudioAttributes::getReadIndex(unsigned port) {
    port -= portHwBase;
    return _layout->readIndex[port + 1].val;
}

void AudioAttributes::setTag(uint64_t tag) {
    _tag = tag;
}

void AudioAttributes::insertTimestamp(uint64_t timestamp) {
    for (auto i = 0u; i < maxPorts; ++i) {
        _layout->deltas[i] = tagDelta; // TODO: +1
    }

    _layout->lastTag = tagDelta + _tag;
    _layout->offset = _offset;
    _layout->timestamps[_offset - 1] = timestamp;

    _tag += 2;
    _offset--;
    if (_offset == 0) {
        _offset = timestampCount;
    }
}
