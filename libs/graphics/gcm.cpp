#include "gcm.h"
#include "graphics.h"
#include "../sys.h"
#include "../../ps3emu/constants.h"
#include "../../ps3emu/Process.h"
#include "../../ps3emu/rsx/Rsx.h"
#include "../../ps3emu/ELFLoader.h"
#include "../../ps3emu/InternalMemoryManager.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <boost/log/trivial.hpp>

using namespace boost::endian;

namespace emu {
namespace Gcm {

struct OffsetTable;
struct {
    Process* proc;
    MainMemory* mm;
    Rsx* rsx;
    TargetCellGcmContextData* context = nullptr;
    uint32_t defaultContextDataEa;
    ps3_uintptr_t gCellGcmCurrentContext = 0;
    uint32_t defaultCommandBufferSize = 0;
    OffsetTable* offsetTable = nullptr;
    ps3_uintptr_t offsetTableEmuEa;
    uint32_t ioSize;
} emuGcmState;

  
struct OffsetTable {
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
        auto ioIdx = ioAddress[ea >> 20];
        if (ioIdx == 0xffff)
            throw std::exception();
        return ((uint32_t)ioAddress[ea >> 20] << 20) | ((ea << 12) >> 12);
    }
    
    uint32_t offsetToEa(uint32_t offset) {
        assert(eaAddress[offset >> 20] != 0xffff);
        return ((uint32_t)eaAddress[offset >> 20] << 20) | ((offset << 12) >> 12);
    }
    
    void unmapEa(uint32_t ea) {
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
        emuGcmState.rsx->updateOffsetTableForReplay();
    }
    
    void unmapOffset(uint32_t offset) {
        unmapEa(offsetToEa(offset));
    }
    
    void map(uint32_t ea, uint32_t io, uint32_t size) {
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
        emuGcmState.rsx->updateOffsetTableForReplay();
    }
};

emu_void_t cellGcmSetFlipMode(uint32_t mode) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    //assert(mode == CELL_GCM_DISPLAY_VSYNC);
    return emu_void;
}

emu_void_t cellGcmGetConfiguration(CellGcmConfig* config) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    config->memoryFrequency = RsxMemoryFrequency;
    config->coreFrequency = RsxCoreFrequency;
    config->localSize = GcmLocalMemorySize;
    config->ioSize = emuGcmState.ioSize; // TODO: compute exactly
    config->localAddress = RsxFbBaseAddr;
    config->ioAddress = emuGcmState.offsetTable->offsetToEa(0);
    return emu_void;
}

void setCurrentCommandBuffer(MainMemory* mm, ps3_uintptr_t va) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    mm->store<4>(emuGcmState.gCellGcmCurrentContext, va);
}

uint32_t _cellGcmInitBody(ps3_uintptr_t defaultGcmContextSymbolVa,
                            uint32_t cmdSize,
                            uint32_t ioSize,
                            ps3_uintptr_t ioAddress,
                            Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    
    auto internalMemory = proc->internalMemoryManager();
    emuGcmState.offsetTable = internalMemory->internalAlloc<128, OffsetTable>(&emuGcmState.offsetTableEmuEa);
    emuGcmState.ioSize = ioSize;
    
    proc->rsx()->setGcmContext(ioSize, ioAddress);
    proc->rsx()->init(proc);
    proc->mm()->setMemory(GcmLabelBaseOffset, 0, 1, true);
    
    emuGcmState.context =
        internalMemory->internalAlloc<128, TargetCellGcmContextData>(
            &emuGcmState.defaultContextDataEa);
    
    emuGcmState.defaultCommandBufferSize = cmdSize;
    emuGcmState.gCellGcmCurrentContext = defaultGcmContextSymbolVa;
    setCurrentCommandBuffer(proc->mm(), emuGcmState.defaultContextDataEa);
    emuGcmState.proc = proc;
    emuGcmState.mm = proc->mm();
    emuGcmState.rsx = proc->rsx();
    
    emuGcmState.offsetTable->map(ioAddress, 0, ioSize);
    proc->mm()->writeMemory(ioAddress, gcmResetCommands, gcmResetCommandsSize);
    proc->mm()->writeMemory(ioAddress + gcmResetCommandsSize, gcmInitCommands, gcmInitCommandsSize);
    emuGcmState.context->begin = ioAddress + gcmResetCommandsSize;
    emuGcmState.context->end = emuGcmState.context->begin + 0x1bff * 4;
    emuGcmState.context->current = ioAddress + gcmResetCommandsSize + gcmInitCommandsSize;
    
    uint32_t callbackDescrEa;
    auto callbackDescr = internalMemory->internalAlloc<128, fdescr>(&callbackDescrEa);
    emuGcmState.context->callback = callbackDescrEa;
    
    uint32_t callbackNCallIndex;
    auto entry = findNCallEntry(calcFnid("defaultContextCallback"), callbackNCallIndex);
    assert(entry);
    (void)entry;
    
    // gCellGcmCurrentContext -> context -> callback-fun-descr -> callback-code
    uint32_t callbackNcallInstrEa;
    internalMemory->internalAlloc<128, uint32_t>(&callbackNcallInstrEa);
    encodeNCall(proc->mm(), callbackNcallInstrEa, callbackNCallIndex);
    
    callbackDescr->va = callbackNcallInstrEa;
    callbackDescr->tocBase = 0;
    
    emuGcmState.rsx->setGet(emuGcmState.context->begin - ioAddress);
    emuGcmState.rsx->setPut(emuGcmState.context->current - ioAddress);
    auto rsxPrimaryDmaLabel = 3;
    emuGcmState.rsx->setLabel(rsxPrimaryDmaLabel, 1);
    return CELL_OK;  
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

emu_void_t cellGcmResetFlipStatus(Process* proc) {
    proc->rsx()->resetFlipStatus(); 
    return emu_void;
}

void setFlipCommand(Process* proc, uint32_t label, uint32_t labelValue, uint32_t buffer) {
    assert(emuGcmState.context->end - emuGcmState.context->current >= 4);
    uint32_t header = (3 << CELL_GCM_COUNT_SHIFT) | EmuFlipCommandMethod;
    emuGcmState.mm->store<4>(emuGcmState.context->current, header);
    emuGcmState.mm->store<4>(emuGcmState.context->current + 4, buffer);
    emuGcmState.mm->store<4>(emuGcmState.context->current + 8, label);
    emuGcmState.mm->store<4>(emuGcmState.context->current + 12, labelValue);
    emuGcmState.context->current += 4 * sizeof(uint32_t);
}

emu_void_t _cellGcmSetFlipCommand(Process* proc, uint32_t buffer) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    setFlipCommand(proc, -1, 0, buffer);
    return emu_void;
}

uint32_t cellGcmGetTiledPitchSize(uint32_t size) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
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
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

uint32_t _cellGcmSetFlipWithWaitLabel(uint8_t id, uint8_t labelindex, uint32_t labelvalue, Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    setFlipCommand(proc, labelindex, labelvalue, id);
    return CELL_OK;
}

int32_t cellGcmBindTile(uint8_t index) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

int32_t cellGcmUnbindTile(uint8_t index) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
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
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

emu_void_t cellGcmSetFlipHandler(ps3_uintptr_t handler) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    // TODO: implement
    return emu_void;
}

emu_void_t cellGcmSetDefaultCommandBuffer(Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    setCurrentCommandBuffer(proc->mm(), emuGcmState.defaultContextDataEa);
    return emu_void;
}

uint32_t defaultContextCallback(TargetCellGcmContextData* data, uint32_t count) {
    uint32_t k32 = 32 * 1024;
    assert(emuGcmState.defaultCommandBufferSize % k32 == 0);
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
    
    emuGcmState.rsx->setPut(data->current - ioBase);
    while ((emuGcmState.rsx->getGet() > nextBuffer - ioBase &&
           emuGcmState.rsx->getGet() < nextBuffer - ioBase + nextSize) ||
           emuGcmState.rsx->isCallActive()) {
        sys_timer_usleep(20);
    }
    
    emuGcmState.rsx->encodeJump(data->current, nextBuffer - ioBase);
    
    BOOST_LOG_TRIVIAL(trace) << 
        ssnprintf("defaultContextCallback(nextSize = %x, nextBuffer = %x, jump = %x, dest = %x, defsize = %x)",
            nextSize, nextBuffer - ioBase, data->current - ioBase, nextBuffer - ioBase, 
            emuGcmState.defaultCommandBufferSize
        );
    
    data->begin = nextBuffer;
    data->current = nextBuffer;
    data->end = nextBuffer + nextSize - 4;
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

int32_t cellGcmMapEaIoAddress(uint32_t ea, uint32_t io, uint32_t size) {
    if ((io & 0xfffff) || (size & 0xfffff)) {
        return CELL_GCM_ERROR_FAILURE;
    }
    emuGcmState.offsetTable->map(ea, io, size);
    return CELL_OK;
}

int32_t cellGcmMapEaIoAddressWithFlags(uint32_t ea, uint32_t io, uint32_t size, uint32_t) {
    return cellGcmMapEaIoAddress(ea, io, size);
}

int32_t cellGcmMapMainMemory(uint32_t address, uint32_t size, boost::endian::big_uint32_t* offset) {
    auto& eas = emuGcmState.offsetTable->eaAddress;
    auto ioGap = findGap<big_uint16_t>(begin(eas),
                                       end(eas),
                                       size / DefaultMainMemoryPageSize,
                                       [](big_uint16_t x) { return x == 0xffff; });
    auto io = (uint32_t)std::distance(begin(eas), ioGap) << 20;
    *offset = io;
    return cellGcmMapEaIoAddress(address, io, size);
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
    auto locationEnum = gcmEnumToLocation(location);
    if (locationEnum == MemoryLocation::Local) {
        assert(index < 2048);
    } else {
        assert(index < 1024 * 1024);
    }
    const auto valueOffset = 8;
    auto offset = 0x0e000000 + 16 * index + valueOffset;
    auto ea = rsxOffsetToEa(locationEnum, offset);
    return mm->load<4>(ea);
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