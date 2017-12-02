#pragma once

#include "../sys_defs.h"
#include "../../constants.h"
#include "sysutil_sysparam.h"
#include <boost/endian/arithmetic.hpp>
#include <vector>
#include "ps3emu/enum.h"

class Process;
class MainMemory;

extern int gcmResetCommandsSize;
extern int gcmInitCommandsSize;
extern uint8_t gcmResetCommands[];
extern uint8_t gcmInitCommands[];

struct TargetCellGcmContextData {
    boost::endian::big_uint32_t begin;
    boost::endian::big_uint32_t end;
    boost::endian::big_uint32_t current;
    boost::endian::big_uint32_t callback;
};

ENUM(MemoryLocation,
    (Main, 0),
    (Local, 1)
)

namespace emu {
namespace Gcm {

struct CellGcmConfig {
    boost::endian::big_uint32_t localAddress;
    boost::endian::big_uint32_t ioAddress;
    boost::endian::big_uint32_t localSize;
    boost::endian::big_uint32_t ioSize;
    boost::endian::big_uint32_t memoryFrequency;
    boost::endian::big_uint32_t coreFrequency;
};
    
struct CellGcmOffsetTable {
    boost::endian::big_uint32_t ioAddress;
    boost::endian::big_uint32_t eaAddress;
};
    
uint32_t _cellGcmInitBody(ps3_uintptr_t defaultGcmContextSymbolVa, uint32_t cmdSize, uint32_t ioSize, ps3_uintptr_t ioAddress, Process* proc);
uint32_t defaultContextCallback(TargetCellGcmContextData* data, uint32_t count);

emu_void_t cellGcmSetFlipMode(uint32_t mode);
emu_void_t cellGcmGetConfiguration(CellGcmConfig *config);
int32_t cellGcmSetDisplayBuffer(const uint8_t id,
                                const uint32_t offset,
                                const uint32_t pitch,
                                const uint32_t width,
                                const uint32_t height,
                                Process* proc);
ps3_uintptr_t cellGcmGetControlRegister();
ps3_uintptr_t cellGcmGetLabelAddress(uint8_t index);
uint32_t cellGcmGetFlipStatus(Process* proc);
emu_void_t cellGcmSetFlipStatus(Process* proc);
emu_void_t cellGcmResetFlipStatus(Process* proc);
uint32_t _cellGcmSetFlipCommand(uint32_t context, uint32_t buffer);
uint32_t cellGcmGetTiledPitchSize(uint32_t size);
int32_t cellGcmSetTileInfo(uint8_t index,
                           uint8_t location,
                           uint32_t offset,
                           uint32_t size,
                           uint32_t pitch,
                           uint8_t comp,
                           uint16_t base,
                           uint8_t bank);
uint32_t _cellGcmSetFlipCommandWithWaitLabel(uint32_t context, uint8_t buffer, uint8_t labelindex, uint32_t labelvalue);
int32_t cellGcmBindTile(uint8_t index);
int32_t cellGcmUnbindTile(uint8_t index);
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
                         uint32_t sMask);
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
                           uint32_t sMask);
int32_t cellGcmUnbindZcull(uint8_t index);

emu_void_t cellGcmSetFlipHandler(ps3_uintptr_t handler);
emu_void_t cellGcmSetDefaultCommandBuffer(Process* proc);
emu_void_t cellGcmSetGraphicsHandler(uint32_t handler);

int32_t cellGcmAddressToOffset(uint32_t address, boost::endian::big_uint32_t *offset);
int32_t cellGcmIoOffsetToAddress(uint32_t offset, boost::endian::big_uint32_t *address);
uint32_t cellGcmGetMaxIoMapSize();
int32_t cellGcmGetOffsetTable(CellGcmOffsetTable* table);
int32_t cellGcmMapEaIoAddress(uint32_t ea, uint32_t io, uint32_t size, Process* proc);
int32_t cellGcmMapEaIoAddressWithFlags(uint32_t ea, uint32_t io, uint32_t size, uint32_t userflags, Process* proc);
int32_t cellGcmMapMainMemory(uint32_t address, uint32_t size, boost::endian::big_uint32_t *offset, Process* proc);
int32_t cellGcmUnmapEaIoAddress(uint32_t ea);
int32_t cellGcmUnmapIoAddress(uint32_t io);
uint32_t cellGcmGetReportDataLocation(uint32_t index, uint32_t location, MainMemory* mm);
uint32_t cellGcmGetReportDataAddressLocation(uint32_t index, uint32_t location);
int32_t cellGcmInitDefaultFifoMode(int32_t mode);
uint32_t cellGcmGetTileInfo();
uint32_t cellGcmGetZcullInfo();
int64_t cellGcmGetLastFlipTime();
uint64_t cellGcmGetTimeStamp(uint32_t index);
uint32_t cellGcmGetReport(uint32_t type, uint32_t index);
emu_void_t cellGcmSetSecondVFrequency(uint32_t freq);
emu_void_t cellGcmSetUserHandler(uint32_t handler);
emu_void_t cellGcmSetVBlankHandler(uint32_t handler, Process* proc);
uint32_t cellGcmGetDefaultCommandWordSize();
uint32_t cellGcmGetDefaultSegmentWordSize();

}}

ps3_uintptr_t rsxOffsetToEa(MemoryLocation location, ps3_uintptr_t offset);
std::vector<uint16_t> serializeOffsetTable();
void deserializeOffsetTable(std::vector<uint16_t> const& vec);
