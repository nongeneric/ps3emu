/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common
 *
 *   File: ppu/spu_thread.ppu.c
 *
 *   Description:
 *     Settings of a SPU thread to measure the performance
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "ppu/spu_thread.h"
#include <sys/spu_image.h>

#include "ppu/buffer.h"
#include "ppu/file.h"

/**
 *E  SPU thread Settings
 */

/* sys_spu_thread_group_{create/destroy} */
#define SPU_THREAD_GROUP_PRIORITY	100
#define NUM_SPU_THREADS	1
#define SPU_NUM			0
sys_spu_thread_group_t gSpuThreadGroupId;

/* sys_spu_image_{open/close} */
sys_spu_image_t spuImage;

/* sys_spu_thread_{connect/disconnect}_event */
sys_spu_thread_t gSpuThreadId;

/* sys_event_queue_{create/destroy} */
#define EVENT_QUEUE_DEPTH	1
sys_event_queue_t gEventQueueId;

/* SPU Configuration Register (SPU_Cfg) */
#define SNR1_LOGICALOR_MASK	(0x1)
#define SNR2_LOGICALOR_MASK	(0x2)


int prepareMeasurementSpuThread(void)
{
	/*E SPU thread group */
	const char *spuThreadGroupName = "Measurement SPU Thread Group";
	sys_spu_thread_group_attribute_t spuThreadGroupAttribute;

	/*E SPU thread */
	const char *spuThreadName = "Measurement SPU Thread";
	sys_spu_thread_attribute_t spuThreadAttribute;
	sys_spu_thread_argument_t spuThreadArgs;

	/*E Event queue */
	sys_event_queue_attribute_t eventQueueAttribute;

	int ret;

	/**
	 *E Create a SPU thread group.
	 */
	spuThreadGroupAttribute.name  = spuThreadGroupName;
	spuThreadGroupAttribute.nsize = strlen(spuThreadGroupAttribute.name) + 1;
	spuThreadGroupAttribute.type  = SYS_SPU_THREAD_GROUP_TYPE_NORMAL;

	ret = sys_spu_thread_group_create(
							&gSpuThreadGroupId,
							NUM_SPU_THREADS,
							SPU_THREAD_GROUP_PRIORITY,
							&spuThreadGroupAttribute);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_create() failed: 0x%08x\n", ret);
		return -1;
	}
	
	/**
	 *E Load a SPU ELF image.
	 */
	ret = sys_spu_image_open(&spuImage, gSpuSelfFilePath);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_image_open() failed: 0x%08x\n", ret);
		sys_spu_thread_group_destroy(gSpuThreadGroupId);
		return -2;
	}

	/**
	 *E Initialize a SPU thread that belongs to the SPU thread group.
	 */
	spuThreadAttribute.name   = spuThreadName;
	spuThreadAttribute.nsize  = strlen(spuThreadName) + 1;
	spuThreadAttribute.option = SYS_SPU_THREAD_OPTION_NONE;

	spuThreadArgs.arg1 = SYS_SPU_THREAD_ARGUMENT_LET_32((uint32_t)dmaBuffer);
	spuThreadArgs.arg2 = SYS_SPU_THREAD_ARGUMENT_LET_32((uint32_t)lockLine);
	spuThreadArgs.arg3 = SYS_SPU_THREAD_ARGUMENT_LET_32((uint32_t)syncBuffer);
	spuThreadArgs.arg4 = SYS_SPU_THREAD_ARGUMENT_LET_32((uint32_t)debugBuffer);

	ret = sys_spu_thread_initialize(
							&gSpuThreadId,
							gSpuThreadGroupId,
							SPU_NUM,
							&spuImage,
							&spuThreadAttribute,
							&spuThreadArgs);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_initialize() failed: 0x%08x\n", ret);
		sys_spu_thread_group_destroy(gSpuThreadGroupId);
		sys_spu_image_close(&spuImage);
		return -3;
	}

#if 0
	/*E Set Signal Notification 1/2 Registers to the logical OR mode. */
	ret = sys_spu_thread_set_spu_cfg(gSpuThreadId,
								(SNR1_LOGICALOR_MASK & SNR2_LOGICALOR_MASK));
	if (ret) {
		fprintf(stderr, "sys_spu_thread_set_spu_cfg() failed: 0x%08x\n", ret);
		return -4;
	}
#endif

	/**
	 *E Initialize an event queue to receive the SPU thread user event.
	 */
	eventQueueAttribute.attr_protocol = SYS_SYNC_FIFO;
	eventQueueAttribute.type = SYS_PPU_QUEUE;
	ret = sys_event_queue_create(
							&gEventQueueId,
							&eventQueueAttribute,
							SYS_EVENT_QUEUE_LOCAL,
							EVENT_QUEUE_DEPTH);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_queue_create() failed: 0x%08x\n", ret);
		sys_spu_thread_group_destroy(gSpuThreadGroupId);
		sys_spu_image_close(&spuImage);
		return -5;
	}

	/**
	 *E Connect the SPU thread to the event queue for the SPU thread user
	 *  event.
	 */
	ret = sys_spu_thread_connect_event(
							gSpuThreadId,
							gEventQueueId,
							SYS_SPU_THREAD_EVENT_USER,
							SPU_THREAD_PORT);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_connect_event() failed: 0x%08x\n", ret);
		sys_event_queue_destroy(gEventQueueId, 0);
		sys_spu_thread_group_destroy(gSpuThreadGroupId);
		sys_spu_image_close(&spuImage);
		return -6;
	}

	return 0;
}

int cleanupMeasurementSpuThread(void)
{
	int ret;

	/**
	 *E Destroy the SPU thread group and clean up resources.
	 */

	ret = sys_spu_thread_disconnect_event(
							gSpuThreadId,
							SYS_SPU_THREAD_EVENT_USER,
							SPU_THREAD_PORT);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_disconnect_event() failed: 0x%08x\n", ret);
		return -1;
	}

	ret = sys_event_queue_destroy(gEventQueueId, 0);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_queue_destroy() failed: 0x%08x\n", ret);
		return -2;
	}

	ret = sys_spu_thread_group_destroy(gSpuThreadGroupId);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_destroy() failed: 0x%08x\n", ret);
		return -3;
	}

	ret = sys_spu_image_close(&spuImage);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_queue_destroy() failed: 0x%08x\n", ret);
		return -4;
	}

	return 0;
}


/**
 *E  The exit cause and status of the SPU thread group
 */

int gSpuThreadGroupExitCause;
int gSpuThreadGroupExitStatus;

void showSpuThreadGroupExitCauseAndStatus(void)
{
	int spuThreadExitStatus;
	int ret;

	switch(gSpuThreadGroupExitCause) {
	case SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT:
		printf("The SPU thread group exited by sys_spu_thread_group_exit().\n");
		printf("The group's exit status = %d\n", gSpuThreadGroupExitStatus);
		break;
	case SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT:
		printf("All SPU thread exited by sys_spu_thread_exit().\n");
		ret = sys_spu_thread_get_exit_status(gSpuThreadId, &spuThreadExitStatus);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_get_exit_status() failed: 0x%08x\n", ret);
		}
		printf("SPU thread %d's exit status = %d\n", SPU_NUM, spuThreadExitStatus);
		break;
	case SYS_SPU_THREAD_GROUP_JOIN_TERMINATED:
		printf("The SPU thread group is terminated by sys_spu_thread_terminate().\n");
		printf("The group's exit status = %d\n", gSpuThreadGroupExitStatus);
		break;
	default:
		fprintf(stderr, "Unknown exit cause: 0x%08x\n", gSpuThreadGroupExitCause);
		break;
	}
	
	return;
}

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
