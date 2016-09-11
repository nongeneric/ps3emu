/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/spu-side/raw_spu
 *
 *   File: responder.ppu.c
 *
 *   Description:
 *     These functions for a PPU program responds to queries of a Raw SPU
 *     synchronously.
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

int respondToRoundTripByRawSpu(void);


#if (SPU_TO_PPU == SPU_OUTBOUND_INTERRUPT_MAILBOX_HANDLE)

#include <sys/synchronization.h>

#define TRUE	1
#define FALSE	0

/**
 *E Synchronization variables between the primary PPU thread and an interrupt
 *  PPU thread
 */
sys_mutex_t mutexId;
sys_cond_t condId;
uint8_t isSignaled = 0;
uint32_t numberOfWaiter = 0;

/**
 *E Interrupt PPU thread to handle interrupts caused by the SPU Outbound
 *  Interrupt Mailbox
 */
void handleRawSpuInterrupt(uint64_t arg);

#endif /* SPU_TO_PPU */


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


int respondToRoundTripByRawSpu(void)
{
	uint32_t outMboxValue = 0x0u;


#if (SPU_TO_PPU == SPU_OUTBOUND_INTERRUPT_MAILBOX_HANDLE)

	/*E Create a condition variable. */

	int ret = 0;
	sys_mutex_attribute_t mutexAttribute;
	sys_cond_attribute_t condAttribute;

	sys_mutex_attribute_initialize(mutexAttribute);
	ret = sys_mutex_create(&mutexId, &mutexAttribute);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_mutex_create() failed: 0x%08x\n", ret);
		return -1;
	}

	sys_cond_attribute_initialize(condAttribute);
	ret = sys_cond_create(&condId, mutexId, &condAttribute);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_cond_create() failed: 0x%08x\n", ret);
		sys_mutex_destroy(mutexId);
		return -2;
	}
#endif /* SPU_TO_PPU */


	/**
	 *E Send buffer addresses to the Raw SPU via the SPU Inbound Mailbox.
	 */
	eaBuffer.dma	= (uint32_t)dmaBuffer;
	eaBuffer.lock	= (uint32_t)lockLine;
	eaBuffer.debug	= (uint32_t)debugBuffer;
	writeSpuInboundMailbox((uint32_t)&eaBuffer);


	for (uint32_t i = 0u; i < NUMBER_OF_ITERATION; i++) {

		/*E Initialize a DMA buffer. */
		__sync();
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


		/**
		 *E Send a message to the SPU to notify that the PPU is ready via the
		 *  Signal Notification 2 Register.
		 */
		__isync();
		__sync();
		sys_raw_spu_mmio_write(gRawSpuId, SPU_Sig_Notify_2, PPU_IS_READY);


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
			return -3;
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
			return -4;
		}
		if (__builtin_expect(outMboxValue != SPU_TO_PPU_SYNC_VALUE, 0)) {
			fprintf(stderr, "Unexpected data has arrived: 0x%08x\n",
													outMboxValue);
			fprintf(stderr, "          -- Expected data : 0x%08x\n",
													SPU_TO_PPU_SYNC_VALUE);
			return -5;
		}

#elif (SPU_TO_PPU == SPU_OUTBOUND_INTERRUPT_MAILBOX_HANDLE)

		/*E Wait for a condition variable signal from a interrupt thread. */

		ret = sys_mutex_lock(mutexId, SYS_NO_TIMEOUT);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_mutex_lock() failed: 0x%08x\n", ret);
			sys_cond_signal_all(condId);
			sys_cond_destroy(condId);
			sys_mutex_destroy(mutexId);
			return -6;
		}

		if (isSignaled) {
			isSignaled = FALSE;
		} else {
			numberOfWaiter++;
			ret = sys_cond_wait(condId, SYS_NO_TIMEOUT);
			if (ret != CELL_OK) {
				fprintf(stderr, "sys_cond_wait() failed: 0x%08x\n", ret);
				sys_cond_signal_all(condId);
				sys_cond_destroy(condId);
				sys_mutex_unlock(mutexId);
				sys_mutex_destroy(mutexId);
				return -7;
			}
		}

		ret = sys_mutex_unlock(mutexId);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_mutex_unlock() failed: 0x%08x\n", ret);
			sys_cond_signal_all(condId);
			sys_cond_destroy(condId);
			sys_mutex_destroy(mutexId);
			return -8;
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
			return -9;
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
		__sync();

#elif (PPU_TO_SPU == SPU_INBOUND_MAILBOX)

		/*E Write a value to the SPU Inbound Mailbox Register. */
		sys_raw_spu_mmio_write(gRawSpuId, SPU_In_MBox, PPU_TO_SPU_SYNC_VALUE);
		__eieio();

#elif (PPU_TO_SPU == SIGNAL_NOTIFICATION)

		/*E Write a value to the SPU Signal Notification 1 Register. */
		sys_raw_spu_mmio_write(gRawSpuId, SPU_Sig_Notify_1, PPU_TO_SPU_SYNC_VALUE);
		__eieio();

#else
#error : Unexpected Case of "PPU_TO_SPU".
#endif /* PPU_TO_SPU */

		/**
		 *E Receive the decrenmenter value from SPU.
		 *  (Please see the above NOTICE.)
		 */
		do {} while (!(sys_raw_spu_mmio_read(gRawSpuId, SPU_MBox_Status) & SPU_OUT_MBOX_COUNT));
		outMboxValue = sys_raw_spu_mmio_read(gRawSpuId, SPU_Out_MBox);
		observed.record[i] = outMboxValue;

	} /* for - i (NUMBER_OF_ITERATION) */


	/**
	 *E Check the SPU Mailbox Status Register in order to receive a data to
	 *  notify the end of the Raw SPU execution.
	 *  (Please see the above NOTICE.)
	 */
	__eieio();
	do {} while (!(sys_raw_spu_mmio_read(gRawSpuId, SPU_MBox_Status) & SPU_OUT_MBOX_COUNT));
	outMboxValue = sys_raw_spu_mmio_read(gRawSpuId, SPU_Out_MBox);
	if (outMboxValue != RAW_SPU_HAS_FINISHED_EXECUTION) {
		fprintf(stderr, "Unexpected data has arrived: 0x%08x\n",
											outMboxValue);
		fprintf(stderr, "          -- Expected data : 0x%08x\n",
											RAW_SPU_HAS_FINISHED_EXECUTION);
		return -10;
	}

#if (SPU_TO_PPU == SPU_OUTBOUND_INTERRUPT_MAILBOX_HANDLE)

	ret = sys_cond_destroy(condId);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_cond_destroy() failed: 0x%08x\n", ret);
		sys_mutex_destroy(mutexId);
		return -11;
	}

	ret = sys_mutex_destroy(mutexId);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_mutex_destroy() failed: 0x%08x\n", ret);
		return -12;
	}

#endif /* SPU_TO_PPU */

	return 0;
}



#if (SPU_TO_PPU == SPU_OUTBOUND_INTERRUPT_MAILBOX_HANDLE)

/**
 *E  Definition of the interrupt thread to handle interrupts caused by writing the
 *   SPU Outbound Interrupt Mailbox. (One of the class 2 SPU interrupts)
 */
void handleRawSpuInterrupt(uint64_t arg)
{
	uint64_t stat = 0x0ull;
	uint32_t outMboxValue = 0u;
	int ret = 0;

	(void)arg;	/*E This thread does not use the argument */

	/**
	 *E Check Class 2 Interrupt Status Register
	 *  to distinguish the interrupt cause.
	 */
	ret = sys_raw_spu_get_int_stat(gRawSpuId, SPU_INTR_CLASS_2, &stat);
	if (__builtin_expect(ret != CELL_OK, 0)) {
		fprintf(stderr, "sys_raw_spu_get_int_stat() failed: 0x%08x\n", ret);
		fprintf(stderr, "Cannot handle this interrupt. Exiting...\n");
		sys_interrupt_thread_eoi();
	}

	if (__builtin_expect(stat & (!OUT_INTR_MBOX_MASK), 0)) {
		fprintf(stderr, "Unexpected Interrupt cause: 0x%016llx\n", stat);
		fprintf(stderr, "         -- Expected cause: 0x%016llx\n",
												(uint64_t)OUT_INTR_MBOX_MASK);
		fprintf(stderr, "Cannot handle this interrupt. Exiting...\n");

		/*E Reset Class 2 Interrupt Status Register. */
		ret = sys_raw_spu_set_int_stat(gRawSpuId, SPU_INTR_CLASS_2, stat);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_raw_spu_set_int_stat() failed: 0x%08x\n", ret);
			fprintf(stderr, "Could not reset interrupt status.  Exiting...\n");
		}
		sys_interrupt_thread_eoi();
	}


	/*E   PPU <-- SPU   */

	/*E Read the SPU Outbound Interrupt Mailbox Register */
	ret = sys_raw_spu_read_puint_mb(gRawSpuId, &outMboxValue);
	if (__builtin_expect(ret != CELL_OK, 0)) {
		fprintf(stderr, "sys_raw_spu_read_puint_mb() failed: 0x%08x\n", ret);

		/*E Reset Class 2 Interrupt Status Register. */
		ret = sys_raw_spu_set_int_stat(gRawSpuId, SPU_INTR_CLASS_2,
														OUT_INTR_MBOX_MASK);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_raw_spu_set_int_stat() failed: 0x%08x\n", ret);
			fprintf(stderr, "Could not reset interrupt status.  Exiting...\n");
		}
		sys_interrupt_thread_eoi();
	}

	if (__builtin_expect(outMboxValue != SPU_TO_PPU_SYNC_VALUE, 0)) {
		fprintf(stderr, "Unexpected data has arrived: 0x%08x\n",
												outMboxValue);
		fprintf(stderr, "          -- Expected data : 0x%08x\n",
												SPU_TO_PPU_SYNC_VALUE);

		/*E Reset Class 2 Interrupt Status Register. */
		ret = sys_raw_spu_set_int_stat(gRawSpuId, SPU_INTR_CLASS_2,
														OUT_INTR_MBOX_MASK);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_raw_spu_set_int_stat() failed: 0x%08x\n", ret);
			fprintf(stderr, "Could not reset interrupt status.  Exiting...\n");
		}
		sys_interrupt_thread_eoi();
	}


	/*E   PPU --> SPU   */

#if (PPU_TO_SPU == GETLLAR_POLLING)

	/*E Store a value to the lock line */
	*(uint32_t *)lockLine = PPU_TO_SPU_SYNC_VALUE;

#else
#error : Unexpected Case of "PPU_TO_SPU".
#endif /* PPU_TO_SPU */


	/*E Reset Class 2 Interrupt Status Register. */
	ret = sys_raw_spu_set_int_stat(gRawSpuId, SPU_INTR_CLASS_2, OUT_INTR_MBOX_MASK);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_raw_spu_set_int_stat() failed: 0x%08x\n", ret);
		fprintf(stderr, "Could not reset interrupt status.  Exiting...\n");
		sys_interrupt_thread_eoi();
	}


	/**
	 *E Wakeup the primary PPU thread
	 */

	ret = sys_mutex_lock(mutexId, SYS_NO_TIMEOUT);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_mutex_lock() failed: 0x%08x\n", ret);
		fprintf(stderr, "Could not lock a mutex.  Exiting...\n");
		sys_interrupt_thread_eoi();
	}

	if (numberOfWaiter == 0) {
		isSignaled = TRUE;
	} else {
		ret = sys_cond_signal_all(condId);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_cond_wait() failed: 0x%08x\n", ret);
			fprintf(stderr, "Cannot wake up the primary PPU thread. Exitiing...\n");
			sys_interrupt_thread_eoi();
		}
		numberOfWaiter = 0;
	}

	ret = sys_mutex_unlock(mutexId);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_mutex_unlock() failed: 0x%08x\n", ret);
		fprintf(stderr, "Could not unlock a mutex.  Exiting...\n");
		sys_interrupt_thread_eoi();
	}

	/*E End of interrupt PPU thread */
	sys_interrupt_thread_eoi();
}

#endif /* SPU_TO_PPU */

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
