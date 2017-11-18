#pragma once

#include "GLBuffer.h"
#include "GLSync.h"
#include <vector>
#include <memory>
#include <functional>

struct RingBufferEntry {
    GLPersistentCpuBuffer buffer;
    GLSync sync;
    RingBufferEntry(size_t bufferSize);
    RingBufferEntry(RingBufferEntry&&) = default;
};

class RingBuffer {
    std::vector<RingBufferEntry> _entries;
    size_t _currentIndex;

public:
    RingBuffer(size_t count, size_t bufferSize);
    void advance(bool copy = false, bool flush = false);
    GLPersistentCpuBuffer* current();
};
