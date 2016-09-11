/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common
 *
 *   File: ppu/interrupt_thread.ppu.c
 *
 *   Description:
 *     Settings of an interrupt PPU thread
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ppu/raw_spu.h"

#include <sys/ppu_thread.h>
#include <sys/interrupt.h>

#define INTERRUPT_PPU_THREAD_PRIORITY	1
#define PPU_THREAD_STACK_SIZE			(0x4000)

sys_interrupt_thread_handle_t intrHandle;
sys_interrupt_tag_t intrTag;


/**
 *E  Creation of an interrupt PPU thread to handle mailbox interrupts
 */
int prepareInterruptPpuThread(void (*entry)(uint64_t))
{
	sys_ppu_thread_t ppuThreadId;
	int ret = 0;

	/*E Create an interrupt PPU thread. */
	ret = sys_ppu_thread_create(
				&ppuThreadId,
				entry,	/*E Pointer of the entry function */
				0,		/*E Argument of the entry function */
				INTERRUPT_PPU_THREAD_PRIORITY,
				PPU_THREAD_STACK_SIZE,
				SYS_PPU_THREAD_CREATE_INTERRUPT,	/*E Interrupt PPU Thread */
				"Interrupt PPU Thread");
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_ppu_thread_create() failed: 0x%08x\n", ret);
		return -1;
	}

	/*E Create an interrupt tag. */
	ret = sys_raw_spu_create_interrupt_tag(
				gRawSpuId,
				SPU_INTR_CLASS_2,
				SYS_HW_THREAD_ANY,
				&intrTag);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_raw_spu_create_interrupt_tag() failed: 0x%08x\n",
																		ret);
		return -2;
	}

	/*E Establish an interrupt tag on the interrupt PPU thread. */
	ret = sys_interrupt_thread_establish(
				&intrHandle,
				intrTag,
				ppuThreadId,
				gRawSpuId);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_intrrupt_thread_establish() failed: 0x%08x\n",
																		ret);
		sys_interrupt_tag_destroy(intrTag);
		return -3;
	}

	/**
	 *E Set the class 2 interrupt mask to handle the SPU Outbound Mailbox
	 *  interrupts.
	 */
	ret = sys_raw_spu_set_int_mask(
				gRawSpuId,
				SPU_INTR_CLASS_2,
				OUT_INTR_MBOX_MASK);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_raw_spu_set_int_mask() failed: 0x%08x\n", ret);
		sys_interrupt_tag_destroy(intrTag);
		sys_interrupt_thread_disestablish(intrHandle);
		return -4;
	}

	return 0;
}

int cleanupInterruptPpuThread(void)
{
	int ret = 0;

	ret = sys_interrupt_thread_disestablish(intrHandle);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_interrupt_thread_disestablish() failed: 0x%08x\n",
																		ret);
		return -1;
	}

	ret = sys_interrupt_tag_destroy(intrTag);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_interrupt_tag_destroy() failed: 0x%08x\n", ret);
		return -2;
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
