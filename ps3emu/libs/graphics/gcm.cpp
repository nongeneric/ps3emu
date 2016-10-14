#include "gcm.h"
#include "graphics.h"
#include "../sys.h"
#include "ps3emu/constants.h"
#include "ps3emu/Process.h"
#include "ps3emu/rsx/Rsx.h"
#include "ps3emu/rsx/RsxContext.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <algorithm>
#include <array>
#include <cstddef>

using namespace boost::endian;

namespace emu {
namespace Gcm {

struct CellGcmTileInfo {
    big_uint32_t tile;
    big_uint32_t limit;
    big_uint32_t pitch;
    big_uint32_t format;
};

struct CellGcmZcullInfo {
    big_uint32_t region;
    big_uint32_t size;
    big_uint32_t start;
    big_uint32_t offset;
    big_uint32_t status0;
    big_uint32_t status1;
};
  
struct OffsetTable;
struct {
    TargetCellGcmContextData* defaultContext = nullptr;
    uint32_t defaultContextDataEa;
    ps3_uintptr_t gCellGcmCurrentContext = 0;
    uint32_t defaultCommandBufferSize = 0;
    OffsetTable* offsetTable = nullptr;
    ps3_uintptr_t offsetTableEmuEa;
    uint32_t ioSize;
    std::array<CellGcmTileInfo, 16>* tileInfos = nullptr;
    ps3_uintptr_t tileInfosOffset = 0;
    std::array<CellGcmZcullInfo, 8>* zcullInfos = nullptr;
    ps3_uintptr_t zcullInfosOffset = 0;
} emuGcmState;

struct OffsetTable {
    boost::mutex _m;
    
    OffsetTable() {
        for (auto& i : ioAddress) {
            i = 0xffff;
        }
        
        for (auto& i : eaAddress) {
            i = 0xffff;
        }
        
        for (auto& i : mapPageCount) {
            i = 0;
        }
    }
    
    std::array<big_uint16_t, 0xbff> ioAddress;
    std::array<big_uint16_t, 511> eaAddress;
    std::array<uint16_t, 511> mapPageCount;
    
    uint32_t eaToOffset(uint32_t ea) {
        boost::lock_guard<boost::mutex> lock(_m);
        auto ioIdx = ioAddress[ea >> 20];
        if (ioIdx == 0xffff)
            throw std::exception();
        return ((uint32_t)ioAddress[ea >> 20] << 20) | ((ea << 12) >> 12);
    }
    
    uint32_t offsetToEa(uint32_t offset) {
        boost::lock_guard<boost::mutex> lock(_m);
        assert(eaAddress[offset >> 20] != 0xffff);
        return ((uint32_t)eaAddress[offset >> 20] << 20) | ((offset << 12) >> 12);
    }
    
    void unmapEa(uint32_t ea) {
        boost::lock_guard<boost::mutex> lock(_m);
        ea >>= 20;
        auto io = ioAddress[ea];
        assert(io != 0xffff);
        auto& count = mapPageCount[io];
        assert(count);
        for (auto i = 0u; i < count; ++i) {
            ioAddress[ea + i] = 0xffff;
            eaAddress[io + i] = 0xffff;
        }
        count = 0;
        g_state.rsx->updateOffsetTableForReplay();
    }
    
    void unmapOffset(uint32_t offset) {
        unmapEa(offsetToEa(offset));
    }
    
    void map(uint32_t ea, uint32_t io, uint32_t size) {
        INFO(rsx) << ssnprintf("mapping ea %08x to io %08x of size %08x", ea, io, size);
        boost::lock_guard<boost::mutex> lock(_m);
        auto pages = size / DefaultMainMemoryPageSize;
        auto ioIndex = io >> 20;
        auto eaIndex = ea >> 20;
        mapPageCount[ioIndex] = pages;
        for (auto i = 0u; i < pages; ++i) {
            ioAddress[eaIndex] = ioIndex;
            eaAddress[ioIndex] = eaIndex;
            ioIndex++;
            eaIndex++;
        }
        g_state.rsx->updateOffsetTableForReplay();
    }
};

emu_void_t cellGcmSetFlipMode(uint32_t mode) {
    INFO(libs) << __FUNCTION__;
    //assert(mode == CELL_GCM_DISPLAY_VSYNC);
    return emu_void;
}

emu_void_t cellGcmGetConfiguration(CellGcmConfig* config) {
    INFO(libs) << __FUNCTION__;
    config->memoryFrequency = RsxMemoryFrequency;
    config->coreFrequency = RsxCoreFrequency;
    config->localSize = GcmLocalMemorySize;
    config->ioSize = emuGcmState.ioSize; // TODO: compute exactly
    config->localAddress = RsxFbBaseAddr;
    config->ioAddress = emuGcmState.offsetTable->offsetToEa(0);
    return emu_void;
}

void setCurrentCommandBuffer(MainMemory* mm, ps3_uintptr_t va) {
    INFO(libs) << __FUNCTION__;
    mm->store<4>(emuGcmState.gCellGcmCurrentContext, va);
}

uint32_t _cellGcmInitBody(ps3_uintptr_t defaultGcmContextSymbolVa,
                          uint32_t cmdSize,
                          uint32_t ioSize,
                          ps3_uintptr_t ioAddress,
                          Process* proc) {
    INFO(libs) << __FUNCTION__;
    
    emuGcmState.offsetTable = g_state.memalloc->internalAlloc<128, OffsetTable>(&emuGcmState.offsetTableEmuEa);
    emuGcmState.ioSize = ioSize;
    
    emuGcmState.tileInfos =
        g_state.memalloc->internalAlloc<128, std::array<CellGcmTileInfo, 16>>(
            &emuGcmState.tileInfosOffset);
        
    emuGcmState.zcullInfos =
        g_state.memalloc->internalAlloc<128, std::array<CellGcmZcullInfo, 8>>(
            &emuGcmState.zcullInfosOffset);
    
    g_state.rsx->setGcmContext(ioSize, ioAddress);
    g_state.rsx->init(proc);
    g_state.mm->setMemory(GcmLabelBaseOffset, 0, 1, true);
    
    emuGcmState.defaultContext =
        g_state.memalloc->internalAlloc<128, TargetCellGcmContextData>(
            &emuGcmState.defaultContextDataEa);
    
    emuGcmState.defaultCommandBufferSize = cmdSize;
    emuGcmState.gCellGcmCurrentContext = defaultGcmContextSymbolVa;
    setCurrentCommandBuffer(g_state.mm, emuGcmState.defaultContextDataEa);
    
    emuGcmState.offsetTable->map(ioAddress, 0, ioSize);
    g_state.mm->provideMemory(ioAddress, ioSize, g_state.rsx->context()->mainMemoryBuffer.mapped());
    g_state.mm->writeMemory(ioAddress, gcmResetCommands, gcmResetCommandsSize);
    g_state.mm->writeMemory(ioAddress + gcmResetCommandsSize, gcmInitCommands, gcmInitCommandsSize);
    emuGcmState.defaultContext->begin = ioAddress + gcmResetCommandsSize;
    emuGcmState.defaultContext->end = emuGcmState.defaultContext->begin + 0x1bff * 4;
    emuGcmState.defaultContext->current = ioAddress + gcmResetCommandsSize + gcmInitCommandsSize;
    
    uint32_t callbackDescrEa;
    auto callbackDescr = g_state.memalloc->internalAlloc<128, fdescr>(&callbackDescrEa);
    emuGcmState.defaultContext->callback = callbackDescrEa;
    
    uint32_t callbackNCallIndex;
    auto entry = findNCallEntry(calcFnid("defaultContextCallback"), callbackNCallIndex);
    assert(entry);
    (void)entry;
    
    // gCellGcmCurrentContext -> context -> callback-fun-descr -> callback-code
    uint32_t callbackNcallInstrEa;
    g_state.memalloc->internalAlloc<128, uint32_t>(&callbackNcallInstrEa);
    encodeNCall(g_state.mm, callbackNcallInstrEa, callbackNCallIndex);
    
    callbackDescr->va = callbackNcallInstrEa;
    callbackDescr->tocBase = 0;

    g_state.rsx->setGet(emuGcmState.defaultContext->begin - ioAddress);
    g_state.rsx->setPut(emuGcmState.defaultContext->current - ioAddress);
    auto rsxPrimaryDmaLabel = 3;
    g_state.rsx->setLabel(rsxPrimaryDmaLabel, 1, false);
    return CELL_OK;  
}

uint32_t cellGcmGetTileInfo() {
    assert(emuGcmState.tileInfosOffset);
    return emuGcmState.tileInfosOffset;
}

int32_t cellGcmSetDisplayBuffer(const uint8_t id,
                                const uint32_t offset,
                                const uint32_t pitch,
                                const uint32_t width,
                                const uint32_t height,
                                Process* proc)
{
    proc->rsx()->setDisplayBuffer(id, offset, pitch, width, height);
    return CELL_OK;
}

ps3_uintptr_t cellGcmGetControlRegister() {
    return GcmControlRegisters;
}

ps3_uintptr_t cellGcmGetLabelAddress(uint8_t index) {
    return GcmLabelBaseOffset + index * 0x10;
}

uint32_t cellGcmGetFlipStatus(Process* proc) {
    return proc->rsx()->isFlipInProgress() ?
        CELL_GCM_DISPLAY_FLIP_STATUS_WAITING :
        CELL_GCM_DISPLAY_FLIP_STATUS_DONE;
}

emu_void_t cellGcmSetFlipStatus(Process* proc) {
    proc->rsx()->setFlipStatus();
    return emu_void;
}

emu_void_t cellGcmResetFlipStatus(Process* proc) {
    proc->rsx()->resetFlipStatus(); 
    return emu_void;
}

void setFlipCommand(uint32_t contextEa, uint32_t label, uint32_t labelValue, uint32_t buffer) {
    auto context = (TargetCellGcmContextData*)g_state.mm->getMemoryPointer(
        contextEa, sizeof(TargetCellGcmContextData));
    if (context->end - context->current <= 16) {
        ERROR(rsx) << "not enough space for EmuFlip in the buffer";
        exit(1);
    }
    uint32_t header = (3 << CELL_GCM_COUNT_SHIFT) | EmuFlipCommandMethod;
    g_state.mm->store<4>(context->current, header);
    g_state.mm->store<4>(context->current + 4, buffer);
    g_state.mm->store<4>(context->current + 8, label);
    g_state.mm->store<4>(context->current + 12, labelValue);
    context->current += 4 * sizeof(uint32_t);
}

emu_void_t _cellGcmSetFlipCommand(uint32_t context, uint32_t buffer) {
    setFlipCommand(context, -1, 0, buffer);
    return emu_void;
}

uint32_t cellGcmGetTiledPitchSize(uint32_t size) {
    INFO(libs) << __FUNCTION__;
    return size;
}

int32_t cellGcmSetTileInfo(uint8_t index,
                           uint8_t location,
                           uint32_t offset,
                           uint32_t size, 
                           uint32_t pitch,
                           uint8_t comp,
                           uint16_t base,
                           uint8_t bank)
{
    INFO(libs) << __FUNCTION__;
    auto& info = (*emuGcmState.tileInfos)[index];
    info.tile = (location + 1) | (bank << 4) | ((offset / 0x10000) << 16) | (location << 31);
    info.limit = (((offset + size - 1) / 0x10000) << 16) | (location << 31);
    info.pitch = (pitch / 0x100) << 8;
    info.format = (base | (base + ((size - 1) / 0x10000)) << 13) | (comp << 26) | (1 << 30);
    return CELL_OK;
}

uint32_t _cellGcmSetFlipWithWaitLabel(uint8_t id, uint8_t labelindex, uint32_t labelvalue) {
    assert(false);
    INFO(libs) << __FUNCTION__;
    setFlipCommand(0, labelindex, labelvalue, id);
    return CELL_OK;
}

int32_t cellGcmBindTile(uint8_t index) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

int32_t cellGcmUnbindTile(uint8_t index) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

int32_t cellGcmBindZcull(uint8_t index, 
                         uint32_t offset, 
                         uint32_t width, 
                         uint32_t height, 
                         uint32_t cullStart, 
                         uint32_t zFormat, 
                         uint32_t aaFormat,
                         uint32_t zCullDir, 
                         uint32_t zCullFormat, 
                         uint32_t sFunc, 
                         uint32_t sRef, 
                         uint32_t sMask) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

int32_t cellGcmUnbindZcull(uint8_t index) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

emu_void_t cellGcmSetFlipHandler(ps3_uintptr_t handler) {
    INFO(libs) << __FUNCTION__;
    g_state.rsx->setFlipHandler(handler);
    return emu_void;
}

emu_void_t cellGcmSetDefaultCommandBuffer(Process* proc) {
    INFO(libs) << __FUNCTION__;
    setCurrentCommandBuffer(g_state.mm, emuGcmState.defaultContextDataEa);
    return emu_void;
}

uint32_t defaultContextCallback(TargetCellGcmContextData* data, uint32_t count) {
    uint32_t k32 = 32 * 1024;
    //assert(emuGcmState.defaultCommandBufferSize % k32 == 0);
    auto ioBase = emuGcmState.offsetTable->offsetToEa(0);
    uint32_t nextBuffer = data->end + 4;
    uint32_t nextSize;
    if (nextBuffer - ioBase >= emuGcmState.defaultCommandBufferSize) {
        nextSize = k32 - gcmResetCommandsSize;
        nextBuffer = ioBase + gcmResetCommandsSize;
    } else {
        nextBuffer = data->end + 4;
        nextSize = k32;
    }
    
    g_state.rsx->setPut(data->current - ioBase);
    while (data->current - ioBase != g_state.rsx->getGet()) {
        sys_timer_usleep(20);
    }
    
    g_state.rsx->encodeJump(data->current, nextBuffer - ioBase);
    
    INFO(libs) << ssnprintf("defaultContextCallback(nextSize = %x, nextBuffer = %x, "
                            "jump = %x, dest = %x, defsize = %x, count = %x)",
                            nextSize,
                            nextBuffer - ioBase,
                            data->current - ioBase,
                            nextBuffer - ioBase,
                            emuGcmState.defaultCommandBufferSize,
                            count);
    
    data->begin = nextBuffer;
    data->current = nextBuffer;
    data->end = nextBuffer + nextSize - 4;
    assert(data->end - ioBase < emuGcmState.defaultCommandBufferSize);
    return CELL_OK;
}

emu_void_t cellGcmSetGraphicsHandler(uint32_t handler) {
    // assume no gcm errors are ever encountered
    return emu_void;
}

int32_t cellGcmIoOffsetToAddress(uint32_t offset, big_uint32_t* address) {
    *address = emuGcmState.offsetTable->offsetToEa(offset);
    return CELL_OK;
}

int32_t cellGcmAddressToOffset(uint32_t address, boost::endian::big_uint32_t* offset) {
    if (address & RsxFbBaseAddr) {
        *offset = address & ~RsxFbBaseAddr;
    } else {
        try {
            *offset = emuGcmState.offsetTable->eaToOffset(address);
        } catch (...) {
            return CELL_GCM_ERROR_FAILURE;
        }
    }
    return CELL_OK;
}

uint32_t cellGcmGetMaxIoMapSize() {
    return 0x10000000;
}

int32_t cellGcmGetOffsetTable(CellGcmOffsetTable* table) {
    table->ioAddress = emuGcmState.offsetTableEmuEa + offsetof(OffsetTable, ioAddress);
    table->eaAddress = emuGcmState.offsetTableEmuEa + offsetof(OffsetTable, eaAddress);
    return CELL_OK;
}

int32_t cellGcmMapEaIoAddress(uint32_t ea, uint32_t io, uint32_t size, Process* proc) {
    if ((io & 0xfffff) || (size & 0xfffff)) {
        return CELL_GCM_ERROR_FAILURE;
    }
    emuGcmState.offsetTable->map(ea, io, size);
    g_state.mm->provideMemory(ea, size, (uint8_t*)proc->rsx()->context()->mainMemoryBuffer.mapped() + io);
    return CELL_OK;
}

int32_t cellGcmMapEaIoAddressWithFlags(uint32_t ea, uint32_t io, uint32_t size, uint32_t, Process* proc) {
    return cellGcmMapEaIoAddress(ea, io, size, proc);
}

int32_t cellGcmMapMainMemory(uint32_t address, uint32_t size, boost::endian::big_uint32_t* offset, Process* proc) {
    auto& eas = emuGcmState.offsetTable->eaAddress;
    auto ioGap = findGap<big_uint16_t>(begin(eas),
                                       end(eas),
                                       size / DefaultMainMemoryPageSize,
                                       [](big_uint16_t x) { return x == 0xffff; });
    auto io = (uint32_t)std::distance(begin(eas), ioGap) << 20;
    *offset = io;
    return cellGcmMapEaIoAddress(address, io, size, proc);
}

int32_t cellGcmUnmapEaIoAddress(uint32_t ea) {
    assert(!(ea & 0xfffff));
    emuGcmState.offsetTable->unmapEa(ea);
    return CELL_OK;
}

int32_t cellGcmUnmapIoAddress(uint32_t io) {
    assert(!(io & 0xfffff));
    emuGcmState.offsetTable->unmapOffset(io);
    return CELL_OK;
}

emu_void_t cellGcmSetVBlankHandler(uint32_t handler, Process* proc) {
    proc->rsx()->setVBlankHandler(handler);
    return emu_void;
}

uint32_t cellGcmGetReportDataLocation(uint32_t index, uint32_t location, MainMemory* mm) {
    const auto valueOffset = 8;
    auto ea = cellGcmGetReportDataAddressLocation(index, location);
    return mm->load<4>(ea + valueOffset);
}

uint32_t cellGcmGetReportDataAddressLocation(uint32_t index, uint32_t location) {
    return getReportDataAddressLocation(index, gcmEnumToLocation(location));
}

emu_void_t cellGcmSetZcull(uint8_t index,
                           uint32_t offset,
                           uint32_t width,
                           uint32_t height,
                           uint32_t cullStart,
                           uint32_t zFormat,
                           uint32_t aaFormat,
                           uint32_t zCullDir,
                           uint32_t zCullFormat,
                           uint32_t sFunc,
                           uint32_t sRef,
                           uint32_t sMask) {
    auto& info = (*emuGcmState.zcullInfos)[index];
    info.region = (1 << 0) | (zFormat << 4) | (aaFormat << 8);
    info.size = ((width >> 6) << 22) | ((height >> 6) << 6);
    info.start = cullStart & ~0xFFF;
    info.offset = offset;
    info.status0 = (zCullDir << 1) | (zCullFormat << 2) 
                 | ((sFunc & 0xF) << 12) | (sRef << 16) | (sMask << 24);
    info.status1 = 0x2000 | (0x20 << 16);
    return emu_void;
}

uint32_t cellGcmGetZcullInfo() {
    assert(emuGcmState.zcullInfosOffset);
    return emuGcmState.zcullInfosOffset;
}

int32_t cellGcmInitDefaultFifoMode(int32_t mode) {
    INFO(libs) << "NOT IMPLEMENTED: cellGcmInitDefaultFifoMode";
    return CELL_OK;
}

uint32_t cellGcmGetLastFlipTime(Process* proc) {
    return proc->rsx()->getLastFlipTime();
}

}}

ps3_uintptr_t rsxOffsetToEa(MemoryLocation location, ps3_uintptr_t offset) {
    if (location == MemoryLocation::Local)
        return RsxFbBaseAddr + offset;
    return emu::Gcm::emuGcmState.offsetTable->offsetToEa(offset);
}

std::vector<uint16_t> serializeOffsetTable() {
    auto table = emu::Gcm::emuGcmState.offsetTable;
    std::vector<uint16_t> res = { 0, 0, 0 };
    uint16_t ioAddressCount = 0, eaAddressCount = 0, pageCount = 0;
    for (auto i = 0u; i < table->ioAddress.size(); ++i) {
        if (table->ioAddress[i] != 0xffff) {
            res.push_back(i);
            res.push_back(table->ioAddress[i]);
            ioAddressCount++;
        }
    }
    
    for (auto i = 0u; i < table->eaAddress.size(); ++i) {
        if (table->eaAddress[i] != 0xffff) {
            res.push_back(i);
            res.push_back(table->eaAddress[i]);
            eaAddressCount++;
        }
    }
    
    for (auto i = 0u; i < table->mapPageCount.size(); ++i) {
        if (table->mapPageCount[i]) {
            res.push_back(i);
            res.push_back(table->mapPageCount[i]);
            pageCount++;
        }
    }
    
    res[0] = ioAddressCount;
    res[1] = eaAddressCount;
    res[2] = pageCount;
    
    return res;
}

void deserializeOffsetTable(std::vector<uint16_t> const& vec) {
    if (!emu::Gcm::emuGcmState.offsetTable) {
        emu::Gcm::emuGcmState.offsetTable = new emu::Gcm::OffsetTable();
    }
    auto table = emu::Gcm::emuGcmState.offsetTable;
    std::fill(begin(table->ioAddress), end(table->ioAddress), 0xffff);
    std::fill(begin(table->eaAddress), end(table->eaAddress), 0xffff);
    std::fill(begin(table->mapPageCount), end(table->mapPageCount), 0);
    
    unsigned pos = 3;
    for (auto i = 0u; i < vec[0]; ++i) {
        table->ioAddress[vec[pos]] = vec[pos + 1];
        pos += 2;
    }
    for (auto i = 0u; i < vec[1]; ++i) {
        table->eaAddress[vec[pos]] = vec[pos + 1];
        pos += 2;
    }
    for (auto i = 0u; i < vec[2]; ++i) {
        table->mapPageCount[vec[pos]] = vec[pos + 1];
        pos += 2;
    }
}
