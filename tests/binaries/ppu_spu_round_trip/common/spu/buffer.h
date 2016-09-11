/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common/
 *
 *   File: spu/buffer.h
 *
 *   Description:
 *     DMA Buffer (on Local Storage) Settings
 */

#ifndef __SAMPLE_ROUND_TRIP_SPU_BUFFER_H__
#define __SAMPLE_ROUND_TRIP_SPU_BUFFER_H__

#include "../association.h"

extern volatile SharedBufferAddress eaBuffer __ALIGNED_CBE_CACHE_LINE__;

extern volatile uint8_t lsDmaBuffer[CBE_CACHE_LINE]		__ALIGNED_CBE_CACHE_LINE__;
extern volatile uint8_t lsLockLine[CBE_CACHE_LINE]		__ALIGNED_CBE_CACHE_LINE__;
extern volatile uint8_t lsSyncBuffer[CBE_CACHE_LINE]	__ALIGNED_CBE_CACHE_LINE__;
extern volatile uint8_t lsDebugBuffer[CBE_CACHE_LINE]	__ALIGNED_CBE_CACHE_LINE__;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void initializeLsBuffer(void);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __SAMPLE_ROUND_TRIP_SPU_BUFFER_H__ */

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
