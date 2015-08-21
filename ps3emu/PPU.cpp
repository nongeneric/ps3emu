#include "PPU.h"
#include <string.h>
#include <functional>
#include <stdio.h>

using namespace boost::endian;

template <class F>
bool mergeAcrossPages(PPU* ppu, F f, uint64_t va, 
                      void* buf, uint len, bool allocate, uint64_t& pageOffset) {
    pageOffset = va % MemoryPage::pageSize;
    if (pageOffset + len > MemoryPage::pageSize) {
        auto firstHalfLen = MemoryPage::pageSize - pageOffset;
        (ppu->*f)(va, buf, firstHalfLen, allocate);
        (ppu->*f)(va + MemoryPage::pageSize, (char*)buf + firstHalfLen, len - firstHalfLen, allocate);
        return true;
    }
    return false;
}

MemoryPage::MemoryPage() : ptr(new uint8_t[pageSize]) {
    memset(ptr.get(), 0, pageSize);
}

void PPU::writeMemory(uint64_t va, const void* buf, uint len, bool allocate) {
    if (va > 0xffffffff) // see Cell_OS-Overview_e
        throw std::runtime_error("writing beyond user adress range");
    assert(va != 0 || len == 0);
    uint64_t pageOffset;
    if (mergeAcrossPages(this, &PPU::writeMemory, va, const_cast<void*>(buf), len, allocate, pageOffset))
        return;
    
    // OPT: effective STL 24 if needed
    uint64_t vaPage = va - pageOffset;
    auto it = _pages.find(vaPage);
    if (it == end(_pages)) {
        if (!allocate)
            throw std::runtime_error("accessing non existing page");
        _pages[vaPage] = MemoryPage();
    }
    memcpy(_pages[vaPage].ptr.get() + pageOffset, buf, len);
}

void PPU::setMemory(uint64_t va, uint8_t value, uint len, bool allocate) {
    // OPT:
    std::unique_ptr<uint8_t> buf(new uint8_t[len]);
    memset(buf.get(), value, len);
    writeMemory(va, buf.get(), len, allocate);
}

struct IForm {
    uint8_t opcode : 6;
    uint8_t _ : 2;
};

void PPU::run() {
}

void PPU::readMemory(uint64_t va, void* buf, uint len, bool allocate) {
    uint64_t pageOffset;
    if (mergeAcrossPages(this, &PPU::readMemory, va, buf, len, allocate, pageOffset))
        return;
    
    // OPT: effective STL 24 if needed
    uint64_t vaPage = va - pageOffset;
    auto it = _pages.find(vaPage);
    if (it == end(_pages))
        throw std::runtime_error("accessing non existing page");
    memcpy(buf, it->second.ptr.get() + pageOffset, len);
}

bool PPU::isAllocated(uint64_t va) {
    auto offset = va % MemoryPage::pageSize;
    auto page = va - offset;
    return _pages.find(page) != end(_pages);
}

int PPU::allocatedPages() {
    return _pages.size();
}
