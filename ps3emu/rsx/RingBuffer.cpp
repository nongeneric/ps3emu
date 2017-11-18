#include "RingBuffer.h"
#include "string.h"

RingBuffer::RingBuffer(size_t count, size_t bufferSize) {
    for (auto i = 0u; i < count; ++i) {
        _entries.emplace_back(bufferSize);
    }
    _currentIndex = 0;
}

void RingBuffer::advance(bool copy, bool flush) {
    auto prev = &_entries[_currentIndex];
    _currentIndex = (_currentIndex + 1) % _entries.size();
    auto current = &_entries[_currentIndex];
    prev->sync = GLSync();
    current->sync.clientWait(flush);
    if (copy) {
        memcpy(current->buffer.mapped(), prev->buffer.mapped(), current->buffer.size());
    }
}

GLPersistentCpuBuffer* RingBuffer::current() {
    return &_entries[_currentIndex].buffer;
}

RingBufferEntry::RingBufferEntry(size_t bufferSize) : buffer(bufferSize) {}
