/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common
 *
 *   File: ppu/statistics.ppu.c
 *
 *   Description:
 *     Functions to take statistics
 */

#include <stdint.h>
#include <stdio.h>
#include "association.h"
#include "ppu/buffer.h"
#include "ppu/statistics.h"

uint64_t gTimeBaseFrequency;


double calculateUsecPerTrial(uint32_t timebase)
{
	double rtn = ((double)timebase / (double)gTimeBaseFrequency) * 1000.0 * 1000.0;
	return rtn;
}

void initializeObservedValues(void)
{
	for (uint32_t i = 0u; i < NUMBER_OF_ITERATION; i++) {
		observed.record[i] = 0;
	}
	observed.min = 0xffffffffu;
	observed.max = 0x0u;
	observed.sum = 0x0ull;
	observed.avg = 0x0u;
	return;
}

void analyzeObservedValues(void)
{
	for (uint32_t i = 0u; i < NUMBER_OF_ITERATION; i++) {
		uint32_t val = observed.record[i];
		observed.sum += (uint64_t)val;
		if (observed.max < val) { observed.max = val; }
		if (observed.min > val) { observed.min = val; }
	}
	observed.avg = (uint32_t)(observed.sum / (uint64_t)NUMBER_OF_ITERATION);
	return;
}

/**
 *E  Utility functions
 */
void showPpuToSpuCommunicationMethod(int ppuToSpu)
{
	printf("[ PPU -> SPU ] : %d\n", ppuToSpu);
	printf("  - reservation lost  : %d\n", LLR_LOST_EVENT);
	printf("  - getllar polling   : %d\n", GETLLAR_POLLING);
	printf("  - SPU_InMbox        : %d (Raw SPU)\n", SPU_INBOUND_MAILBOX);
	printf("  - SPU_Sig_Notify_1  : %d\n", SIGNAL_NOTIFICATION);
	return;
}

void showSpuToPpuCommunicationMethod(int spuToPpu)
{
	printf("[ PPU <- SPU ] : %d\n", spuToPpu);
	printf("  - SPU_OutMbox       : %d (Raw SPU)\n", SPU_OUTBOUND_MAILBOX);
	printf("  - SPU_OutIntrMbox   : %d (Raw SPU)\n", SPU_OUTBOUND_INTERRUPT_MAILBOX);
	printf("  - SPU_OutIntrMbox   : %d (Raw SPU - Interrupt PPU thread)\n",
										SPU_OUTBOUND_INTERRUPT_MAILBOX_HANDLE);
	printf("  - event queue send  : %d (SPU thread)\n", EVENT_QUEUE_SEND);
	printf("  - event queue throw : %d (SPU thread)\n", EVENT_QUEUE_THROW);
	printf("  - DMA     put       : %d\n", DMA_PUT);
	printf("  - Atomic  putlluc   : %d\n", ATOMIC_PUTLLUC);
	return;
}

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
