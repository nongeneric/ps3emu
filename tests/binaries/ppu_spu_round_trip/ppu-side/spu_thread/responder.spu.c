/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/ppu-side/spu_thread
 *
 *   File: responder.spu.c
 *
 *   Description:
 *     This SPU program responds to queries of a PPU thread synchronously.
 */

#include <stdint.h>
#include <spu_intrinsics.h>
#include <spu_mfcio.h>

#include <sys/spu_thread.h>
#include <sys/spu_event.h>
#include <sys/return_code.h>

#include "association.h"
#include "spu/buffer.h"
#include "spu/status_check.h"

/*E Event Settings */
#define MFC_NO_EVENT	(0x0000U)

/*E DMA and Buffer Settings */
#define DMA_TAG			(0x00U)
#define DMA_TAG_MASK	(1 << DMA_TAG)
#define TID				(0)
#define RID				(0)


int main(uint32_t eaDmaBuffer, uint32_t eaLockLine,
		 uint32_t eaSyncBuffer, uint32_t eaDebugBuffer)
{
	register uint32_t ch = 0u;
	int ret = 0;

	/**
	 *E Initialize buffers on the SPU local storage. (spu/buffer.spu.c)
	 *  And write "SPU_TO_PPU_SYNC_VALUE" to the first word of "lsDmaBuffer".
	 */
	initializeLsBuffer();

	/**
	 *E Send the buffer addresses that is known by the SPU to a shared buffer
	 *  just only for the debug. (spu/status_check.spu.c)
	 */
	status[0]  = SEPARATOR;
	status[1]  = SEPARATOR;
	status[2]  = SEPARATOR;
	status[3]  = SEPARATOR;
	status[4]  = (uint32_t)lsDmaBuffer;
	status[5]  = (uint32_t)lsLockLine;
	status[6]  = (uint32_t)lsSyncBuffer;
	status[7]  = (uint32_t)lsDebugBuffer;
	status[8]  = eaDmaBuffer;
	status[9]  = eaLockLine;
	status[10] = eaSyncBuffer;
	status[11] = eaDebugBuffer;

	gEaDebugBuffer = eaDebugBuffer;
	ret = checkStatus(SIZE_OF_STATUS, status);
	if (__builtin_expect(ret != 0, 0)) {
		/*E Error: Unexpected return value */
		notifyError((0x001 | UNEXPECTED_VALUE), 0, ret);
	}

	/**
	 *E Disable all SPU events.
	 */
	spu_writech(SPU_WrEventMask, MFC_NO_EVENT);
	spu_sync_c();
	if (__builtin_expect(spu_readchcnt(SPU_RdEventStat), 0)) {
		/*E Error: Any unexpected events had occured. */
		ch = spu_readch(SPU_RdEventStat);
		spu_writech(SPU_WrEventAck, ch);
		notifyError((0x002 | UNEXPECTED_EVENT), 0, ch);
	}


	for (uint32_t i = 0u; i < NUMBER_OF_ITERATION; i++) {

		/*E Wait for the PPU to initialize all the buffers. */
		do {
			mfc_getllar((volatile void *)lsSyncBuffer, (uint64_t)eaSyncBuffer, TID, RID);
			spu_readch(MFC_RdAtomicStat);
		} while (
			*(volatile uint32_t *)lsSyncBuffer != PPU_HAS_INITIALIZED_BUFFERS);

#if (PPU_TO_SPU == LLR_LOST_EVENT)

		/*E Clear the previous event caused by "syncBuffer". */
		spu_writech(SPU_WrEventMask, MFC_LLR_LOST_EVENT);
		spu_sync_c();
		if (__builtin_expect(spu_readchcnt(SPU_RdEventStat), 1)) {
			/*E Acknowledge all events occured. */
			ch = spu_readch(SPU_RdEventStat);
			spu_writech(SPU_WrEventAck, ch);
		}

		/*E Disable all events. */
		spu_writech(SPU_WrEventMask, MFC_LLR_LOST_EVENT);
		spu_sync_c();

		/*E Get the lock line and reserve it. */
		mfc_getllar((volatile void *)lsLockLine, (uint64_t)eaLockLine, TID, RID);
		ch = spu_readch(MFC_RdAtomicStat);
		if (__builtin_expect(ch != MFC_GETLLAR_STATUS, 0)) {
			/*E Error: Unexpected atomic command status */
			notifyError((0x003 | UNEXPECTED_ATOMIC_STATUS),
													MFC_GETLLAR_STATUS, ch);
		}

#endif /* PPU_TO_SPU */


		/**
		 *E Send data to the "syncBuffer".
		 *  And release the reservation from the buffer.
		 */
		*(volatile uint32_t *)lsSyncBuffer = SPU_IS_READY;
		mfc_putlluc((volatile void *)lsSyncBuffer, (uint64_t)eaSyncBuffer, TID, RID);
		ch = spu_readch(MFC_RdAtomicStat);
		if (__builtin_expect(ch != MFC_PUTLLUC_STATUS, 0)) {
			/*E Error: Unexpected atomic command status */
			notifyError((0x004 | UNEXPECTED_ATOMIC_STATUS),
													MFC_GETLLAR_STATUS, ch);
		}


		/**
		 *E    PPU  -->  SPU
		 *
		 *     Receive a value "PPU_TO_SPU_SYNC_VALUE" from the PPU.
		 */

#if (PPU_TO_SPU == LLR_LOST_EVENT)

		uint32_t mask = MFC_LLR_LOST_EVENT; /*E SPU event mask */

		do {

			/**
			 *E Set the SPU event mask to handle the lock line reservation lost
			 *  event.
			 */
			spu_writech(SPU_WrEventMask, mask);

			/**
			 *E Read the SPU Read Event Status Channel, and it stalls the SPU
			 *  until any enabled events have occurred.
			 * ("wait on event" function)
			 */
			ch = spu_readch(SPU_RdEventStat);
			if (__builtin_expect(ch != MFC_LLR_LOST_EVENT, 0)) {
				/*E Error: Any unexpected events have occured. */
				spu_writech(SPU_WrEventMask, MFC_NO_EVENT);
				spu_writech(SPU_WrEventAck, ch);
				notifyError((0x006 | UNEXPECTED_EVENT),
													MFC_LLR_LOST_EVENT, ch);
			}

			/*E Save the SPU event mask. */
			mask = spu_readch(SPU_RdEventMask);

			/**
			 *E Mask and acknowledge the lock line reservation lost event by
			 *  issuing writes the corresponding bits to the SPU Write Event
			 *  Mask Channel and the SPU Write Event Acknowledgement Channel.
			 */
			spu_writech(SPU_WrEventMask, MFC_NO_EVENT);
			spu_writech(SPU_WrEventAck, MFC_LLR_LOST_EVENT);

			/*E Get the lock line to check the value. */
			mfc_getllar((volatile void *)lsLockLine, (uint64_t)eaLockLine,
																	TID, RID);
			ch = spu_readch(MFC_RdAtomicStat);
			if (__builtin_expect(ch != MFC_GETLLAR_STATUS, 0)) {
				/*E Error: Unexpected atomic command status */
				notifyError((0x007 | UNEXPECTED_ATOMIC_STATUS),
													MFC_GETLLAR_STATUS, ch);
			}

			/*E Check the value on the lock line. */
		} while (__builtin_expect(
						*(uint32_t *)lsLockLine != PPU_TO_SPU_SYNC_VALUE, 0));

		/*E Release the lock line reservation. */
		mfc_putlluc((volatile void *)lsLockLine, (uint64_t)eaLockLine, TID, RID);
		ch = spu_readch(MFC_RdAtomicStat);
		if (__builtin_expect(ch != MFC_PUTLLUC_STATUS, 0)) {
			/*E Error: Unexpected atomic command status */
			notifyError((0x008 | UNEXPECTED_ATOMIC_STATUS),
													MFC_PUTLLUC_STATUS, ch);
		}

#elif (PPU_TO_SPU == GETLLAR_POLLING)

		/*E Poll a PPU's store for the lock line by "getllar". */
		do {
			mfc_getllar((volatile void *)lsLockLine, (uint64_t)eaLockLine, TID, RID);
			spu_readch(MFC_RdAtomicStat);
		} while (__builtin_expect(
				*(volatile uint32_t *)lsLockLine != PPU_TO_SPU_SYNC_VALUE, 0));

		/*E Release the reservation for the lock line. */
		mfc_putlluc((volatile void *)lsLockLine, (uint64_t)eaLockLine, TID, RID);
		ch = spu_readch(MFC_RdAtomicStat);
		if (__builtin_expect(ch != MFC_PUTLLUC_STATUS, 0)) {
			/*E Error: Unexpected atomic command status */
			notifyError((0x009 | UNEXPECTED_ATOMIC_STATUS),
													MFC_PUTLLUC_STATUS, ch);
		}

#elif (PPU_TO_SPU == SIGNAL_NOTIFICATION)

		/**
		 *E Read SPU Signal Notification 1 Channel, and it stalls the SPU until
		 *  a signal is issued.
		 */
		ch = spu_readch(SPU_RdSigNotify1);
		if (__builtin_expect(ch != PPU_TO_SPU_SYNC_VALUE, 0)) {
			/*E Error: Unexpected value to synchronize with the PPU */
			notifyError((0x00a | UNEXPECTED_VALUE), PPU_TO_SPU_SYNC_VALUE, ch);
		}

#else
#error : Unexpected Case of "PPU_TO_SPU".
#endif /* PPU_TO_SPU */


		/**
		 *E    PPU  <--  SPU
		 *
		 *     Send a value "SPU_TO_PPU_SYNC_VALUE" to the PPU.
		 */

#if (SPU_TO_PPU == DMA_PUT)

		/**
		 *E Write the value on the shared DMA buffer.
		 */
#if (CACHE_LINE_DMA)
		mfc_put((volatile void *)lsDmaBuffer, (uint64_t)eaDmaBuffer,
									CBE_CACHE_LINE, DMA_TAG, TID, RID);
#else
		mfc_put((volatile void *)lsDmaBuffer, (uint64_t)eaDmaBuffer,
									MFC_MIN_DMA_SIZE, DMA_TAG, TID, RID);
#endif /* CACHE_LINE_DMA */

		/*E Wait for the DMA tag group completion. */
		spu_writech(MFC_WrTagUpdate, 0);
		do {} while (spu_readchcnt(MFC_WrTagUpdate) == 0);
		spu_readch(MFC_RdTagStat);

		spu_writech(MFC_WrTagMask, DMA_TAG_MASK);
		do {} while (__builtin_expect(
					spu_mfcstat(MFC_TAG_UPDATE_IMMEDIATE) != DMA_TAG_MASK, 0));

#elif (SPU_TO_PPU == ATOMIC_PUTLLUC)

		/**
		 *E Write the value via the MFC atomic cache.
		 */
		mfc_putlluc((volatile void *)lsDmaBuffer, (uint64_t)eaDmaBuffer, TID, RID);

		/*E Wait for the completion of the atomic oparation. */
		ch = spu_readch(MFC_RdAtomicStat);
		if (__builtin_expect(ch != MFC_PUTLLUC_STATUS, 0)) {
			/*E Error: Unexpected atomic command status */
			notifyError((0x00b | UNEXPECTED_ATOMIC_STATUS),
													MFC_PUTLLUC_STATUS, ch);
		}

#elif (SPU_TO_PPU == EVENT_QUEUE_SEND)

		/**
		 *E Send the value via Cell OS Lv-2 event queue by "send_event".
		 */
		ret = sys_spu_thread_send_event(
							SPU_THREAD_PORT, NO_DATA, SPU_TO_PPU_SYNC_VALUE);
		if (__builtin_expect(ret != CELL_OK, 0)) {
			/*E Error: Unexpected return value */
			notifyError((0x00c | UNEXPECTED_VALUE), CELL_OK, ret);
		}

#elif (SPU_TO_PPU == EVENT_QUEUE_THROW)

		/**
		 *E Send the value via Cell OS Lv-2 event queue by "throw_event".
		 */
		ret = sys_spu_thread_throw_event(
							SPU_THREAD_PORT, NO_DATA, SPU_TO_PPU_SYNC_VALUE);
		if (__builtin_expect(ret != CELL_OK, 0)) {
			/*E Error: Unexpected return value */
			notifyError((0x00d | UNEXPECTED_VALUE), CELL_OK, ret);
		}

#else
#error : Unexpected Case of "SPU_TO_PPU".
#endif /* SPU_TO_PPU */


	} /* for - i (NUMBER_OF_ITERATION) */

	sys_spu_thread_exit(0);
}

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
