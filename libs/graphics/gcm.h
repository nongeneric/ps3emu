#pragma once

#include "../sys_defs.h"
#include "../../ps3emu/constants.h"
#include "sysutil_sysparam.h"

#define GCM_EMU
#include <gcm_tool.h>

class PPU;

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

namespace emu {
namespace Gcm {

// typedef struct CellGcmConfig {
//     boost::endian::big_uint32_t localAddress;
//     boost::endian::big_uint32_t ioAddress;
//     boost::endian::big_uint32_t localSize;
//     boost::endian::big_uint32_t ioSize;
//     boost::endian::big_uint32_t memoryFrequency;
//     boost::endian::big_uint32_t coreFrequency;
// } CellGcmConfig;
// 
// typedef struct CellGcmSurface{
//     uint8_t type;
//     uint8_t antialias;
//     uint8_t colorFormat;
//     uint8_t colorTarget;
//     uint8_t colorLocation[CELL_GCM_MRT_MAXCOUNT];
//     boost::endian::big_uint32_t colorOffset[CELL_GCM_MRT_MAXCOUNT];
//     boost::endian::big_uint32_t colorPitch[CELL_GCM_MRT_MAXCOUNT];
//     uint8_t depthFormat;
//     uint8_t depthLocation;
//     uint8_t _padding[2];
//     boost::endian::big_uint32_t depthOffset;
//     boost::endian::big_uint32_t depthPitch;
//     boost::endian::big_uint16_t width;
//     boost::endian::big_uint16_t height;
//     boost::endian::big_uint16_t x;
//     boost::endian::big_uint16_t y;__restrict
// } CellGcmSurface;

uint32_t _cellGcmInitBody(ps3_uintptr_t callGcmCallback, uint32_t cmdSize, uint32_t ioSize, ps3_uintptr_t ioAddress, PPU* ppu);

emu_void_t cellGcmSetFlipMode(uint32_t mode);
emu_void_t cellGcmGetConfiguration(CellGcmConfig *config);
int32_t cellGcmAddressToOffset(uint32_t address, boost::endian::big_uint32_t *offset);
emu_void_t cellGcmSetSurface(const CellGcmSurface *surface);
int32_t cellGcmSetDisplayBuffer(const uint8_t id,
                                const uint32_t offset,
                                const uint32_t pitch,
                                const uint32_t width,
                                const uint32_t height);
ps3_uintptr_t cellGcmGetControlRegister();
ps3_uintptr_t cellGcmGetLabelAddress(uint8_t index);
uint32_t cellGcmGetFlipStatus(PPU* ppu);
emu_void_t cellGcmResetFlipStatus(PPU* ppu);
emu_void_t _cellGcmSetFlipCommand(PPU* ppu);

}}