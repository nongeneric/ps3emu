/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/spu-side/spu_thread
 *
 *   File: main.ppu.c
 *
 *   Description:
 *     This file for a PPU program contains an entry function.
 *     The sequence is as follows.
 *
 *       1. Make filepaths for the SPU self and the log file.
 *       2. Obtain the Time Base frequency of the system.
 *       3. Initialize a buffer to observe the Time Base.
 *       4. Print the configuration.
 *       5. Prepare and start a SPU thread to measure the performance.
 *       6. Respond to the SPU thread by "respondRoundTripBySpuThread()".
 *       7. Wait for and cleanup the SPU thread.
 *       8. Save the results.
 */

/* Standard headers */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* Cell OS Lv-2 headers */
#include <sys/process.h>			/*	SYS_PROCESS_PARAM */
#include <sys/paths.h>				/*	SYS_APP_HOME */
#include <sys/sys_time.h>			/*	sys_time_get_timebase_frequency() */
#include <sys/spu_initialize.h>		/*	sys_spu_initialize() */
#include <sys/spu_thread_group.h>	/*	sys_spu_thread_group_start(),
									 *	sys_spu_thread_group_join()	*/

/* sample headers */
#include "association.h"		/*	PPU_TO_SPU,
								 *	SPU_TO_PPU	*/

#include "ppu/buffer.h"			/*	dmaBuffer,
								 *	lockLine,
								 *	debugBuffer,
								 *	observed	*/

#include "ppu/statistics.h"		/*	gTimeBaseFrequency,
								 *	initializeObservedValues(),
								 *	analyzeObservedValues(),
								 *	showPpuToSpuCommunicationMethod(),
								 *	showSpuToPpuCommunicationMethod()	*/

#include "ppu/spu_thread.h"		/*	gSpuThreadGroupId,
								 *	gSpuThreadGroupExitCause,
								 *	gSpuThreadGroupExitStatus,
								 *	prepareMeasurementSpuThread(),
								 *	cleanupMeasurementSpuThread(),
								 *	showSpuThreadGroupExitCauseAndStatus()	*/

#include "ppu/file.h"			/*	gSpuSelfFilePath,
								 *	gLogFilePath,
								 *	makeFilePath(),
								 *	makeSpuSideRoundTripLogFilePath(),
								 *	recordObservedValuesOnFile(),		*/

#define MAX_USABLE_SPU	6
#define MAX_RAW_SPU		0

#define SPU_SELF_FILE_NAME		"observer.spu.elf"
#define LOG_FILE_NAME_PREFIX	"SPU-side_SPU_thread"

/*E The folloing function is defined in "responder.ppu.c" */
extern int respondToRoundTripBySpuThread(void);

SYS_PROCESS_PARAM(1001, 0x10000)

int main(void)
{
	int ret;

	/*E Make a filepath for the SPU self file. */
	ret = makeFilePath(gSpuSelfFilePath, SPU_SELF_FILE_NAME, SYS_APP_HOME);
	if (ret < 0) {
		fprintf(stderr, "makeFilePath() failed: %d\n", ret);
		exit(1);
	}

	/*E Make a filepath for the log file. */
	ret = makeSpuSideRoundTripLogFilePath(
								LOG_FILE_NAME_PREFIX, PPU_TO_SPU, SPU_TO_PPU);
	if (ret < 0) {
		fprintf(stderr, "makeSpuSideRoundTripLogFilePath() failed: %d\n", ret);
		exit(2);
	}

	/*E Obtain the Time Base frequency of the system. */
	gTimeBaseFrequency = sys_time_get_timebase_frequency();

	/*E Initialize a buffer to observe the Time Base. */
	initializeObservedValues();

	/*E Print the configuration. */
	printf("----------------------------------\n");
	printf(" SPU-side Round Trip (SPU thread) \n");
	printf("----------------------------------\n");
	showSpuToPpuCommunicationMethod(SPU_TO_PPU);
	printf("----------------------------------\n");
	showPpuToSpuCommunicationMethod(PPU_TO_SPU);
	printf("----------------------------------\n");
	printf("SPU self: %s\n",gSpuSelfFilePath);
	printf("log file: %s\n",gLogFilePath);
	printf("----------------------------------\n");
	printf("eaDmaBuffer   : 0x%08x\n", (uintptr_t)dmaBuffer);
	printf("eaLockLine    : 0x%08x\n", (uintptr_t)lockLine);
	printf("eaSyncBuffer  : 0x%08x\n", (uintptr_t)syncBuffer);
	printf("eaDebugBuffer : 0x%08x\n", (uintptr_t)debugBuffer);
	printf("----------------------------------\n");
	printf("Time Base Frequency = %llu Hz\n", gTimeBaseFrequency);
	printf("----------------------------------\n");


	/*E Initialize SPUs. */
	ret = sys_spu_initialize(MAX_USABLE_SPU, MAX_RAW_SPU);
	switch (ret) {
	case CELL_OK:
		break;
	case EBUSY:
		printf("SPUs have already been initialized.\n");
		printf("...but continue!\n");
		break;
	default:
		fprintf(stderr, "sys_spu_initialize(%d, %d) failed: 0x%08x\n",
											MAX_USABLE_SPU, MAX_RAW_SPU, ret);
		exit(3);
	}

	/*E Create a SPU thread to measure the performance. */
	ret = prepareMeasurementSpuThread();
	if (ret) {
		fprintf(stderr, "prepareMeasurementSpuThread() failed: %d\n", ret);
		exit(4);
	}

	/*E Start the SPU thread group. */
	ret = sys_spu_thread_group_start(gSpuThreadGroupId);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_start() failed: 0x%08x\n", ret);
		exit(5);
	}


	/**
	 *E Start to respond to a SPU thread to take the performance of the round
	 *  trip.
	 */
	ret = respondToRoundTripBySpuThread();
	if (ret) {
		fprintf(stderr, "respondToRoundTripBySpuThread() failed: %d\n", ret);
		cleanupMeasurementSpuThread();
		exit(6);
	}


	/*E Wait for the termination of the SPU thread group. */
	ret = sys_spu_thread_group_join(
							gSpuThreadGroupId,
							&gSpuThreadGroupExitCause,
							&gSpuThreadGroupExitStatus);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_join() failed: 0x%08x\n", ret);
		cleanupMeasurementSpuThread();
		exit(7);
	}

	/*E Show the exit cause and status. */
	showSpuThreadGroupExitCauseAndStatus();

	/*E Cleanup the SPU thread group. */
	ret = cleanupMeasurementSpuThread();
	if (ret) {
		fprintf(stderr, "cleanupMeasurementSpuThread() failed: %d\n", ret);
		exit(8);
	}


	/*E Analyze and record the observed values. */
	analyzeObservedValues();
	ret = recordObservedValuesOnFile();
	if (ret) {
		fprintf(stderr, "recordObservedValuesOnFile() failed: %d\n", ret);
		exit(9);
	}

	printf(" \"SPU-side Round Trip (SPU thread)\" has finihsed successfully.\n");
	return 0;
}

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
