/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/spu-side/spu_thread
 *
 *   File: responder.ppu.c
 *
 *   Description:
 *     This function for a PPU program responds to queries of a SPU thread
 *     synchronously.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ppu_intrinsics.h>
#include <sys/spu_thread.h>
#include <sys/event.h>

#include "association.h"
#include "ppu/buffer.h"

#include "ppu/spu_thread.h"
#define SNR1	0
#define SNR2	1

/**
 *E  DMA/Atomic buffers
 */
volatile uint8_t dmaBuffer[CBE_CACHE_LINE]		__ALIGNED_CBE_CACHE_LINE__;
volatile uint8_t lockLine[CBE_CACHE_LINE]		__ALIGNED_CBE_CACHE_LINE__;
volatile uint8_t syncBuffer[CBE_CACHE_LINE]		__ALIGNED_CBE_CACHE_LINE__;
volatile uint8_t debugBuffer[CBE_CACHE_LINE]	__ALIGNED_CBE_CACHE_LINE__;

ObservedValues observed __ALIGNED_CBE_CACHE_LINE__;


int respondToRoundTripBySpuThread(void);


int respondToRoundTripBySpuThread(void)
{
	sys_event_t event;
	int ret;

	for (uint32_t i = 0u; i < NUMBER_OF_ITERATION; i++) {

		/*E Initialize a DMA buffer. */
		memset((void *)(uintptr_t)dmaBuffer, INIT_MEMSET_VALUE, CBE_CACHE_LINE);

#if (ENABLE_CACHE_FLUSH)
		__sync();
		__dcbf((uintptr_t)dmaBuffer);
		__dcbt((uintptr_t)dmaBuffer);
#endif /* ENABLE_CACHE_FLUSH */


#if (PPU_TO_SPU == GETLLAR_POLLING)

		/*E Initialize a lock line. */
		memset((void *)(uintptr_t)lockLine, INIT_MEMSET_VALUE, CBE_CACHE_LINE);
		*(uint32_t *)lockLine = INIT_LOCK32_VALUE;

#endif /* PPU_TO_SPU */


		/*E Send a signal to the SPU to notify that the PPU is ready. */
		ret = sys_spu_thread_write_snr(gSpuThreadId, SNR2, PPU_IS_READY);
		if (__builtin_expect(ret != CELL_OK, 0)) {
			fprintf(stderr, "sys_spu_thread_write_snr() failed: 0x%08x\n", ret);
			return -1;
		}


		/**
		 *E  PPU <-- SPU
		 */

#if (SPU_TO_PPU == DMA_PUT || SPU_TO_PPU == ATOMIC_PUTLLUC)

		/*E Poll the DMA buffer. */
		do {} while (__builtin_expect(dmaBuffer[0] == INIT_MEMSET_VALUE, 0));

		/*E Read the buffer. */
		if (__builtin_expect(
				*(volatile uint32_t *)dmaBuffer != SPU_TO_PPU_SYNC_VALUE, 0)) {
			fprintf(stderr, "Unexpected data has arrived: 0x%08x\n",
													*(uint32_t *)dmaBuffer);
			fprintf(stderr, "          -- Expected data : 0x%08x\n",
													SPU_TO_PPU_SYNC_VALUE);
			return -2;
		}

#elif (SPU_TO_PPU == EVENT_QUEUE_SEND || SPU_TO_PPU == EVENT_QUEUE_THROW)

		ret = sys_event_queue_receive(gEventQueueId, &event, SYS_NO_TIMEOUT);
		if (__builtin_expect(ret != CELL_OK
						|| event.source != SYS_SPU_THREAD_EVENT_USER_KEY, 0)) {
			fprintf(stderr, "sys_event_queue_receive() failed: 0x%08x\n", ret);
			fprintf(stderr, "                    event.source: %llu\n",
													event.source);
			return -3;
		}
		if (__builtin_expect(
						(uint32_t)event.data3 != SPU_TO_PPU_SYNC_VALUE, 0)) {
			fprintf(stderr, "Unexpected data has arrived: 0x%08x\n",
													(uint32_t)event.data3);
			fprintf(stderr, "          -- Expected data : 0x%08x\n",
													SPU_TO_PPU_SYNC_VALUE);
			return -4;
		}

#else
#error : Unexpected Case of "SPU_TO_PPU".
#endif /* SPU_TO_PPU */


		/**
		 *E  PPU --> SPU
		 */

#if (PPU_TO_SPU == GETLLAR_POLLING)

		/*E Store a value to the lock line. */
		*(uint32_t *)lockLine = PPU_TO_SPU_SYNC_VALUE;

#elif (PPU_TO_SPU == SIGNAL_NOTIFICATION)

		ret = sys_spu_thread_write_snr(gSpuThreadId, SNR1,
												PPU_TO_SPU_SYNC_VALUE);
		if (__builtin_expect(ret != CELL_OK, 0)) {
			fprintf(stderr, "sys_spu_thread_write_snr() failed: 0x%08x\n", ret);
			return -5;
		}

#else
#error : Unexpected Case of "PPU_TO_SPU".
#endif /* PPU_TO_SPU */


		/*E Receive the decrenmenter value from SPU. */
		ret = sys_event_queue_receive(gEventQueueId, &event, SYS_NO_TIMEOUT);
		if (__builtin_expect(ret != CELL_OK
						|| event.source != SYS_SPU_THREAD_EVENT_USER_KEY, 0)) {
			fprintf(stderr, "sys_event_queue_receive() failed: 0x%08x\n", ret);
			fprintf(stderr, "                    event.source: %llu\n",
													event.source);
			return -6;
		}

		observed.record[i] = (uint32_t)event.data3;
		__lwsync();

	} /* for - i (NUMBER_OF_ITERATION) */

	return 0;
}

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
