#include "gcm.h"
#include "graphics.h"
#include "../../ps3emu/constants.h"
#include "../../ps3emu/PPU.h"
#include <boost/log/trivial.hpp>

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
    uint32_t currentContextOffset;
    std::vector<IOMapping> ioMaps;
    TargetCellGcmContextData context;
    void writeContextToMemory() {
        ppu->writeMemory(GcmLocalMemoryBase + currentContextOffset, &context, sizeof(context));
    }
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
        emuGcmState.context.current += (uintptr_t)_hostContext.current - (uintptr_t)_hostContext.begin;
        emuGcmState.writeContextToMemory();
        emuGcmState.rsx->setPut(emuGcmState.context.current);
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
    emuGcmState.ppu = ppu;
    emuGcmState.rsx = ppu->getRsx();
    emuGcmState.ppu->map(ioAddress, GcmLocalMemoryBase, ioSize);
    emuGcmState.ioMaps.push_back({ ioAddress, ioSize, 0 });
    ppu->writeMemory(ioAddress, gcmResetCommands, gcmResetCommandsSize);
    ppu->writeMemory(ioAddress + gcmResetCommandsSize, gcmInitCommands, gcmInitCommandsSize);
    emuGcmState.context.begin = ioAddress + gcmResetCommandsSize;
    emuGcmState.context.end = ioAddress + 0x6ffc;
    emuGcmState.context.current = ioAddress + gcmInitCommandsSize;
    emuGcmState.context.callback = 0;
    CellGcmControl regs;
    regs.get = emuGcmState.context.begin - ioAddress;
    regs.put = emuGcmState.context.current - ioAddress;
    regs.ref = 0xffffffff;
    emuGcmState.rsx->setRegs(&regs);
    window.Init();
    return CELL_OK;  
}

}}

