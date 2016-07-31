#include "InternalMemoryManager.h"

#include "MainMemory.h"
#include "constants.h"
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include <boost/thread.hpp>
#include <assert.h>

using namespace boost::interprocess;

using fit_t = rbtree_best_fit<mutex_family, void*>;

namespace {
    fit_t *fit(MainMemory* mm) {
        return (fit_t*)mm->getMemoryPointer(EmuInternalArea, sizeof(fit_t));
    }
}

void* InternalMemoryManager::allocInternalMemory(uint32_t* ea,
                                                 uint32_t size,
                                                 uint32_t alignment) {
    auto fitp = fit(_mm);
    auto ptr = fitp->allocate_aligned(size, alignment);
    *ea = (uintptr_t)ptr - (uintptr_t)fitp + EmuInternalArea;
    return ptr;
}

InternalMemoryManager::InternalMemoryManager() { }

void InternalMemoryManager::setMainMemory(MainMemory* mm) {
    _mm = mm;
    auto preallocated = new uint8_t[EmuInternalAreaSize];
    _mm->provideMemory(EmuInternalArea, EmuInternalAreaSize, preallocated);
    new(preallocated) fit_t(EmuInternalAreaSize, 0);
}

void InternalMemoryManager::free(uint32_t ea) {
    free(_mm->getMemoryPointer(ea, 1));
}

void InternalMemoryManager::free(void* ptr) {
    fit(_mm)->deallocate(ptr);
}
