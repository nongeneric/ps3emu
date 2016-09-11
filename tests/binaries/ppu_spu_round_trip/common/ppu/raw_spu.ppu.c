/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common
 *
 *   File: ppu/raw_spu.ppu.c
 *
 *   Description:
 *     Settings of a Raw SPU to measure the performance
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <ppu_intrinsics.h>
#include <sys/raw_spu.h>
#include <sys/spu_utility.h>	/* sys_raw_spu_load */

#include "ppu/raw_spu.h"
#include "ppu/file.h"

/* SPU Run Control Register (SPU_RunCntl) */
#define SPU_RUN_REQUEST				(0x1)

/* SPU Configuration Register (SPU_Cfg) */
#define SNR1_LOGICALOR_MASK			(0x1)
#define SNR2_LOGICALOR_MASK			(0x2)

/* Class 0 Interrupt Status Register (INT_Stat_class0) */
#define SPU_INTR_CLASS_0_RESET_FLAG	(0x7ULL)

/* Class 2 Interrupt Status Register (INT_Stat_class2) */
#define SPU_INTR_CLASS_2_RESET_FLAG	(0x1FULL)

sys_raw_spu_t gRawSpuId;
uint32_t rawSpuEntry;


/**
 *E  Creation of a Raw SPU to measure performance
 */
int prepareMeasurementRawSpu(void)
{
	int ret = 0;

	/*E Create a Raw SPU. */
	ret = sys_raw_spu_create(&gRawSpuId, NULL);
	if (ret) {
		fprintf(stderr, "sys_raw_spu_create() failed: 0x%08x\n", ret);
		return -1;
	}

#if 0
	/*E Set Signal Notification 1/2 Registers to the logical OR mode. */
	ret = sys_raw_spu_set_spu_cfg(gRawSpuId,
								(SNR1_LOGICALOR_MASK & SNR2_LOGICALOR_MASK));
	if (ret) {
		fprintf(stderr, "sys_raw_spu_set_spu_cfg() failed: 0x%08x\n", ret);
		return -2;
	}
#endif

	/**
	 *E Reset all pending interrupts before starting.
	 *  XXX: This is a workaround for SDK0.5.0. Interrupt Status Registers
	 *  should be reset when a new Raw SPU is created.
	 */
	ret = sys_raw_spu_set_int_stat(
						gRawSpuId,
						SPU_INTR_CLASS_2,
						SPU_INTR_CLASS_2_RESET_FLAG);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_raw_spu_set_int_stat() failed: 0x%08x\n", ret);
		return -3;
	}

	ret = sys_raw_spu_set_int_stat(
						gRawSpuId,
						SPU_INTR_CLASS_0,
						SPU_INTR_CLASS_0_RESET_FLAG);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_raw_spu_set_int_stat() failed: 0x%08x\n", ret);
		return -4;
	}

	/**
	 *E  Loading an SPU program to the Raw SPU.
	 *
	 * Note: sys_raw_spu_load() may be deprecated in future, and the user will
	 *       need to load a program himself by the following sequence.
	 *       1. Open the ELF file. (either mapping to the address space or 
	 *          loading in the main memory.)
	 *       2. Parse and obtain the entry point.
	 *       3. Load the opend ELF file to the Raw SPU by DMA.
	 */
	 printf("loading %s\n", gSpuSelfFilePath);
	ret = sys_raw_spu_load(gRawSpuId, gSpuSelfFilePath, &rawSpuEntry);
	if (ret) {
		fprintf(stderr, "sys_raw_spu_load() failed: 0x%08x\n", ret);
		cleanupMeasurementRawSpu();
		return -5;
	}

	return 0;
}

void kickMeasurementRawSpu(void)
{
	/*E Set the entry point address on SPU_NPC register. */
	sys_raw_spu_mmio_write(gRawSpuId, SPU_NPC, rawSpuEntry);

	/*E Issue a SPU run request on SPU_RunCntl register. */
	sys_raw_spu_mmio_write(gRawSpuId, SPU_RunCntl, SPU_RUN_REQUEST);

	__eieio();
	return;
}

int cleanupMeasurementRawSpu(void)
{
	int ret = 0;

	ret = sys_raw_spu_destroy(gRawSpuId);
	if (ret) {
		fprintf(stderr, "sys_raw_spu_destroy() failed: 0x%08x\n", ret);
		return -1;
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
