/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#ifndef __PARTICLE_DATA_H
#define __PARTICLE_DATA_H

#include <stdint.h>

// E This PPU pointer type acts on the PPU side as the native
// E type, and on the SPU side as a 32 bit unsigned integer,
// E since it isn't an actual pointer to data in SPU memory.

#if __PPU__
#define PPU_POINTER(type) type *
#elif __SPU__
#define PPU_POINTER(type) uint32_t
#endif

typedef enum cmdParticleSimulator {
	CMD_RELOAD_POINTERS=0,
	CMD_INIT_PARTICLES,
	CMD_SIMULATE_PARTICLES,
	CMD_EXIT,
} cmdParticleSimulator;

typedef struct particlePointerInfo {
	// E This ensures that this DMA structure is a multiple of 16 bytes
	PPU_POINTER(Point3) ppu_P3Positions __attribute__ ((aligned(16)));
	PPU_POINTER(Vector3) ppu_V3Velocities;
  
	PPU_POINTER(float) ppu_fPhases;
}  __attribute__ ((aligned(16))) particlePointerInfo;

#endif
