/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common
 *
 *   File: ppu/statistics.h
 *
 *   Description:
 *     Functions to take statistics
 */

#ifndef __SAMPLE_ROUND_TRIP_PPU_STATISTICS_H__
#define __SAMPLE_ROUND_TRIP_PPU_STATISTICS_H__

#include <stdint.h>

#define CBE_TB_CLOCK_RATIO	40

extern uint64_t gTimeBaseFrequency;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

double calculateUsecPerTrial(uint32_t timebase);

void initializeObservedValues(void);
void analyzeObservedValues(void);

void showPpuToSpuCommunicationMethod(int ppuToSpu);
void showSpuToPpuCommunicationMethod(int spuToPpu);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __SAMPLE_ROUND_TRIP_PPU_STATISTICS_H__ */

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
