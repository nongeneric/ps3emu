#include "PPU.h"

#include "disassm/disasm.h"
#include <string.h>
#include <functional>
#include <stdio.h>

using namespace boost::endian;

bool mergeAcrossPages(PPU* ppu, decltype(&PPU::writeMemory) f, uint64_t va, 
                      void* buf, uint len, uint64_t& pageOffset) {
    pageOffset = va % MemoryPage::pageSize;
    if (pageOffset + len > MemoryPage::pageSize) {
        auto firstHalfLen = MemoryPage::pageSize - pageOffset;
        (ppu->*f)(va, buf, firstHalfLen);
        (ppu->*f)(va + MemoryPage::pageSize, (char*)buf + firstHalfLen, len - firstHalfLen);
        return true;
    }
    return false;
}

MemoryPage::MemoryPage() : ptr(new uint8_t[pageSize]) {
    memset(ptr.get(), 0, pageSize);
}

void PPU::writeMemory(uint64_t va, void* buf, uint len) {
    assert(va != 0 || len == 0);
    uint64_t pageOffset;
    if (mergeAcrossPages(this, &PPU::writeMemory, va, buf, len, pageOffset))
        return;
    
    // OPT: effective STL 24 if needed
    uint64_t vaPage = va - pageOffset;
    auto it = _pages.find(vaPage);
    if (it == end(_pages)) {
        _pages[vaPage] = MemoryPage();
    }
    memcpy(_pages[vaPage].ptr.get() + pageOffset, buf, len);
}

void PPU::setMemory(uint64_t va, uint8_t value, uint len) {
    // OPT:
    std::unique_ptr<uint8_t> buf(new uint8_t[len]);
    memset(buf.get(), value, len);
    writeMemory(va, buf.get(), len);
}

struct IForm {
    uint8_t opcode : 6;
    uint8_t _ : 2;
};

void PPU::run() {
}

void PPU::readMemory(uint64_t va, void* buf, uint len) {
    uint64_t pageOffset;
    if (mergeAcrossPages(this, &PPU::readMemory, va, buf, len, pageOffset))
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
