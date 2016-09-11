/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/ppu-side/spu_thread
 *
 *   File: observer.ppu.c
 *
 *   Description:
 *     This function for a PPU program observes time of PPU-side round trips.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ppu_intrinsics.h>
#include <sys/spu_thread.h>
#include <sys/event.h>
#include <sys/time_util.h>

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

int observeRoundTripBySpuThread(void);


int observeRoundTripBySpuThread(void)
{
	for (uint32_t i = 0u; i < NUMBER_OF_ITERATION; i++) {

		register uint64_t tb_start, tb_end;

		/*E Initialize a DMA buffer. */
		__lwsync();
		memset((void *)(uintptr_t)dmaBuffer, INIT_MEMSET_VALUE, CBE_CACHE_LINE);
		memset((void *)(uintptr_t)syncBuffer, INIT_MEMSET_VALUE, CBE_CACHE_LINE);

#if (ENABLE_CACHE_FLUSH)
		__sync();
		__dcbf((uintptr_t)dmaBuffer);
#endif /* ENABLE_CACHE_FLUSH */


#if (PPU_TO_SPU == LLR_LOST_EVENT || PPU_TO_SPU == GETLLAR_POLLING)

		/*E Initialize a lock line. */
		memset((void *)(uintptr_t)lockLine, INIT_MEMSET_VALUE, CBE_CACHE_LINE);
		*(uint32_t *)lockLine = INIT_LOCK32_VALUE;

#endif /* PPU_TO_SPU */


		/**
		 *E Send a message to the SPU that notifies all buffers has been
		 *  initialized.
		 */
		__sync();
		*(volatile uint32_t *)syncBuffer = PPU_HAS_INITIALIZED_BUFFERS;

		/**
		 *E Receive a message that notifies the SPU is ready.
		 */
		__sync();
		do {} while (*(volatile uint32_t *)syncBuffer != SPU_IS_READY);


#define DUMMY_LOOP 1
#if DUMMY_LOOP
		for (uint32_t cnt = 0u; cnt < 5000; cnt++) { __nop(); }
#endif /* DUMMY_LOOP */


		/*E Get the start value of the Time Base. */
		__isync();
		__sync();
		SYS_TIMEBASE_GET(tb_start);


		/**
		 *E  PPU --> SPU
		 */

#if (PPU_TO_SPU == LLR_LOST_EVENT || PPU_TO_SPU == GETLLAR_POLLING)

		/*E Store a value to the lock line. */
		*(uint32_t *)lockLine = PPU_TO_SPU_SYNC_VALUE;

#elif (PPU_TO_SPU == SIGNAL_NOTIFICATION)

		int ret1 = sys_spu_thread_write_snr(gSpuThreadId, SNR1,
												PPU_TO_SPU_SYNC_VALUE);
		if (__builtin_expect(ret1 != CELL_OK, 0)) {
			fprintf(stderr, "sys_spu_thread_write_snr() failed: 0x%08x\n", ret1);
			return -1;
		}

#else
#error : Unexpected Case of "PPU_TO_SPU".
#endif /* PPU_TO_SPU */


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

		sys_event_t event;

		int ret2 = sys_event_queue_receive(gEventQueueId, &event, SYS_NO_TIMEOUT);
		if (__builtin_expect(ret2 != CELL_OK
						|| event.source != SYS_SPU_THREAD_EVENT_USER_KEY, 0)) {
			fprintf(stderr, "sys_event_queue_receive() failed: 0x%08x\n", ret2);
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


		/*E Get the end value of the Time Base. */
		SYS_TIMEBASE_GET(tb_end);
		observed.record[i] = (uint32_t)tb_end - (uint32_t)tb_start;

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
