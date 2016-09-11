/*   SCE CONFIDENTIAL                                         */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*   All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/ppu-side/spu_thread
 *
 *   File: association.h
 *
 *   Description:
 *     Common settings to the PPU and the SPU
 */

#ifndef __SAMPLE_ROUND_TRIP_ASSOCIATION_H__
#define __SAMPLE_ROUND_TRIP_ASSOCIATION_H__

#include <stdint.h>

#define NUMBER_OF_ITERATION		(400u)

/**
 *E  DMA Size and Cache Settings
 */
#define CACHE_LINE_DMA		(1)
#define ENABLE_CACHE_FLUSH	(0)

/**
 *E  Size of CBE's cache line and cache block (128 bytes)
 */
#define CBE_CACHE_LINE		(128)
#define __ALIGNED_CBE_CACHE_LINE__	__attribute__((aligned(CBE_CACHE_LINE)))

/**
 *E  PPU's Query/Reply Settings (PPU --> SPU)
 */
#define LLR_LOST_EVENT		(0x1)
#define GETLLAR_POLLING		(0x2)
#define SPU_INBOUND_MAILBOX	(0x3)	/*E Raw SPU */
#define SIGNAL_NOTIFICATION	(0x4)

#ifndef PPU_TO_SPU
	#define PPU_TO_SPU		GETLLAR_POLLING
#endif /* PPU_TO_SPU */

/**
 *E  SPU's Query/Reply Settings (PPU <-- SPU)
 */
#define SPU_OUTBOUND_MAILBOX					(0x1)	/*E Raw SPU */
#define SPU_OUTBOUND_INTERRUPT_MAILBOX			(0x2)	/*E Raw SPU */
#define SPU_OUTBOUND_INTERRUPT_MAILBOX_HANDLE	(0x3)	/*E Raw SPU */
#define EVENT_QUEUE_SEND						(0x4)	/*E SPU thread */
#define EVENT_QUEUE_THROW						(0x5)	/*E SPU thread */
#define DMA_PUT									(0x6)
#define ATOMIC_PUTLLUC							(0x7)

#ifndef SPU_TO_PPU
	#define SPU_TO_PPU		ATOMIC_PUTLLUC
#endif /* PPU_TO_SPU */

/**
 *E  SPU thread event port number
 */
#define SPU_THREAD_PORT		(58)

/**
 *E  Macros for Synchronization (32bit) 
 */
#define PPU_TO_SPU_SYNC_VALUE			(0x80001000U)
#define SPU_TO_PPU_SYNC_VALUE			(0x80001001U)

#define PPU_HAS_INITIALIZED_LOCK_LINE	(0x80002000U)	/*E PPU-side round trip */
#define PPU_HAS_INITIALIZED_BUFFERS		(0x80002001U)	/*E PPU-side round trip */
#define SPU_IS_READY					(0x80002002U)	/*E PPU-side round trip */
#define PPU_IS_READY					(0x80002003U)	/*E SPU-side round trip */
#define RAW_SPU_HAS_FINISHED_EXECUTION	(0x80002004U)	/*E Raw SPU */

/**
 *E  Initial Values
 */
#define INIT_MEMSET_VALUE	(0xEE)			/*  8bit */
#define INIT_LOCK32_VALUE	(0xC0000001U)	/* 32bit */

/**
 *E  Data Type for the Shared Buffer
 *   (Size of the type is just 16 bytes)
 */
typedef struct SharedBufferAddress {
	uint32_t dma;
	uint32_t lock;
	uint32_t debug;
	uint32_t _pad_;
} SharedBufferAddress __ALIGNED_CBE_CACHE_LINE__;

#endif /* __SAMPLE_ROUND_TRIP_ASSOCIATION_H__ */

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
