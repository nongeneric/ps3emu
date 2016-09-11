/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common
 *
 *   File: ppu/raw_spu.h
 *
 *   Description:
 *     - Settings of a Raw SPU to measure the performance
 *     - Settings of an interrupt PPU thread
 */

#ifndef __SAMPLE_ROUND_TRIP_PPU_RAW_SPU_H__
#define __SAMPLE_ROUND_TRIP_PPU_RAW_SPU_H__

#include <sys/raw_spu.h>

extern sys_raw_spu_t gRawSpuId;

/* SPU Mailbox Status Register (SPU_Mbox_Stat) */
#define SPU_OUT_INTR_MBOX_COUNT_SHIFT	(16)
#define SPU_OUT_INTR_MBOX_COUNT			(0xFF << SPU_OUT_INTR_MBOX_COUNT_SHIFT)
#define SPU_IN_MBOX_COUNT_SHIFT			(8)
#define SPU_IN_MBOX_COUNT				(0xFF << SPU_IN_MBOX_COUNT_SHIFT)
#define SPU_OUT_MBOX_COUNT_SHIFT		(0)
#define SPU_OUT_MBOX_COUNT				(0xFF << SPU_OUT_MBOX_COUNT_SHIFT)

/*E The interrupt class number */
#define SPU_INTR_CLASS_2	2
#define SPU_INTR_CLASS_0	0

/*E Class 2 Interrupt Mask Register (INT_Mask_class2) */
#define OUT_INTR_MBOX_SHIFT	0
#define OUT_INTR_MBOX_MASK	(0x1 << OUT_INTR_MBOX_SHIFT)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*E Raw SPU */
int prepareMeasurementRawSpu(void);
void kickMeasurementRawSpu(void);
int cleanupMeasurementRawSpu(void);

/*E Interrupt PPU thread */
int prepareInterruptPpuThread(void (*entry)(uint64_t));
int cleanupInterruptPpuThread(void);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __SAMPLE_ROUND_TRIP_PPU_RAW_SPU_H__ */

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
