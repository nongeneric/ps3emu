#include "RingBuffer.h"
#include "string.h"
#include "ps3emu/utils.h"

RingBuffer::RingBuffer(size_t count, std::vector<RingBufferEntryInfo> infos)
    : _infos(infos), _currentIndex(0)
{
    GLint alignment;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);

    auto offset = 0ul;
    for (auto& info : _infos) {
        info.size = align(info.size, alignment);
        _offsets.push_back(offset);
        offset += info.size;
    }
    for (auto i = 0u; i < count; ++i) {
        _entries.emplace_back(offset);
    }
}

void RingBuffer::advance() {
    auto prev = &_entries[_currentIndex];
    _currentIndex = (_currentIndex + 1) % _entries.size();
    auto current = &_entries[_currentIndex];
    prev->sync = GLSync();
    current->sync.clientWait();
    for (auto i = 0u; i < _infos.size(); ++i) {
        if (_infos[i].copyOnAdvance) {
            auto source = prev->buffer.mapped() + _offsets[i];
            auto dest = current->buffer.mapped() + _offsets[i];
            memcpy(dest, source, _infos[i].size);
        }
    }
}

uint8_t* RingBuffer::current(int buffer) {
    return _entries[_currentIndex].buffer.mapped() + _offsets[buffer];
}

void RingBuffer::bindUniform(int buffer, GLuint binding) {
    glBindBufferRange(GL_UNIFORM_BUFFER,
                      binding,
                      _entries[_currentIndex].buffer.handle(),
                      _offsets[buffer],
                      _infos[buffer].size);
}

void RingBuffer::bindElementArray(int buffer) {
//    glBufferAddressRangeNV(
//        GL_ELEMENT_ARRAY_ADDRESS_NV, 0, current()->gpuPointer(), current()->size());
}

RingBufferEntry::RingBufferEntry(size_t bufferSize) : buffer(bufferSize, true) {}
