#include "gcm.h"
#include "graphics.h"
#include "../../ps3emu/constants.h"
#include "../../ps3emu/PPU.h"
#include <boost/log/trivial.hpp>
#include "../../ps3emu/ELFLoader.h"
#include <algorithm>

using namespace boost::endian;

namespace emu {
namespace Gcm {

struct IOMapping {
    ps3_uintptr_t address;
    uint32_t size;
    uint32_t offset;
};

struct {
    PPU* ppu;
    Rsx* rsx;
    std::vector<IOMapping> ioMaps;
    TargetCellGcmContextData* context = nullptr;
    ps3_uintptr_t defaultCommandBuffer = 0;
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
    config->localAddress = GcmLocalMemoryBase;
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
    *offset = address - GcmLocalMemoryBase;
    return CELL_OK;
}

void setCurrentCommandBuffer(PPU* ppu, ps3_uintptr_t va) {
    auto contextVa = ppu->getELFLoader()->getSymbolValue("gCellGcmCurrentContext");
    ppu->store<4>(contextVa, va);
}

uint32_t _cellGcmInitBody(ps3_uintptr_t callGcmCallback, uint32_t cmdSize, uint32_t ioSize, ps3_uintptr_t ioAddress, PPU* ppu) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    
    ppu->getRsx()->setGcmContext(ioSize, ioAddress);
    ppu->getRsx()->init();
    ppu->setMemory(GcmLabelBaseOffset, 0, 1, true);
    
    ps3_uintptr_t pageVa;
    void* pagePtr;
    ppu->allocPage(&pagePtr, &pageVa);
    emuGcmState.context = (TargetCellGcmContextData*)pagePtr;
    emuGcmState.defaultCommandBuffer = pageVa;
    setCurrentCommandBuffer(ppu, pageVa);
    emuGcmState.ppu = ppu;
    emuGcmState.rsx = ppu->getRsx();
    emuGcmState.ioMaps.push_back({ ioAddress, ioSize, 0 });
    ppu->writeMemory(ioAddress, gcmResetCommands, gcmResetCommandsSize);
    ppu->writeMemory(ioAddress + gcmResetCommandsSize, gcmInitCommands, gcmInitCommandsSize);
    emuGcmState.context->begin = ioAddress + gcmResetCommandsSize;
    emuGcmState.context->end = emuGcmState.context->begin + 0x1bff * 4;
    emuGcmState.context->current = ioAddress + gcmResetCommandsSize + gcmInitCommandsSize;
    emuGcmState.context->callback = 0;
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
                                const uint32_t height)
{
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    assert(pitch == 5120);
    assert(width == 1280);
    assert(height == 720);
    return CELL_OK;
}

ps3_uintptr_t cellGcmGetControlRegister() {
    return GcmControlRegisters;
}

ps3_uintptr_t cellGcmGetLabelAddress(uint8_t index) {
    return GcmLabelBaseOffset + index * 0x10;
}

uint32_t cellGcmGetFlipStatus(PPU* ppu) {
    return ppu->getRsx()->isFlipInProgress() ?
        CELL_GCM_DISPLAY_FLIP_STATUS_WAITING :
        CELL_GCM_DISPLAY_FLIP_STATUS_DONE;
}

emu_void_t cellGcmResetFlipStatus(PPU* ppu) {
    ppu->getRsx()->resetFlipStatus(); 
    return emu_void;
}

void setFlipCommand(PPU* ppu, uint32_t label, uint32_t labelValue, uint32_t buffer) {
    assert(emuGcmState.context->end - emuGcmState.context->current >= 4);
    uint32_t header = (3 << CELL_GCM_COUNT_SHIFT) | EmuFlipCommandMethod;
    emuGcmState.ppu->store<4>(emuGcmState.context->current, header);
    emuGcmState.ppu->store<4>(emuGcmState.context->current + 4, buffer);
    emuGcmState.ppu->store<4>(emuGcmState.context->current + 8, label);
    emuGcmState.ppu->store<4>(emuGcmState.context->current + 12, labelValue);
    emuGcmState.context->current += 4 * sizeof(uint32_t);
}

emu_void_t _cellGcmSetFlipCommand(PPU* ppu, uint32_t buffer) {
    setFlipCommand(ppu, -1, 0, buffer);
    return emu_void;
}

int32_t cellGcmIoOffsetToAddress(uint32_t offset, big_uint32_t* address) {
    for (auto& m : emuGcmState.ioMaps) {
        if (m.offset <= offset && offset < m.offset + m.size) {
            *address = offset - m.offset + m.address;
            return CELL_OK;
        }
    }
    *address = GcmLocalMemoryBase + offset;
    return CELL_OK;
}

uint32_t cellGcmGetTiledPitchSize(uint32_t size) {
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
    return CELL_OK;
}

uint32_t _cellGcmSetFlipWithWaitLabel(uint8_t id, uint8_t labelindex, uint32_t labelvalue, PPU* ppu) {
    setFlipCommand(ppu, labelindex, labelvalue, id);
    return CELL_OK;
}

int32_t cellGcmBindTile(uint8_t index) {
    return CELL_OK;
}

int32_t cellGcmUnbindTile(uint8_t index) {
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
    return CELL_OK;
}

int32_t cellGcmMapMainMemory(ps3_uintptr_t address, uint32_t size, big_uint32_t* offset, PPU* ppu) {
    auto& maps = emuGcmState.ioMaps;
    auto lastOffsetMapping = std::max_element(begin(maps), end(maps), [](auto& a, auto& b) {
        return a.offset + a.size < b.offset + b.size;
    });
    assert(lastOffsetMapping != end(maps));
    auto firstFreeOffset = lastOffsetMapping->offset + lastOffsetMapping->size;
    maps.push_back({address, size, firstFreeOffset});
    *offset = GcmLocalMemoryBase + firstFreeOffset;
    ppu->map(address, *offset, size);
    return CELL_OK;
}

emu_void_t cellGcmSetFlipHandler(ps3_uintptr_t handler) {
    // TODO: implement
    return emu_void;
}

emu_void_t cellGcmSetDefaultCommandBuffer(PPU* ppu) {
    setCurrentCommandBuffer(ppu, emuGcmState.defaultCommandBuffer);
    return emu_void;
}

}}

