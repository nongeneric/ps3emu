#pragma once

#include "../sys_defs.h"
#include "../../ps3emu/constants.h"
#include "sysutil_sysparam.h"
#include <boost/endian/arithmetic.hpp>

#define GCM_EMU
#include <gcm_tool.h>

class Process;

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

enum class MemoryLocation {
    Main, Local
};

namespace emu {
namespace Gcm {
    
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
emu_void_t cellGcmResetFlipStatus(Process* proc);
emu_void_t _cellGcmSetFlipCommand(Process* proc, uint32_t buffer);
uint32_t cellGcmGetTiledPitchSize(uint32_t size);
int32_t cellGcmSetTileInfo(uint8_t index,
                           uint8_t location,
                           uint32_t offset,
                           uint32_t size,
                           uint32_t pitch,
                           uint8_t comp,
                           uint16_t base,
                           uint8_t bank);
uint32_t _cellGcmSetFlipWithWaitLabel(uint8_t id, uint8_t labelindex, uint32_t labelvalue, Process* proc);
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
emu_void_t cellGcmSetFlipHandler(ps3_uintptr_t handler);
emu_void_t cellGcmSetDefaultCommandBuffer(Process* proc);
emu_void_t cellGcmSetGraphicsHandler(uint32_t handler);

int32_t cellGcmAddressToOffset(uint32_t address, boost::endian::big_uint32_t *offset);
int32_t cellGcmIoOffsetToAddress(uint32_t offset, boost::endian::big_uint32_t *address);
uint32_t cellGcmGetMaxIoMapSize();
int32_t cellGcmGetOffsetTable(CellGcmOffsetTable* table);
int32_t cellGcmMapEaIoAddress(uint32_t ea, uint32_t io, uint32_t size);
int32_t cellGcmMapEaIoAddressWithFlags(uint32_t ea, uint32_t io, uint32_t size, uint32_t userflags);
int32_t cellGcmMapMainMemory(uint32_t address, uint32_t size, boost::endian::big_uint32_t *offset);
int32_t cellGcmUnmapEaIoAddress(uint32_t ea);
int32_t cellGcmUnmapIoAddress(uint32_t io);

emu_void_t cellGcmSetVBlankHandler(uint32_t handler, Process* proc);

}}

ps3_uintptr_t rsxOffsetToEa(MemoryLocation location, ps3_uintptr_t offset);