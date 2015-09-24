#pragma once

#include "../sys_defs.h"
#include "../../ps3emu/constants.h"
#include "sysutil_sysparam.h"

#define GCM_EMU
#include <gcm_tool.h>

class PPU;

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

void cellGcmSetFlipMode(uint32_t mode);
void cellGcmGetConfiguration(CellGcmConfig *config);
int32_t cellGcmAddressToOffset(uint32_t address, boost::endian::big_uint32_t *offset);

void cellGcmSetSurface(const CellGcmSurface *surface);

}}