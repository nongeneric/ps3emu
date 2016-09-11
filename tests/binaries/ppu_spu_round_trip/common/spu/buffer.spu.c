/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common/
 *
 *   File: spu/buffer.spu.c
 *
 *   Description:
 *     DMA Buffer (on Local Storage) Settings
 */

#include <stdint.h>
#include <spu_intrinsics.h>

#include "buffer.h"

#define LS_INIT_32	(0xDEADBEEFU)

volatile uint8_t lsDmaBuffer[CBE_CACHE_LINE]	__ALIGNED_CBE_CACHE_LINE__;
volatile uint8_t lsLockLine[CBE_CACHE_LINE]		__ALIGNED_CBE_CACHE_LINE__;
volatile uint8_t lsSyncBuffer[CBE_CACHE_LINE]	__ALIGNED_CBE_CACHE_LINE__;
volatile uint8_t lsDebugBuffer[CBE_CACHE_LINE]	__ALIGNED_CBE_CACHE_LINE__;


void initializeLsBuffer(void)
{
	/**
	 *E Write the "LS_INIT_32" to all slots (all 32 bits) of buffers by using
	 *  spu_splats().
	 */
	for (int i = 0; i < CBE_CACHE_LINE; i += sizeof(qword)) {
		*(volatile vec_uint4 *)&lsLockLine[i] =
								(vec_uint4)spu_splats((uint32_t)LS_INIT_32);
		*(volatile vec_uint4 *)&lsDmaBuffer[i] =
								(vec_uint4)spu_splats((uint32_t)LS_INIT_32);
		*(volatile vec_uint4 *)&lsSyncBuffer[i] =
								(vec_uint4)spu_splats((uint32_t)LS_INIT_32);
		*(volatile vec_uint4 *)&lsDebugBuffer[i] =
								(vec_uint4)spu_splats((uint32_t)LS_INIT_32);
	}

	/**
	 *E Write the 32 bits value "SPU_TO_PPU_SYNC_VALUE" to the preferred slot
	 *  (the first 32 bits) of "*(vec_uint4 *)lsDmaBuffer" by spu_insert().
	 */
	*(volatile vec_uint4 *)lsDmaBuffer = (vec_uint4)spu_insert(
											(uint32_t)SPU_TO_PPU_SYNC_VALUE,
											*(vec_uint4 *)lsDmaBuffer,
											0);
	return;
}

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
