#include "gcm.h"
#include "graphics.h"
#include "../../ps3emu/LocalMemory.h"
#include "../../ps3emu/constants.h"
#include "../../ps3emu/PPU.h"

Window window;

using namespace boost::endian;

namespace emu {
namespace Gcm {

CellGcmContextData gCellGcmContextData;

struct {
    PPU* ppu;
    Rsx* rsx;
    LocalMemory* localMemory;
    uint32_t currentContextOffset;
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
        auto currentContext = emuGcmState.rsx->getCurrentContext(emuGcmState.currentContextOffset);
        currentContext.current += _hostContext.current - _hostContext.begin;
        emuGcmState.rsx->updateCurrentContext(&currentContext, emuGcmState.currentContextOffset);
    }
    CellGcmContextData* getContext() {
        return &_hostContext;
    }
};

void cellGcmSetFlipMode(uint32_t mode) {
    assert(mode == CELL_GCM_DISPLAY_VSYNC);
}

void cellGcmGetConfiguration(CellGcmConfig* config) {
    config->memoryFrequency = RsxMemoryFrequency;
    config->coreFrequency = RsxCoreFrequency;
    config->localSize = GcmLocalMemorySize;
    config->ioSize = RsxIoAddressSpaceSize;
    config->localAddress = GcmLocalMemoryBase;
    config->ioAddress = RsxIoAddressSpace;
}

int32_t cellGcmAddressToOffset(uint32_t address, boost::endian::big_uint32_t* offset) {
    *offset = address - GcmLocalMemoryBase; // add io mapping
    return CELL_OK;
}

void cellGcmSetSurface(const CellGcmSurface* surface) {
    TempGcmContext context;
    ::cellGcmSetSurfaceUnsafeInline(context.getContext(), surface);
}

uint32_t _cellGcmInitBody(ps3_uintptr_t callGcmCallback, uint32_t cmdSize, uint32_t ioSize, ps3_uintptr_t ioAddress, PPU* ppu) {
    emuGcmState.ppu = ppu;
    emuGcmState.rsx = ppu->getRsx();
    emuGcmState.localMemory = emuGcmState.rsx->localMemory();
    emuGcmState.localMemory->mapIO(0, ioAddress, ioSize);
    gCellGcmContextData.begin = (uint32_t*)(uintptr_t)ioAddress;
    gCellGcmContextData.end = (uint32_t*)(uintptr_t)ioAddress;
    gCellGcmContextData.current = (uint32_t*)(uintptr_t)ioAddress;
    gCellGcmContextData.callback = 0;
    window.Init();
    return CELL_OK;  
}

}}

