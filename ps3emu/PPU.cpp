#include "PPU.h"
#include <string.h>
#include <functional>
#include <stdio.h>
#include <assert.h>

using namespace boost::endian;

bool coversRsxRegsRange(ps3_uintptr_t va, uint len) {
    return !(va + len <= GcmControlRegisters || va >= GcmControlRegisters + 12);
}

template <class F>
bool mergeAcrossPages(PPU* ppu, F f, ps3_uintptr_t va, 
                      void* buf, uint len, bool allocate) {
    auto pageOffset = va % DefaultMainMemoryPageSize;
    if (pageOffset + len > DefaultMainMemoryPageSize) {
        auto firstHalfLen = DefaultMainMemoryPageSize - pageOffset;
        (ppu->*f)(va, buf, firstHalfLen, allocate);
        (ppu->*f)(va + DefaultMainMemoryPageSize, (char*)buf + firstHalfLen, len - firstHalfLen, allocate);
        return true;
    }
    return false;
}

void PPU::writeMemory(ps3_uintptr_t va, const void* buf, uint len, bool allocate) {
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
    
    assert(va != 0 || len == 0);
    if (mergeAcrossPages(this, &PPU::writeMemory, va, const_cast<void*>(buf), len, allocate))
        return;
    
    VirtualAddress split { va };
    auto& page = _pages[split.page.u()];
    if (!page.ptr && !allocate)
        throw std::runtime_error("accessing non existing page");
    page.alloc();
    memcpy(page.ptr + split.offset.u(), buf, len);
}

void PPU::setMemory(ps3_uintptr_t va, uint8_t value, uint len, bool allocate) {
    std::unique_ptr<uint8_t[]> buf(new uint8_t[len]);
    memset(buf.get(), value, len);
    writeMemory(va, buf.get(), len, allocate);
}

struct IForm {
    uint8_t opcode : 6;
    uint8_t _ : 2;
};

void PPU::run() {
}

void PPU::readMemory(ps3_uintptr_t va, void* buf, uint len, bool allocate) {
    assert(buf);
    
    if (coversRsxRegsRange(va, len)) {
        assert(len == 4);        
        if (va == GcmControlRegisters + 8) {
            *(big_uint32_t*)buf = _rsx->getRef();
            return;
        }
        throw std::runtime_error("reading rsx registers not implemented");
    }
    
    if (mergeAcrossPages(this, &PPU::readMemory, va, buf, len, allocate))
        return;
    
    VirtualAddress split { va };
    auto& page = _pages[split.page.u()];
    if (!page.ptr) {
        if (allocate) {
            page.alloc();
        } else {
            throw std::runtime_error("accessing non existing page");
        }
    }
    memcpy(buf, page.ptr + split.offset.u(), len);
}

bool PPU::isAllocated(ps3_uintptr_t va) {
    VirtualAddress split { va };
    return (bool)_pages[split.page.u()].ptr;
}

int PPU::allocatedPages() {
    int i = 0;
    for (auto j = 0u; j != DefaultMainMemoryPageCount; ++j) {
        if (_pages[j].ptr) {
            i++;
        }
    }
    return i;
}

void PPU::reset() {
    if (_pages) {
        for (auto i = 0u; i != DefaultMainMemoryPageCount; ++i) {
            if (!_providedMemoryPages[i]) {
                _pages[i].dealloc();
            }
            _providedMemoryPages[i] = false;
        }
    }
    _pages.reset(new MemoryPage[DefaultMainMemoryPageCount]);
    for (auto& r : _GPR)
        r = 0;
    for (auto& r : _FPR)
        r = 0;
    for (auto& r : _V)
        r = 0;
    _systemStart = boost::chrono::high_resolution_clock::now();
}

ps3_uintptr_t PPU::malloc(ps3_uintptr_t size) {
    ps3_uintptr_t va = 0;
    for (auto i = 0u; i != DefaultMainMemoryPageCount; ++i) {
        auto& p =_pages[i];
        if (p.ptr) {
            va = std::max(va, i * DefaultMainMemoryPageSize);
        }
    }
    va += DefaultMainMemoryPageSize;
    setMemory(va, 0, size, true);
    return va;
}

void PPU::setRsx(Rsx* rsx) {
    _rsx = rsx;
}

PPU::PPU() {
    reset();
}

void MemoryPage::alloc() {
    if (ptr)
        return;
    ptr = new uint8_t[DefaultMainMemoryPageSize];
    memset(ptr, 0, DefaultMainMemoryPageSize);
}

void MemoryPage::dealloc() {
    if (ptr) {
        delete [] ptr;
    }
    ptr = nullptr;
}

void PPU::shutdown() {
    if (_rsx) {
        _rsx->shutdown();
    }
}

void PPU::map(ps3_uintptr_t src, ps3_uintptr_t dest, uint32_t size) {
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
        srcPage.ptr = destPage.ptr;
    }
}

void PPU::setELFLoader(ELFLoader* elfLoader) {
    _elfLoader = elfLoader;
}

ELFLoader* PPU::getELFLoader() {
    return _elfLoader;
}

void PPU::allocPage(void** ptr, ps3_uintptr_t* va) {
    *va = malloc(DefaultMainMemoryPageSize);
    VirtualAddress split { *va };
    assert(split.offset.u() == 0);
    *ptr = _pages[split.page.u()].ptr;
}

uint8_t* PPU::getMemoryPointer(ps3_uintptr_t va, uint32_t len) {
    VirtualAddress split { va };
    auto page = _pages[split.page.u()];
    if (!page.ptr)
        throw std::runtime_error("getting memory pointer for not allocated memory");
    auto offset = split.offset.u();
    if (offset + len > DefaultMainMemoryPageSize)
        throw std::runtime_error("getting memory pointer acress page boundaries");
    return page.ptr + offset;
}

void PPU::provideMemory(ps3_uintptr_t src, uint32_t size, void* memory) {
    VirtualAddress split { src };
    if (split.offset.u() != 0 || size % DefaultMainMemoryPageSize != 0)
        throw std::runtime_error("expecting multiple of page size");
    auto pageCount = size / DefaultMainMemoryPageSize;
    auto firstPage = split.page.u();
    for (auto i = 0u; i < pageCount; ++i) {
        auto& page = _pages[firstPage + i];
        auto memoryPtr = (uint8_t*)memory + i * DefaultMainMemoryPageSize;
        if (page.ptr) {
            memcpy(memoryPtr, page.ptr, DefaultMainMemoryPageSize);
        }
        page.ptr = memoryPtr;
        _providedMemoryPages[firstPage + i] = true;
    }
}

uint64_t PPU::getFrequency() {
    return 79800000;
}

uint64_t PPU::getTimeBase() {
    auto now = boost::chrono::high_resolution_clock::now();
    auto diff = now - _systemStart;
    auto us = boost::chrono::duration_cast<boost::chrono::microseconds>(diff);
    return 1000000 * us.count() * getFrequency();
}