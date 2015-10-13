#include "gcm.h"
#include "graphics.h"
#include "../../ps3emu/constants.h"
#include "../../ps3emu/PPU.h"
#include <boost/log/trivial.hpp>
#include "../../ps3emu/ELFLoader.h"

Window window;

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
} emuGcmState;

class TempGcmContext {
    CellGcmContextData _hostContext;
    uint32_t buffer[40];
public:
    TempGcmContext() {
        _hostContext.begin = buffer;
        _hostContext.end = buffer + sizeof(buffer) / sizeof(uint32_t) - 1;
        _hostContext.current = buffer;
    }
    ~TempGcmContext() {
        emuGcmState.context->current += (uintptr_t)_hostContext.current - (uintptr_t)_hostContext.begin;
        emuGcmState.rsx->setPut(emuGcmState.context->current);
    }
    CellGcmContextData* getContext() {
        return &_hostContext;
    }
};

emu_void_t cellGcmSetFlipMode(uint32_t mode) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    assert(mode == CELL_GCM_DISPLAY_VSYNC);
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

emu_void_t cellGcmSetSurface(const CellGcmSurface* surface) {
    TempGcmContext context;
    ::cellGcmSetSurfaceUnsafeInline(context.getContext(), surface);
    return emu_void;
}

uint32_t _cellGcmInitBody(ps3_uintptr_t callGcmCallback, uint32_t cmdSize, uint32_t ioSize, ps3_uintptr_t ioAddress, PPU* ppu) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    
    ppu->setMemory(GcmLabelBaseOffset, 0, 1, true);
    ppu->setMemory(GcmLocalMemoryBase, 0, GcmLocalMemorySize, true);
    
    ps3_uintptr_t pageVa;
    void* pagePtr;
    ppu->allocPage(&pagePtr, &pageVa);
    emuGcmState.context = (TargetCellGcmContextData*)pagePtr;
    auto contextVa = ppu->getELFLoader()->getSymbolValue("gCellGcmCurrentContext");
    ppu->store<4>(contextVa, pageVa);
    emuGcmState.ppu = ppu;
    emuGcmState.rsx = ppu->getRsx();
    emuGcmState.ppu->map(ioAddress, GcmLocalMemoryBase, ioSize);
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
    window.Init();
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

emu_void_t _cellGcmSetFlipCommand(PPU* ppu) {
    return emu_void;
}

}}
