/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/ppu-side/raw_spu
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
 *       5. Prepare and start a Raw SPU to measure the performance.
 *       6. Observe the performance by "observeRoundTripByRawSpu()".
 *       7. Cleanup the Raw SPU.
 *       8. Save the results.
 */

/* Standard headers */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* Cell OS Lv-2 headers */
#include <sys/process.h>		/*	SYS_PROCESS_PARAM */
#include <sys/paths.h>			/*	SYS_APP_HOME */
#include <sys/sys_time.h>		/*	sys_time_get_timebase_frequency() */
#include <sys/spu_initialize.h>	/*	sys_spu_initialize() */

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

#include "ppu/raw_spu.h"		/*	prepareMeasurementRawSpu(),
								 *	kickMeasurementRawSpu(),
								 *	cleanupMeasurementRawSpu()	*/

#include "ppu/file.h"			/*	gSpuSelfFilePath,
								 *	gLogFilePath,
								 *	makeFilePath(),
								 *	makePpuSideRoundTripLogFilePath(),
								 *	recordObservedValuesOnFile(),		*/

#define MAX_USABLE_SPU	6
#define MAX_RAW_SPU		1

#define SPU_SELF_FILE_NAME		"responder.spu.elf"
#define LOG_FILE_NAME_PREFIX	"PPU-side_Raw_SPU"

/*E The folloing function is defined in "observer.ppu.c" */
extern int observeRoundTripByRawSpu(void);

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
	ret = makePpuSideRoundTripLogFilePath(
								LOG_FILE_NAME_PREFIX, PPU_TO_SPU, SPU_TO_PPU);
	if (ret < 0) {
		fprintf(stderr, "makePpuSideRoundTripLogFilePath() failed: %d\n", ret);
		exit(2);
	}

	/*E Obtain the Time Base frequency of the system. */
	gTimeBaseFrequency = sys_time_get_timebase_frequency();

	/*E Initialize a buffer to observe the Time Base. */
	initializeObservedValues();
	printf("observed va: %08x\n", (uint32_t)&observed);

	/*E Print the configuration. */
	printf("-------------------------------\n");
	printf(" PPU-side Round Trip (Raw SPU) \n");
	printf("-------------------------------\n");
	showPpuToSpuCommunicationMethod(PPU_TO_SPU);
	printf("-------------------------------\n");
	showSpuToPpuCommunicationMethod(SPU_TO_PPU);
	printf("-------------------------------\n");
	printf("SPU self: %s\n",gSpuSelfFilePath);
	printf("log file: %s\n",gLogFilePath);
	printf("-------------------------------\n");
	printf("eaDmaBuffer   : 0x%08x\n", (uintptr_t)dmaBuffer);
	printf("eaLockLine    : 0x%08x\n", (uintptr_t)lockLine);
	printf("eaDebugBuffer : 0x%08x\n", (uintptr_t)debugBuffer);
	printf("-------------------------------\n");
	printf("Time Base Frequency = %llu Hz\n", gTimeBaseFrequency);
	printf("-------------------------------\n");


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

	/*E Create a Raw SPU to measure the performance. */
	ret = prepareMeasurementRawSpu();
	if (ret) {
		fprintf(stderr, "prepareMeasurementRawSpu() failed: %d\n", ret);
		exit(4);
	}

	/*E Kick the Raw SPU. */
	kickMeasurementRawSpu();


	/*E Start to observe the performance of the round trip. */
	ret = observeRoundTripByRawSpu();
	if (ret) {
		fprintf(stderr, "observeRoundTripByRawSpu() failed: %d\n", ret);
		cleanupMeasurementRawSpu();
		exit(5);
	}


	/*E Cleanup the Raw SPU. */
	ret = cleanupMeasurementRawSpu();
	if (ret) {
		fprintf(stderr, "cleanupMeasurementRawSpu() failed: %d\n", ret);
		exit(6);
	}


	/*E Analyze and record the observed values. */
	analyzeObservedValues();
	ret = recordObservedValuesOnFile();
	if (ret) {
		fprintf(stderr, "recordObservedValuesOnFile() failed: %d\n", ret);
		exit(7);
	}

	printf(" \"PPU-side Round Trip (Raw SPU)\" has finihsed successfully.\n");
	return 0;
}

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
