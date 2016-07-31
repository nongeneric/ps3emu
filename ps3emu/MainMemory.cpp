#include "MainMemory.h"

#include "libs/spu/sysSpu.h"
#include "rsx/Rsx.h"
#include "log.h"

using namespace boost::endian;

bool coversRsxRegsRange(ps3_uintptr_t va, uint len) {
    return !(va + len <= GcmControlRegisters || va >= GcmControlRegisters + 12);
}

static constexpr auto PagePtrMask = ~uintptr_t() - 1;

bool MainMemory::storeMemoryWithReservation(void* dest, 
                                            const void* source, 
                                            uint size,
                                            uint32_t va,
                                            bool cond) {
    
    boost::unique_lock<boost::detail::spinlock> lock(_storeLock);
    bool success = false;
    auto thread = boost::this_thread::get_id();
    for (auto i = (int)_reservations.size() - 1; i >= 0; --i) {
        auto& r = _reservations[i];
        if (r.va <= va && r.va + r.size >= va + size) {
            success |= r.thread == thread;
            _reservations.erase(begin(_reservations) + i);
        }
    }
    if (!cond || success) {
        memcpy(dest, source, size);
    }
    return success;
}

template <bool Read>
void MainMemory::copy(ps3_uintptr_t va, 
          const void* buf, 
          uint len, 
          bool allocate,
          bool locked) {
    assert(va != 0 || len == 0);
    char* chars = (char*)buf;
    for (auto curVa = va; curVa != va + len;) {
        VirtualAddress split { curVa };
        unsigned pageIndex = split.page.u();
        ps3_uintptr_t pageEnd = (pageIndex + 1) * DefaultMainMemoryPageSize;
        auto end = std::min(pageEnd, va + len);
        auto& page = _pages[pageIndex];
        if (!Read) {
            auto ptr = page.ptr.fetch_and(PagePtrMask);
            if (ptr & 1) {
                _memoryWriteHandler(pageIndex * DefaultMainMemoryPageSize,
                                    DefaultMainMemoryPageSize);
            }
        }
        if (!page.ptr) {
            boost::unique_lock<boost::mutex> lock(_pageMutex);
            if (!page.ptr) {
                if (allocate) {
                    page.alloc();
                } else {
                    throw MemoryAccessException();
                }
            }
        }
        
        auto offset = split.offset.u();
        auto source = chars + (curVa - va);
        auto dest = (uint8_t*)((page.ptr & PagePtrMask) + offset);
        auto size = end - curVa;
        if (Read) {
            if (locked)
                _storeLock.lock();
            memcpy(source, dest, size);
            if (locked)
                _storeLock.unlock();
        } else {
            storeMemoryWithReservation(dest, source, size, curVa, false);
        }
        curVa = end;
    }
}

void MainMemory::writeMemory(ps3_uintptr_t va, const void* buf, uint len, bool allocate) {
    if ((va & RawSpuBaseAddr) == RawSpuBaseAddr) {
        writeSpuAddress(va, buf, len);
        return;
    }
    if (coversRsxRegsRange(va, len)) {
        if (!_rsx) {
            throw std::runtime_error("rsx not set");
        }
        if (len == 4) {
            uint32_t val = *(boost::endian::big_uint32_t*)buf;
            if (va == GcmControlRegisters) {
                _rsx->setPut(val);
            } else if (va == GcmControlRegisters + 4) {
                _rsx->setGet(val);
            } else {
                throw std::runtime_error("only put and get rsx registers are supported");
            }
        } else {
            throw std::runtime_error("only one rsx register can be set at a time");
        }
        return;
    }
    
    copy<false>(va, buf, len, allocate, true);
}

void MainMemory::setMemory(ps3_uintptr_t va, uint8_t value, uint len, bool allocate) {
    std::unique_ptr<uint8_t[]> buf(new uint8_t[len]);
    memset(buf.get(), value, len);
    writeMemory(va, buf.get(), len, allocate);
}

struct IForm {
    uint8_t opcode : 6;
    uint8_t _ : 2;
};

void MainMemory::readMemory(ps3_uintptr_t va, void* buf, uint len, bool allocate, bool locked) {
    assert(buf);
    
    if ((va & RawSpuBaseAddr) == RawSpuBaseAddr) {
        assert(len == 4);
        *(big_uint32_t*)buf = readSpuAddress(va);
        return;
    }
    
    if (coversRsxRegsRange(va, len)) {
        assert(len == 4);
        if (va == GcmControlRegisters) {
            *(big_uint32_t*)buf = _rsx->getPut();
            return;
        } else if (va == GcmControlRegisters + 4) {
            *(big_uint32_t*)buf = _rsx->getGet();
            return;
        } else if (va == GcmControlRegisters + 8) {
            *(big_uint32_t*)buf = _rsx->getRef();
            return;
        }
        throw std::runtime_error("unknown rsx register");
    }

    copy<true>(va, buf, len, allocate, locked);
}

bool MainMemory::isAllocated(ps3_uintptr_t va) {
    VirtualAddress split { va };
    return va && (bool)_pages[split.page.u()].ptr;
}

int MainMemory::allocatedPages() {
    int i = 0;
    for (auto j = 0u; j != DefaultMainMemoryPageCount; ++j) {
        if (_pages[j].ptr) {
            i++;
        }
    }
    return i;
}

void MainMemory::reset() {
    if (_pages) {
        for (auto i = 0u; i != DefaultMainMemoryPageCount; ++i) {
            if (!_providedMemoryPages[i]) {
                _pages[i].dealloc();
            }
            _providedMemoryPages[i] = false;
        }
    }
    _pages.reset(new MemoryPage[DefaultMainMemoryPageCount]);
}

// TODO: preallocate the same area for both the internal memory manager and the heap
ps3_uintptr_t MainMemory::malloc(ps3_uintptr_t size) {
    VirtualAddress split{HeapArea};
    VirtualAddress maxSplit{HeapArea + HeapAreaSize};
    
    auto gap = findGap<MemoryPage>(&_pages[split.page.u()],
                                   &_pages[maxSplit.page.u()],
                                   size / DefaultMainMemoryPageSize,
                                   [](auto& page) { return !page.ptr; });
    auto va = std::distance(_pages.get(), gap) * DefaultMainMemoryPageSize;
    setMemory(va, 0, size, true);
    return va;
}

void MainMemory::free(ps3_uintptr_t addr) {
    LOG << "free() not implemented";
}

void MainMemory::setRsx(Rsx* rsx) {
    _rsx = rsx;
}

struct alignas(2) page_byte { uint8_t v; };

void MemoryPage::alloc() {
    auto mem = new page_byte[DefaultMainMemoryPageSize];
    memset(mem, 0, DefaultMainMemoryPageSize);
    uintptr_t e = 0;
    if (!ptr.compare_exchange_strong(e, (uintptr_t)mem))
        delete [] mem;
    assert((ptr & 1) == 0);    
}

void MemoryPage::dealloc() {
    auto mem = ptr.exchange(0);
    if (mem) {
        delete [] (page_byte*)mem;
    }
}

void MainMemory::map(ps3_uintptr_t src, ps3_uintptr_t dest, uint32_t size) {
    if (src == 0 || size == 0)
        throw std::runtime_error("zero size or zero source address");
    auto mbMask = 1024 * 1024 - 1;
    if (src & mbMask || dest & mbMask || size & mbMask)
        throw std::runtime_error("size, source or dest isn't multiple of 1 MB");
    auto srcPageIndex =  VirtualAddress { src }.page.u();
    auto destPageIndex = VirtualAddress { dest }.page.u();
    auto pageCount = size / DefaultMainMemoryPageSize;
    for (auto i = 0u; i < pageCount; ++i) {
        auto& destPage = _pages[destPageIndex + i];
        destPage.alloc();
        auto& srcPage = _pages[srcPageIndex + i];
        srcPage.dealloc();
        srcPage.ptr.store(destPage.ptr.load());
    }
}

uint8_t* MainMemory::getMemoryPointer(ps3_uintptr_t va, uint32_t len) {
    VirtualAddress split { va };
    auto& page = _pages[split.page.u()];
    if (!page.ptr)
        throw std::runtime_error("getting memory pointer for not allocated memory");
    auto offset = split.offset.u();
    if (offset + len > DefaultMainMemoryPageSize)
        throw std::runtime_error("getting memory pointer across page boundaries");
    return (uint8_t*)((page.ptr & PagePtrMask) + offset);
}

void MainMemory::provideMemory(ps3_uintptr_t src, uint32_t size, void* memory) {
    VirtualAddress split { src };
    if (split.offset.u() != 0 || size % DefaultMainMemoryPageSize != 0)
        throw std::runtime_error("expecting multiple of page size");
    auto pageCount = size / DefaultMainMemoryPageSize;
    auto firstPage = split.page.u();
    assert(firstPage < DefaultMainMemoryPageCount);
    for (auto i = 0u; i < pageCount; ++i) {
        auto& page = _pages[firstPage + i];
        auto memoryPtr = (uint8_t*)memory + i * DefaultMainMemoryPageSize;
        if (page.ptr) {
            memcpy(memoryPtr, (void*)(page.ptr & PagePtrMask), DefaultMainMemoryPageSize);
        }
        page.ptr = (uintptr_t)memoryPtr;
        _providedMemoryPages[firstPage + i] = true;
    }
}

void MainMemory::memoryBreak(uint32_t va, uint32_t size) {
    VirtualAddress split { va };
    auto* page = &_pages[split.page.u()];
    for (auto i = 0u; i <= size / DefaultMainMemoryPageSize; ++i) {
        auto ptr = page->ptr.fetch_or(1);
        if (!ptr)
            throw std::runtime_error("memory break on nonexisting page");
    }
}

void MainMemory::memoryBreakHandler(std::function<void(uint32_t, uint32_t)> handler) {
    _memoryWriteHandler = handler;
}

void encodeNCall(MainMemory* mm, ps3_uintptr_t va, uint32_t index) {
    uint32_t ncall = (1 << 26) | index;
    mm->store<4>(va, ncall);
}

MainMemory::MainMemory() {
    reset();
}

void MainMemory::setProc(Process* proc) {
    _proc = proc;
}

void splitSpuRegisterAddress(uint32_t va, uint32_t& id, uint32_t& offset) {
    offset = va & 0x3ffff;
    id = ((va - offset - RawSpuBaseAddr) & ~RawSpuProblemOffset) / RawSpuOffset;
}

void MainMemory::writeSpuAddress(ps3_uintptr_t va, const void* src, uint32_t len) {
    uint32_t id, offset;
    splitSpuRegisterAddress(va, id, offset);
    auto th = findRawSpuThread(id);
    if ((va & RawSpuProblemOffset) == RawSpuProblemOffset) {
        assert(len == 4);
        auto val = *(big_uint32_t*)src;
        th->channels()->mmio_write(offset, val);
        return;
    }
    memcpy(th->ptr(offset), src, len);
}

uint32_t MainMemory::readSpuAddress(ps3_uintptr_t va) {
    uint32_t id, offset;
    splitSpuRegisterAddress(va, id, offset);
    auto th = findRawSpuThread(id);
    if ((va & RawSpuProblemOffset) == RawSpuProblemOffset) {
        return th->channels()->mmio_read(offset);
    }
    return *(big_uint32_t*)th->ptr(offset);
}

void readString(MainMemory* mm, uint32_t va, std::string& str) {
    constexpr size_t chunk = 16;
    str.resize(0);
    auto pos = 0u;
    do {
        str.resize(str.size() + chunk);
        mm->readMemory(va + pos, &str[0] + pos, chunk);
        pos += chunk;
    } while (std::find(begin(str), end(str), 0) == end(str));
    str = str.c_str();
}
