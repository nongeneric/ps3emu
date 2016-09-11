/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/ppu-side/raw_spu
 *
 *   File: observer.ppu.c
 *
 *   Description:
 *     This function for a PPU program observes time of PPU-side round trips.
 */

/**
 *E  NOTICE !!!
 *   This sample program contains codes to poll the SPU Mailbox Status Register.
 *   But, another hardware thread may be influenced by polling MMIO registers.
 *   Normally, please avoid frequent access to them.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ppu_intrinsics.h>
#include <sys/time_util.h>

#include "association.h"
#include "ppu/buffer.h"

#include "ppu/raw_spu.h"

/**
 *E  DMA/Atomic buffers
 */
volatile uint8_t dmaBuffer[CBE_CACHE_LINE]		__ALIGNED_CBE_CACHE_LINE__;
volatile uint8_t lockLine[CBE_CACHE_LINE]		__ALIGNED_CBE_CACHE_LINE__;
volatile uint8_t debugBuffer[CBE_CACHE_LINE]	__ALIGNED_CBE_CACHE_LINE__;

ObservedValues observed __ALIGNED_CBE_CACHE_LINE__;

volatile SharedBufferAddress eaBuffer __ALIGNED_CBE_CACHE_LINE__;

int observeRoundTripByRawSpu(void);


static void writeSpuInboundMailbox(uint32_t value)
{
	uint8_t isPolling = 1;
	do {
		/*E Check the SPU Mailbox Status Register. */
		if ((sys_raw_spu_mmio_read(gRawSpuId, SPU_MBox_Status) & SPU_IN_MBOX_COUNT)) {
		    /*E Write a value to the SPU Inbound Mailbox Register. */
			sys_raw_spu_mmio_write(gRawSpuId, SPU_In_MBox, value);
			isPolling = 0;
		} else {
			/*E Avoid frequent access to the SPU Mailbox Status Register. */
			for (uint32_t i = 0; i < 100; i++) { __nop(); }
		}
	} while (isPolling);
}


int observeRoundTripByRawSpu(void)
{
	uint32_t outMboxValue = 0x0u;

	/**
	 *E Send buffer addresses to the Raw SPU via the SPU Inbound Mailbox.
	 */
	eaBuffer.dma	= (uint32_t)dmaBuffer;
	eaBuffer.lock	= (uint32_t)lockLine;
	eaBuffer.debug	= (uint32_t)debugBuffer;
	writeSpuInboundMailbox((uint32_t)&eaBuffer);

	for (uint32_t i = 0u; i < NUMBER_OF_ITERATION; i++) {

		register uint64_t tb_start, tb_end;

		/*E Initialize a DMA buffer. */
		__sync();
		memset((void *)(uintptr_t)dmaBuffer, INIT_MEMSET_VALUE, CBE_CACHE_LINE);

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
		 *  initialized via the SPU Inbound Mailbox Register.
		 */
		__sync();
		writeSpuInboundMailbox(PPU_HAS_INITIALIZED_BUFFERS);

		/**
		 *E Receive a message that notifies the SPU is ready.
		 *  (Please see the above NOTICE.)
		 */
		__eieio();
		do {} while (!(sys_raw_spu_mmio_read(gRawSpuId, SPU_MBox_Status) & SPU_OUT_MBOX_COUNT));
		outMboxValue = sys_raw_spu_mmio_read(gRawSpuId, SPU_Out_MBox);
		if (__builtin_expect(outMboxValue != SPU_IS_READY, 0)) {
			fprintf(stderr, "Unexpected data has arrived: 0x%08x\n",
													outMboxValue);
			fprintf(stderr, "          -- Expected data : 0x%08x\n",
													SPU_IS_READY);
			return -1;
		}


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

#elif (PPU_TO_SPU == SPU_INBOUND_MAILBOX)

		/*E Write a value to the SPU Inbound Mailbox Register. */
		sys_raw_spu_mmio_write(gRawSpuId, SPU_In_MBox, PPU_TO_SPU_SYNC_VALUE);

#elif (PPU_TO_SPU == SIGNAL_NOTIFICATION)

		/*E Write a value to the SPU Signal Notification 1 Register. */
		sys_raw_spu_mmio_write(gRawSpuId, SPU_Sig_Notify_1, PPU_TO_SPU_SYNC_VALUE);

#else
#error : Unexpected Case of "PPU_TO_SPU".
#endif /* PPU_TO_SPU */


		/**
		 *E  PPU <-- SPU
		 */

#if (SPU_TO_PPU == SPU_OUTBOUND_MAILBOX)

		/**
		 *E Check the SPU Mailbox Status Register.
		 *  (Please see the above NOTICE.)
		 */
		do {} while (!(sys_raw_spu_mmio_read(gRawSpuId, SPU_MBox_Status) & SPU_OUT_MBOX_COUNT));

		/*E Read the SPU Outbound Mailbox Register. */
		outMboxValue = sys_raw_spu_mmio_read(gRawSpuId, SPU_Out_MBox);
		if (__builtin_expect(outMboxValue != SPU_TO_PPU_SYNC_VALUE, 0)) {
			fprintf(stderr, "Unexpected data has arrived: 0x%08x\n",
													outMboxValue);
			fprintf(stderr, "          -- Expected data : 0x%08x\n",
													SPU_TO_PPU_SYNC_VALUE);
			return -2;
		}

#elif (SPU_TO_PPU == SPU_OUTBOUND_INTERRUPT_MAILBOX)

		int ret;

		/**
		 *E Check the SPU Mailbox Status Register.
		 *  (Please see the above NOTICE.)
		 */
		do {} while (!(sys_raw_spu_mmio_read(gRawSpuId, SPU_MBox_Status) & SPU_OUT_INTR_MBOX_COUNT));

		/*E Read the SPU Outbound Interrupt Mailbox Register. */
		ret = sys_raw_spu_read_puint_mb(gRawSpuId, &outMboxValue);
		if (__builtin_expect(ret != CELL_OK, 0)) {
			fprintf(stderr, "sys_raw_spu_read_puint_mb() failed: 0x%08x\n",
													ret);
			return -3;
		}
		if (__builtin_expect(outMboxValue != SPU_TO_PPU_SYNC_VALUE, 0)) {
			fprintf(stderr, "Unexpected data has arrived: 0x%08x\n",
													outMboxValue);
			fprintf(stderr, "          -- Expected data : 0x%08x\n",
													SPU_TO_PPU_SYNC_VALUE);
			return -4;
		}

#elif (SPU_TO_PPU == DMA_PUT || SPU_TO_PPU == ATOMIC_PUTLLUC)

		/*E Poll the DMA buffer. */
		do {} while (__builtin_expect(dmaBuffer[0] == INIT_MEMSET_VALUE, 0));

		/*E Read the buffer. */
		if (__builtin_expect(
				*(volatile uint32_t *)dmaBuffer != SPU_TO_PPU_SYNC_VALUE, 0)) {
			fprintf(stderr, "Unexpected data has arrived: 0x%08x\n",
													*(uint32_t *)dmaBuffer);
			fprintf(stderr, "          -- Expected data : 0x%08x\n",
													SPU_TO_PPU_SYNC_VALUE);
			return -5;
		}

#else
#error : Unexpected Case of "SPU_TO_PPU".
#endif /* SPU_TO_PPU */


		/*E Get the end value of the Time Base. */
		SYS_TIMEBASE_GET(tb_end);
		observed.record[i] = (uint32_t)tb_end - (uint32_t)tb_start;

	} /* for - i (NUMBER_OF_ITERATION) */


	/**
	 *E Check the SPU Mailbox Status Register in order to receive a data to
	 *  notify the end of the Raw SPU execution.
	 *  (Please see the above NOTICE.)
	 */
	do {} while (!(sys_raw_spu_mmio_read(gRawSpuId, SPU_MBox_Status) & SPU_OUT_MBOX_COUNT));
	outMboxValue = sys_raw_spu_mmio_read(gRawSpuId, SPU_Out_MBox);
	if (outMboxValue != RAW_SPU_HAS_FINISHED_EXECUTION) {
		fprintf(stderr, "Unexpected data has arrived: 0x%08x\n",
											outMboxValue);
		fprintf(stderr, "          -- Expected data : 0x%08x\n",
											RAW_SPU_HAS_FINISHED_EXECUTION);
		return -6;
	}

	return 0;
}

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
