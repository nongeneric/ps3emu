#include "InternalMemoryManager.h"

#include "MainMemory.h"
#include "constants.h"
#include "state.h"
#include "log.h"
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include <boost/thread.hpp>
#include <assert.h>

using namespace boost::interprocess;

using fit_t = rbtree_best_fit<mutex_family, void*>;
auto g_maxalignment = 1u << 20;

namespace {
    fit_t *fit(uint32_t base) {
        return (fit_t*)g_state.mm->getMemoryPointer(base, sizeof(fit_t));
    }
}

void* InternalMemoryManager::allocInternalMemory(uint32_t* ea,
                                                 uint32_t size,
                                                 uint32_t alignment) {
    auto lock = boost::lock_guard(_mutex);
    assert(alignment <= g_maxalignment);
    auto fitp = fit(_base);
    assert(fitp->check_sanity());
    auto ptr = fitp->allocate_aligned(size, alignment);
    if (!ptr) {
        *ea = 0;
        return nullptr;
    }
    memset(ptr, 0, size);
    *ea = (uintptr_t)ptr - (uintptr_t)fitp + _base;
    g_state.mm->mark(*ea, size, false, _name);
    assert((*ea & (alignment - 1)) == 0);
    return ptr;
}

InternalMemoryManager::InternalMemoryManager(uint32_t base, uint32_t size, std::string name)
    : _base(base), _name(name) {
    auto preallocated = aligned_alloc(g_maxalignment, size);
    g_state.mm->provideMemory(base, size, preallocated);
    new (preallocated) fit_t(size, 0);
}

void InternalMemoryManager::free(uint32_t ea) {
    free(g_state.mm->getMemoryPointer(ea, 1));
}

void InternalMemoryManager::free(void* ptr) {
    auto lock = boost::lock_guard(_mutex);
    fit(_base)->deallocate(ptr);
}

unsigned InternalMemoryManager::available() const {
    return fit(_base)->get_free_memory();
}
