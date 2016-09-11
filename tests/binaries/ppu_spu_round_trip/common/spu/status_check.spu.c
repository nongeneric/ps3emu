/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common/
 *
 *   File: spu/status_check.spu.c
 *
 *   Description:
 *     Status check functions for debugging
 */

#include <stdint.h>
#include <spu_intrinsics.h>
#include <spu_mfcio.h>

#include "../association.h"
#include "spu/buffer.h"
#include "spu/status_check.h"

#define SPU_HALT \
do { \
	spu_hcmpeq(0, 0); \
	while(1); \
} while (0)

/*E DMA and buffer settings for status check functions */
#define DMA_TAG			(0x00U)
#define DMA_TAG_MASK	(1 << DMA_TAG)
#define TID				(0)
#define RID				(0)

uint32_t status[SIZE_OF_STATUS];
uint32_t gEaDebugBuffer;

static inline void issueDmaAndWait(void)
{
	mfc_put((volatile void *)lsDebugBuffer, (uint64_t)gEaDebugBuffer,
									CBE_CACHE_LINE, DMA_TAG, TID, RID);

	/*E Clear any pending tag status requests. */
	spu_writech(MFC_WrTagUpdate, 0);
	do {} while (spu_readchcnt(MFC_WrTagUpdate) == 0);
	spu_readch(MFC_RdTagStat);

	/*E Wait for a DMA tag group status update (All tag group completions). */
	spu_writech(MFC_WrTagMask, DMA_TAG_MASK);
	spu_writech(MFC_WrTagUpdate, MFC_TAG_UPDATE_ALL);
	spu_readch(MFC_RdTagStat);
	return;
}

void notifyError(uint32_t errorStatus, uint32_t expected, uint32_t observed)
{
	*(uint32_t *)lsDebugBuffer = errorStatus;
	*(uint32_t *)(lsDebugBuffer + 1 * sizeof(uint32_t)) = expected;
	*(uint32_t *)(lsDebugBuffer + 2 * sizeof(uint32_t)) = observed;
	*(uint32_t *)(lsDebugBuffer + 3 * sizeof(uint32_t)) = NO_DATA;
	issueDmaAndWait();
	SPU_HALT;
}

int checkStatus(uint32_t argc, uint32_t *argv)
{
	if (__builtin_expect(argc < 1, 0)) {
		return -1;
	}
	if (__builtin_expect(argc > (CBE_CACHE_LINE / sizeof(uint32_t)), 0)) {
		return -2;
	}

	for (uint32_t i = 0u; i < argc; i++) {
		*(uint32_t *)(lsDebugBuffer + i * sizeof(uint32_t)) = argv[i];
	}
	issueDmaAndWait();
	return 0;
}

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
