#pragma once

#include "GLBuffer.h"
#include "GLSync.h"
#include <vector>
#include <memory>
#include <functional>

struct RingBufferEntry {
    GLPersistentCpuBuffer buffer;
    GLQuerySync sync;
    RingBufferEntry(size_t bufferSize);
    RingBufferEntry(RingBufferEntry&&) = default;
};

struct RingBufferEntryInfo {
    size_t size;
    bool copyOnAdvance;
};

class RingBuffer {
    std::vector<RingBufferEntry> _entries;
    std::vector<RingBufferEntryInfo> _infos;
    std::vector<uint64_t> _offsets;
    size_t _currentIndex;

public:
    RingBuffer(size_t count, std::vector<RingBufferEntryInfo> infos);
    void advance();
    uint8_t* current(int buffer);
    void bindUniform(int buffer, GLuint binding);
    void bindElementArray(int buffer);
};
