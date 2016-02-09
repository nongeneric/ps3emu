#include "gcm.h"
#include "graphics.h"
#include "../sys.h"
#include "../../ps3emu/constants.h"
#include "../../ps3emu/Process.h"
#include "../../ps3emu/rsx/Rsx.h"
#include <algorithm>
#include <boost/log/trivial.hpp>

using namespace boost::endian;

namespace emu {
namespace Gcm {

struct IOMapping {
    ps3_uintptr_t address;
    uint32_t size;
    uint32_t offset;
};

struct {
    Process* proc;
    MainMemory* mm;
    Rsx* rsx;
    std::vector<IOMapping> ioMaps;
    TargetCellGcmContextData* context = nullptr;
    ps3_uintptr_t gCellGcmCurrentContext = 0;
    uint32_t defaultCommandBufferSize = 0;
} emuGcmState;

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
    config->ioSize = emuGcmState.ioMaps.at(0).size;
    config->localAddress = RsxFbBaseAddr;
    config->ioAddress = emuGcmState.ioMaps.at(0).address;
    return emu_void;
}

int32_t cellGcmAddressToOffset(uint32_t address, boost::endian::big_uint32_t* offset) {
    for (auto& m : emuGcmState.ioMaps) {
        if (m.address <= address && address < m.address + m.size) {
            *offset = address - m.address + m.offset;
            return CELL_OK;
        }
    }
    *offset = address - RsxFbBaseAddr;
    return CELL_OK;
}

void setCurrentCommandBuffer(MainMemory* mm, ps3_uintptr_t va) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    mm->store<4>(emuGcmState.gCellGcmCurrentContext, va);
}

uint32_t _cellGcmInitBody(ps3_uintptr_t defaultGcmContextSymbolVa, uint32_t cmdSize, uint32_t ioSize, ps3_uintptr_t ioAddress, Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    
    proc->rsx()->setGcmContext(ioSize, ioAddress);
    proc->rsx()->init(proc->mm());
    proc->mm()->setMemory(GcmLabelBaseOffset, 0, 1, true);
    proc->mm()->setMemory(DefaultGcmDefaultContextData, 0, 16, true);
    
    emuGcmState.context = (TargetCellGcmContextData*)
        proc->mm()->getMemoryPointer(DefaultGcmDefaultContextData, sizeof(TargetCellGcmContextData));
    emuGcmState.defaultCommandBufferSize = cmdSize;
    emuGcmState.gCellGcmCurrentContext = defaultGcmContextSymbolVa;
    setCurrentCommandBuffer(proc->mm(), DefaultGcmDefaultContextData);
    emuGcmState.proc = proc;
    emuGcmState.mm = proc->mm();
    emuGcmState.rsx = proc->rsx();
    emuGcmState.ioMaps.push_back({ ioAddress, ioSize, 0 });
    proc->mm()->writeMemory(ioAddress, gcmResetCommands, gcmResetCommandsSize);
    proc->mm()->writeMemory(ioAddress + gcmResetCommandsSize, gcmInitCommands, gcmInitCommandsSize);
    emuGcmState.context->begin = ioAddress + gcmResetCommandsSize;
    emuGcmState.context->end = emuGcmState.context->begin + 0x1bff * 4;
    emuGcmState.context->current = ioAddress + gcmResetCommandsSize + gcmInitCommandsSize;
    emuGcmState.context->callback = DefaultGcmBufferOverflowCallback;
    uint32_t callbackNCallIndex;
    auto entry = findNCallEntry(calcFnid("defaultContextCallback"), callbackNCallIndex);
    assert(entry);
    (void)entry;
    
    // gCellGcmCurrentContext -> context -> callback-fun-descr -> callback-code
    proc->mm()->store<4>(DefaultGcmBufferOverflowCallback, DefaultGcmBufferOverflowCallback + 4);
    encodeNCall(proc->mm(), DefaultGcmBufferOverflowCallback + 4, callbackNCallIndex);
    
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

int32_t cellGcmIoOffsetToAddress(uint32_t offset, big_uint32_t* address) {
    for (auto& m : emuGcmState.ioMaps) {
        if (m.offset <= offset && offset < m.offset + m.size) {
            *address = offset - m.offset + m.address;
            return CELL_OK;
        }
    }
    *address = RsxFbBaseAddr + offset;
    return CELL_OK;
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

int32_t cellGcmMapMainMemory(ps3_uintptr_t address, uint32_t size, big_uint32_t* offset, Process* proc) {
    auto& maps = emuGcmState.ioMaps;
    auto lastOffsetMapping = std::max_element(begin(maps), end(maps), [](auto& a, auto& b) {
        return a.offset + a.size < b.offset + b.size;
    });
    assert(lastOffsetMapping != end(maps));
    auto firstFreeOffset = lastOffsetMapping->offset + lastOffsetMapping->size;
    maps.push_back({address, size, firstFreeOffset});
    *offset = firstFreeOffset;
    proc->mm()->map(address, firstFreeOffset + RsxFbBaseAddr, size);
    return CELL_OK;
}

emu_void_t cellGcmSetFlipHandler(ps3_uintptr_t handler) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    // TODO: implement
    return emu_void;
}

emu_void_t cellGcmSetDefaultCommandBuffer(Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    setCurrentCommandBuffer(proc->mm(), DefaultGcmDefaultContextData);
    return emu_void;
}

uint32_t defaultContextCallback(TargetCellGcmContextData* data, uint32_t count) {
    uint32_t k32 = 32 * 1024;
    assert(emuGcmState.defaultCommandBufferSize % k32 == 0);
    auto ioBase = emuGcmState.ioMaps.at(0).address;
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

}}

