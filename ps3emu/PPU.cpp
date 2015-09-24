#include "PPU.h"
#include <string.h>
#include <functional>
#include <stdio.h>
#include <assert.h>

using namespace boost::endian;

bool coversRsxRegsRange(ps3_uintptr_t va, uint len) {
    return va <= GcmControlRegisters && va + len >= GcmControlRegisters;
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
        if (va + len < GcmControlRegisters + sizeof(emu::CellGcmControl))
            throw std::runtime_error("incomplete rsx regs update not implemented");
        auto regs = (emu::CellGcmControl*)((char*)buf + (GcmControlRegisters - va));
        if (_rsx) {
            _rsx->setRegs(regs);
        }
    }
    
    assert(va != 0 || len == 0);
    if (mergeAcrossPages(this, &PPU::writeMemory, va, const_cast<void*>(buf), len, allocate))
        return;
    
    VirtualAddress split { va };
    auto& page = _pages[split.page.u()];
    if (!page.ptr && !allocate)
        throw std::runtime_error("accessing non existing page");
    if (!page.ptr)
        page.alloc();
    memcpy(page.ptr + split.offset.u(), buf, len);
}

void PPU::setMemory(ps3_uintptr_t va, uint8_t value, uint len, bool allocate) {
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

void PPU::readMemory(ps3_uintptr_t va, void* buf, uint len, bool allocate) {
    if (coversRsxRegsRange(va, len)) {
        throw std::runtime_error("reading rsx registers not implemented");
    }
    
    if (mergeAcrossPages(this, &PPU::readMemory, va, buf, len, allocate))
        return;
    
    VirtualAddress split { va };
    auto& page = _pages[split.page.u()];
    if (!page.ptr)
        throw std::runtime_error("accessing non existing page");
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
            if (_pages[i].ptr) {
                delete _pages[i].ptr;
            }
        }
    }
    _pages.reset(new MemoryPage[DefaultMainMemoryPageCount]);
    memset(_GPR, 0xcc, 8 * 32);
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
    ptr = new uint8_t[DefaultMainMemoryPageSize];
    memset(ptr, 0, DefaultMainMemoryPageSize);
}

void PPU::shutdown() {
    if (_rsx) {
        _rsx->shutdown();
    }
}
