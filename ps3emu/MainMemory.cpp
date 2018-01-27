#include "MainMemory.h"

#include "libs/spu/sysSpu.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "state.h"
#include "rsx/Rsx.h"
#include "log.h"
#include "utils.h"
#include <stdlib.h>

template<int Len>
inline typename IntTraits<Len>::Type MainMemory::load(ps3_uintptr_t va, bool validate) {
    if constexpr(Len == 4) {
        uint32_t special;
        if (readSpecialMemory(va, &special))
            return fast_endian_reverse(special);
    }
    if (validate)
        this->validate(va, Len, false);
    VirtualAddress split { va };
    auto& page = _pages[split.page.u()];
    auto offset = split.offset.u();
    auto ptr = page.ptr + offset;
    auto typedPtr = (typename IntTraits<Len>::Type*)ptr;
    return fast_endian_reverse(*typedPtr);
}

template <int Len>
inline void MainMemory::store(ps3_uintptr_t va,
                              typename IntTraits<Len>::Type value,
                              ReservationGranule* granule)
{
    auto reversed = fast_endian_reverse(value);
    if (unlikely(writeSpecialMemory(va, &reversed, Len)))
        return;
    validate(va, Len, false);
    VirtualAddress split { va };
    auto pageIndex = split.page.u();
    auto& page = _pages[pageIndex];
    auto offset = split.offset.u();
    auto ptr = page.ptr.load();
    ptr = ptr + offset;
    ReservationLine *line, *nextLine;
    _rmap.lock<Len>(va, &line, &nextLine);
    auto typedPtr = (typename IntTraits<Len>::Type*)ptr;
    auto reversedValue = fast_endian_reverse(value);
    *typedPtr = reversedValue;
    _rmap.destroyExcept(line, nextLine, granule);
    _rmap.unlock(line, nextLine);
    _mmap.mark<Len>(va);
}

void MainMemory::writeMemory(ps3_uintptr_t va,
                             const void* buf,
                             uint len)
{
    if (writeSpecialMemory(va, buf, len))
        return;
    validate(va, len, true);
    auto last = va + len;
    auto src = (const char*)buf;
    _mmap.mark(va, len);
    while (va != last) {
        auto next = std::min(last, (va + 128) & 0xffffff80);
        auto size = next - va;
        auto dest = getMemoryPointer(va, size);
        ReservationLine *line, *nextLine;
        _rmap.lock<128>(va & 0xffffff80, &line, &nextLine);
        memcpy(dest, src, size);
        _rmap.destroyExcept(line, nextLine, g_state.granule);
        _rmap.unlock(line, nextLine);
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
    if (len == 4 && readSpecialMemory(va, &buf))
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
    assert(page.ptr);
    auto offset = split.offset.u();
    // TODO: check that the range is continuous
    return (uint8_t*)page.ptr.load() + offset;
}

void MainMemory::provideMemory(ps3_uintptr_t src, uint32_t size, void* memory) {
    VirtualAddress split { src };
    if (split.offset.u() != 0 || size % DefaultMainMemoryPageSize != 0) {
        ERROR(libs) << "expecting multiple of page size";
        exit(1);
    }
    auto pageCount = size / DefaultMainMemoryPageSize;
    auto firstPage = split.page.u();
    assert(firstPage < DefaultMainMemoryPageCount);
    for (auto i = 0u; i < pageCount; ++i) {
        auto& page = _pages[firstPage + i];
        auto memoryPtr = (uint8_t*)memory + i * DefaultMainMemoryPageSize;
        auto ptr = page.ptr.load();
        if (ptr) {
            memcpy(memoryPtr, (void*)ptr, DefaultMainMemoryPageSize);
        }
        page.ptr = (uintptr_t)memoryPtr;
        _providedMemoryPages.set(firstPage + i);
    }
}

uint32_t encodeNCall(MainMemory* mm, ps3_uintptr_t va, uint32_t index) {
    assert(index < 0x3ffffff);
    uint32_t ncall = (NCALL_OPCODE << 26) | index;
    if (va) {
        mm->store32(va, ncall, g_state.granule);
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

void MainMemory::store8(ps3_uintptr_t va, uint8_t value, ReservationGranule* granule) {
    store<1>(va, value, granule);
}

void MainMemory::store16(ps3_uintptr_t va, uint16_t value, ReservationGranule* granule) {
    store<2>(va, value, granule);
}

void MainMemory::store32(ps3_uintptr_t va, uint32_t value, ReservationGranule* granule) {
    store<4>(va, value, granule);
}

void MainMemory::store64(ps3_uintptr_t va, uint64_t value, ReservationGranule* granule) {
    store<8>(va, value, granule);
}

void MainMemory::store128(ps3_uintptr_t va, uint128_t value, ReservationGranule* granule) {
    store<16>(va, value, granule);
}

uint8_t MainMemory::load8(ps3_uintptr_t va, bool validate) {
    return load<1>(va, validate);
}

uint16_t MainMemory::load16(ps3_uintptr_t va, bool validate) {
    return load<2>(va, validate);
}

uint32_t MainMemory::load32(ps3_uintptr_t va, bool validate) {
    return load<4>(va, validate);
}

uint64_t MainMemory::load64(ps3_uintptr_t va, bool validate) {
    return load<8>(va, validate);
}

uint128_t MainMemory::load128(ps3_uintptr_t va, bool validate) {
    return load<16>(va, validate);
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
