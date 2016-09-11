/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common
 *
 *   File: ppu/buffer.h
 *
 *   Description:
 *     DMA Buffer Settings
 */

#ifndef __SAMPLE_ROUND_TRIP_PPU_BUFFER_H__
#define __SAMPLE_ROUND_TRIP_PPU_BUFFER_H__

#include "association.h"

extern volatile uint8_t dmaBuffer[CBE_CACHE_LINE]	__ALIGNED_CBE_CACHE_LINE__;
extern volatile uint8_t lockLine[CBE_CACHE_LINE]	__ALIGNED_CBE_CACHE_LINE__;
extern volatile uint8_t syncBuffer[CBE_CACHE_LINE]	__ALIGNED_CBE_CACHE_LINE__;
extern volatile uint8_t debugBuffer[CBE_CACHE_LINE]	__ALIGNED_CBE_CACHE_LINE__;

typedef struct ObservedValues {
	uint32_t record[NUMBER_OF_ITERATION];
	uint32_t min;
	uint32_t max;
	uint64_t sum;
	uint32_t avg;
} ObservedValues __ALIGNED_CBE_CACHE_LINE__;

extern ObservedValues observed __ALIGNED_CBE_CACHE_LINE__;

#endif /* __SAMPLE_ROUND_TRIP_PPU_BUFFER_H__ */

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
