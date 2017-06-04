#include "MainMemory.h"

#include "libs/spu/sysSpu.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "state.h"
#include "rsx/Rsx.h"
#include "log.h"
#include "utils.h"
#include <stdlib.h>

inline bool coversRsxRegsRange(ps3_uintptr_t va, uint len) {
    return !(va + len <= GcmControlRegisters || va >= GcmControlRegisters + 12);
}

bool MainMemory::writeSpecialMemory(ps3_uintptr_t va, const void* buf, uint len) {
    if ((va & SpuThreadBaseAddr) == SpuThreadBaseAddr) {
        writeSpuThreadVa(va, buf, len);
        return true;
    }
    if ((va & RawSpuBaseAddr) == RawSpuBaseAddr) {
        writeRawSpuVa(va, buf, len);
        return true;
    }
    if (coversRsxRegsRange(va, len)) {
        if (!g_state.rsx) {
            throw std::runtime_error("rsx not set");
        }
        if (len == 4) {
            uint32_t val = *(boost::endian::big_uint32_t*)buf;
            if (va == GcmControlRegisters) {
                g_state.rsx->setPut(val);
            } else if (va == GcmControlRegisters + 4) {
                g_state.rsx->setGet(val);
            } else {
                throw std::runtime_error("only put and get rsx registers are supported");
            }
        } else {
            throw std::runtime_error("only one rsx register can be set at a time");
        }
        return true;
    }
    return false;
}

bool MainMemory::readSpecialMemory(ps3_uintptr_t va, void* buf, uint len) {
    if ((va & RawSpuBaseAddr) == RawSpuBaseAddr) {
        assert(len == 4);
        *(big_uint32_t*)buf = readRawSpuVa(va);
        return true;
    }
    
    auto GcmControlRegistersSegment = 0x40000000u;
    
    if ((va & GcmControlRegistersSegment) && coversRsxRegsRange(va, len)) {
        assert(len == 4);
        if (va == GcmControlRegisters) {
            *(big_uint32_t*)buf = g_state.rsx->getPut();
            return true;
        } else if (va == GcmControlRegisters + 4) {
            *(big_uint32_t*)buf = g_state.rsx->getGet();
            return true;
        } else if (va == GcmControlRegisters + 8) {
            *(big_uint32_t*)buf = g_state.rsx->getRef();
            return true;
        }
        throw std::runtime_error("unknown rsx register");
    }
    return false;
}

#define DEFINE_LOAD_X(x) \
    uint##x##_t MainMemory::load##x(ps3_uintptr_t va, bool validate) { \
        uint##x##_t special; \
        if (readSpecialMemory(va, &special, x / 8)) \
            return endian_reverse(special); \
        if (validate) \
            this->validate(va, x / 8, false); \
        VirtualAddress split { va }; \
        auto& page = _pages[split.page.u()]; \
        auto offset = split.offset.u(); \
        auto ptr = page.ptr + offset; \
        return endian_reverse(*(uint##x##_t*)ptr); \
    }

DEFINE_LOAD_X(8)
DEFINE_LOAD_X(16)
DEFINE_LOAD_X(32)
DEFINE_LOAD_X(64)
DEFINE_LOAD_X(128)

#define DEFINE_STORE_X(x) \
    void MainMemory::store##x(ps3_uintptr_t va, uint##x##_t value) { \
        auto reversed = endian_reverse(value); \
        if (writeSpecialMemory(va, &reversed, x / 8)) \
            return; \
        validate(va, x / 8, false); \
        VirtualAddress split { va }; \
        auto pageIndex = split.page.u(); \
        auto& page = _pages[pageIndex]; \
        auto offset = split.offset.u(); \
        auto ptr = page.ptr.load(); \
        ptr = ptr + offset; \
        auto line = _rmap.lock<x / 8>(va); \
        *(uint##x##_t*)ptr = reversed; \
        _rmap.destroyExcept(line, g_state.granule); \
        _rmap.unlock(line); \
        _mmap.mark<x / 8>(va); \
    }

DEFINE_STORE_X(8)
DEFINE_STORE_X(16)
DEFINE_STORE_X(32)
DEFINE_STORE_X(64)
DEFINE_STORE_X(128)

void MainMemory::writeMemory(ps3_uintptr_t va, const void* buf, uint len) {
    if (writeSpecialMemory(va, buf, len))
        return;
    validate(va, len, true);
    auto last = va + len;
    auto src = (const char*)buf;
    _mmap.mark(va, len);
    while (va != last) {
        auto next = std::min(last, (va + 128) & 0xffffff80);
        auto line = _rmap.lock<128>(va & 0xffffff80);
        auto size = next - va;
        auto dest = getMemoryPointer(va, size);
        memcpy(dest, src, size);
        _rmap.destroyExcept(line, g_state.granule);
        _rmap.unlock(line);
        src += size;
        va = next;
    }
}

MainMemory::~MainMemory() {
    dealloc();
}

void MainMemory::setMemory(ps3_uintptr_t va, uint8_t value, uint len) {
    std::unique_ptr<uint8_t[]> buf(new uint8_t[len]);
    memset(buf.get(), value, len);
    writeMemory(va, buf.get(), len);
}

void MainMemory::readMemory(ps3_uintptr_t va, void* buf, uint len, bool validate) {
    if (readSpecialMemory(va, &buf, len))
        return;
    auto last = va + len;
    auto dest = (char*)buf;
    while (va != last) {
        auto next = std::min(last, (va + 128) & 0xffffff80);
        auto size = next - va;
        auto src = getMemoryPointer(va, size);
        memcpy(dest, src, size);
        dest += size;
        va = next;
    }
}

struct IForm {
    uint8_t opcode : 6;
    uint8_t _ : 2;
};

bool MainMemory::isAllocated(ps3_uintptr_t va) {
    return true;
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

void MainMemory::dealloc() {
    if (_pages) {
        for (auto i = 0u; i != DefaultMainMemoryPageCount; ++i) {
            if (!_providedMemoryPages.test(i)) {
                _pages[i].dealloc();
            }
        }
    }
}

void MainMemory::reset() {
    dealloc();
    _pages.reset(new MemoryPage[DefaultMainMemoryPageCount]);
    auto realloc = [&] (uint32_t i) {
        auto page = (i << 28) / DefaultMainMemoryPageSize;
        auto count = 0x10000000u / DefaultMainMemoryPageSize;
        for (auto i = page; i < page + count; ++i) {
            _pages[i].alloc();
        }
    };
    realloc(0);
    realloc(1);
    realloc(3);
    realloc(4);
    realloc(7);
    realloc(0xd);
    
    mark(GcmLabelBaseOffset, 0x100000, false, "gcm labels");
    mark(RsxFbBaseAddr, 0x10000000, false, "rsx local region");
}

void MemoryPage::alloc() {
    auto mem = (uint8_t*)aligned_alloc(2, DefaultMainMemoryPageSize);
    memset(mem, 0, DefaultMainMemoryPageSize);
    uintptr_t e = 0;
    if (!ptr.compare_exchange_strong(e, (uintptr_t)mem))
        free(mem);
    assert((ptr & 1) == 0);
}

void MemoryPage::dealloc() {
    auto mem = ptr.load();
    if (mem) {
        free((void*)mem);
    }
}

uint8_t* MainMemory::getMemoryPointer(ps3_uintptr_t va, uint32_t len) {
    VirtualAddress split { va };
    auto& page = _pages[split.page.u()];
    if (!page.ptr)
        throw std::runtime_error("getting memory pointer for not allocated memory");
    auto offset = split.offset.u();
    // TODO: check that the range is continuous
    return (uint8_t*)page.ptr.load() + offset;
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
            memcpy(memoryPtr, (void*)page.ptr.load(), DefaultMainMemoryPageSize);
        }
        page.ptr = (uintptr_t)memoryPtr;
        _providedMemoryPages.set(firstPage + i);
    }
}

uint32_t encodeNCall(MainMemory* mm, ps3_uintptr_t va, uint32_t index) {
    uint32_t ncall = (1 << 26) | index;
    if (va) {
        mm->store32(va, ncall);
    }
    return ncall;
}

MainMemory::MainMemory() {
    reset();
}

void splitRawSpuAddress(uint32_t va, uint32_t& id, uint32_t& offset) {
    offset = va & 0x3ffff;
    id = ((va - offset - RawSpuBaseAddr) & ~RawSpuProblemOffset) / RawSpuOffset;
}

void MainMemory::writeRawSpuVa(ps3_uintptr_t va, const void* src, uint32_t len) {
    uint32_t spuNum, offset;
    splitRawSpuAddress(va, spuNum, offset);
    auto th = findRawSpuThread(spuNum);
    if ((va & RawSpuProblemOffset) == RawSpuProblemOffset) {
        assert(len == 4);
        auto val = *(big_uint32_t*)src;
        th->channels()->mmio_write(offset, val);
        return;
    }
    memcpy(th->ptr(offset), src, len);
}

uint32_t MainMemory::readRawSpuVa(ps3_uintptr_t va) {
    uint32_t spuNum, offset;
    splitRawSpuAddress(va, spuNum, offset);
    auto th = findRawSpuThread(spuNum);
    if ((va & RawSpuProblemOffset) == RawSpuProblemOffset) {
        return th->channels()->mmio_read(offset);
    }
    return *(big_uint32_t*)th->ptr(offset);
}

void splitSpuThreadAddress(uint32_t va, uint32_t& spuNum, uint32_t& offset) {
    va &= ~SpuThreadBaseAddr;
    spuNum = va / SpuThreadOffsetAddr;
    offset = va & 0x5ffff;
}

void MainMemory::writeSpuThreadVa(ps3_uintptr_t va, const void* val, uint32_t len) {
    uint32_t spu, offset;
    splitSpuThreadAddress(va, spu, offset);
    auto th = g_state.proc->getSpuThreadBySpuNum(spu);
    if (offset == SpuThreadSnr1) {
        th->channels()->mmio_write(SPU_Sig_Notify_1, *(big_uint32_t*)val);
    } else if (offset == SpuThreadSnr1) {
        th->channels()->mmio_write(SPU_Sig_Notify_2, *(big_uint32_t*)val);
    } else {
        memcpy(th->ptr(offset), val, len);
    }
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

#ifdef MEMORY_PROTECTION

auto pageSize = 1u << 10;

void iterate(uint32_t start,
             uint32_t len,
             std::function<void(uint32_t, uint32_t, uint32_t)> action) {
    for (auto i = start; i <= start + len;) {
        auto page = (i / pageSize) * pageSize;
        auto nextPage = page + pageSize;
        if (nextPage - page == pageSize) {
            action(page, 0, pageSize);
        } else {
            bool isFirstPage = i == start;
            auto rangeStart = isFirstPage ? i - start : 0;
            auto rangeEnd = (start + len) % pageSize;
            action(page, rangeStart, rangeEnd);
        }
        i = nextPage;
    }
}

void MainMemory::reportViolation(uint32_t ea, uint32_t len, bool write) {
    auto range = std::find_if(begin(protectionRanges), end(protectionRanges), [&](auto& range) {
        return intersects(range.start, range.len, ea, len);
    });
    auto rangeMessage = range == end(protectionRanges)
                            ? "no range"
                            : ssnprintf("range %08x-%08x %s %s",
                                        range->start,
                                        range->start + range->len,
                                        range->readonly ? "R" : "RW",
                                        range->comment);
    auto message = ssnprintf("memory %s violation at %08x size %x (%s)",
                             write ? "write" : "read",
                             ea,
                             len,
                             rangeMessage);
    ERROR(libs) << message;
    throw std::runtime_error("memory access violation");
}

void MainMemory::mark(uint32_t ea, uint32_t len, bool readonly, std::string comment) {
    protectionRanges.push_back({ea, len, readonly, comment});
    iterate(ea, len, [&] (auto page, auto start, auto end) {
        auto pageIndex = page / pageSize;
        if (start == 0 && end == pageSize) {
            readMap[pageIndex] = true;
            writeMap[pageIndex] = !readonly;
        } else {
            auto& readRange = readInfos[pageIndex].subrange;
            auto& writeRange = writeInfos[pageIndex].subrange;
            for (; start != end; ++start) {
                readRange[start] = true;
                writeRange[start] = !readonly;
            }
        }
    });
}

void MainMemory::unmark(uint32_t ea, uint32_t len) {
    
}

void MainMemory::dbgMemoryBreakpoint(uint32_t ea, int32_t len, bool write) {
    _memoryBreakpoints.push_back({ea, (uint32_t)len, write});
}

void MainMemory::validate(uint32_t ea, uint32_t len, bool write) {
    auto mb = std::find_if(begin(_memoryBreakpoints), end(_memoryBreakpoints), [&](auto& m) {
        return m.write == write && ::intersects(ea, len, m.ea, m.len);
    });
    if (mb != end(_memoryBreakpoints)) {
        _memoryBreakpoints.erase(mb);
        throw BreakpointException();
    }
    auto& map = write ? writeMap : readMap;
    auto& infos = write ? writeInfos : readInfos;
    iterate(ea, len, [&] (auto page, auto start, auto end) {
        auto index = page / pageSize;
        if (map.test(index))
            return;
        auto info = infos.find(index);
        if (info == std::end(infos)) {
            this->reportViolation(ea, len, write);
            return;
        }
        for (; start != end; ++start) {
            if (!info->second.subrange.test(start)) {
                this->reportViolation(ea, len, write);
                return;
            }
        }
    });
}

ProtectionRange MainMemory::addressRange(uint32_t ea) {
    for (auto& range : protectionRanges) {
        if (intersects<uint32_t>(range.start, range.len, ea, 1))
            return range;
    }
    return {0};
}

#endif
