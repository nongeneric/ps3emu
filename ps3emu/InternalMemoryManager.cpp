#include "InternalMemoryManager.h"

#include "MainMemory.h"
#include "constants.h"
#include <assert.h>

void* InternalMemoryManager::allocInternalMemory(uint32_t* ea,
                                                 uint32_t size,
                                                 uint32_t alignment) {
    assert(_current + size <= EmuInternalArea + EmuInternalAreaSize);
    _current = align(_current, alignment);
    _mm->setMemory(_current, 0, size, true);
    *ea = _current;
    _current += size;
    return _mm->getMemoryPointer(*ea, size);
}

InternalMemoryManager::InternalMemoryManager() : _current(EmuInternalArea) {}

void InternalMemoryManager::setMainMemory(MainMemory* mm) {
    _mm = mm;
}