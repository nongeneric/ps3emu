/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common
 *
 *   File: ppu/file.ppu.c
 *
 *   Description:
 *     Functions to read/write files on the host.
 */

#include <string.h>

/**
 *E  for myfprintf() functioin to output log data
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <cell/cell_fs.h>
#include <cell/sysmodule.h>
#include "ppu/buffer.h"
#include "ppu/statistics.h"
#define MYFPRINTF_BUFFER_SIZE	256
#define VERBOSE	1

int myfprintf(int fd, const char *format, ...);

/**
 *E  for creating a file path to read/write
 */
#include <sys/paths.h>		/* SYS_APP_HOME */
#include "association.h"
#include "ppu/file.h"

char gSpuSelfFilePath[FILE_PATH_SIZE];
char gLogFilePath[FILE_PATH_SIZE];

void appendPpuToSpuCommunicationMethodName(int ppuToSpu);
void appendSpuToPpuCommunicationMethodName(int spuToPpu);


int recordObservedValuesOnFile(void)
{
	return; /////////////////////////////////////////
	
	int fd;
	CellFsErrno err;
	int ret;

#if VERBOSE
	fprintf(stdout, "Writing the log file");
	fflush(stdout);
#endif

	/*E Load prx for libfs */
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
	if (ret != CELL_OK) {
		fprintf(stderr, "cellSysmoduleLoadModule() failed: 0x%08x\n", ret);
		return -1;
	}

	/*E Open the log file. */
	err = cellFsOpen(
				gLogFilePath,
				CELL_FS_O_RDWR | CELL_FS_O_CREAT | CELL_FS_O_TRUNC,
				&fd,
				NULL, 0);
	if (err != CELL_FS_SUCCEEDED) {
		fprintf(stderr, "cellFsOpen() failed: 0x%08x\n", err);
		return -2;
	}

	myfprintf(fd, "--- Statistics ---\n");
	myfprintf(fd, "itr : %u\n", NUMBER_OF_ITERATION);
	myfprintf(fd, "------------------\n");
	myfprintf(fd, "      cycles ( TimeBase )\n");
	myfprintf(fd, "min : %6d (0x%08x)\n",
							observed.min * CBE_TB_CLOCK_RATIO, observed.min);
	myfprintf(fd, "avg : %6d (0x%08x)\n",
							observed.avg * CBE_TB_CLOCK_RATIO, observed.avg);
	myfprintf(fd, "max : %6d (0x%08x)\n",
							observed.max * CBE_TB_CLOCK_RATIO, observed.max);
	myfprintf(fd, "------------------\n");
	myfprintf(fd, "min : %3.5lf (usec/trial)\n",
							calculateUsecPerTrial(observed.min));
	myfprintf(fd, "avg : %3.5lf (usec/trial)\n",
							calculateUsecPerTrial(observed.avg));
	myfprintf(fd, "max : %3.5lf (usec/trial)\n",
							calculateUsecPerTrial(observed.max));
	myfprintf(fd, "------------------\n");

	myfprintf(fd, ", index, usec, cycle, timebase,\n");

	for (uint32_t idx = 0u; idx < NUMBER_OF_ITERATION; idx++) {
		uint32_t val = observed.record[idx];
		myfprintf(fd, ", %6u, %3.5lf, %6u, 0x%08x\n",
					idx, calculateUsecPerTrial(val), val * CBE_TB_CLOCK_RATIO, val);
#if VERBOSE
		if ((idx % 500) == 499){
			fprintf(stdout, ".");
			fflush(stdout);
		}
#endif

	}

	myfprintf(fd, "---    Exit    ---\n");

	/*E Close the log file. */
	err = cellFsClose(fd);
	if (err != CELL_FS_SUCCEEDED) {
		fprintf(stderr, "cellFsClose() failed: 0x%08x\n", err);
		return -3;
	}

	/*E Unload prx for libfs */
	ret = cellSysmoduleUnloadModule(CELL_SYSMODULE_FS);
	if (ret != CELL_OK) {
		fprintf(stderr, "cellSysmoduleUnloadModule() failed: 0x%08x\n", ret);
		return -4;
	}

#if VERBOSE
	fprintf(stdout, " finished successfully.\n");
#endif

	return 0;
}

int myfprintf(int fd, const char *format, ...)
{
	char buffer[MYFPRINTF_BUFFER_SIZE];
	va_list argp;
	uint64_t nwrite = 0;
	CellFsErrno err;
	int length = 0;

	va_start(argp, format);
	length = vsnprintf(buffer, (size_t)MYFPRINTF_BUFFER_SIZE, format, argp);
	va_end(argp);
	if (__builtin_expect((length < 0) || (length >= MYFPRINTF_BUFFER_SIZE), 0))
	{
		fprintf(stderr, "vsnprintf() failed: 0x%08x, 0x%08x\n",
												length, MYFPRINTF_BUFFER_SIZE);
		return -1;
	}

	err = cellFsWrite(fd, (const void *)buffer, (uint64_t)length, &nwrite);
	if (err != CELL_FS_SUCCEEDED) {
		fprintf(stderr, "cellFsWrite() failed: 0x%08x\n", err);
		return -2;
	}
	if (__builtin_expect(
				(uint32_t)length != (uint32_t)(nwrite & 0xffffffffllu), 0)) {
		fprintf(stderr, "cellFsWrite() failed: 0x%08x, 0x%016llx\n",
												length, nwrite);
		return -3;
	}

	return length;
}

int makeFilePath(char *buffer, const char *fileName, const char *mountPoint)
{
	if ((strlen(fileName) + strlen(mountPoint)) > (FILE_PATH_SIZE - 2)) {
		return -1;
	}

    strcpy(buffer, mountPoint);
    strcat(buffer, "/");
    strcat(buffer, fileName);
    strcat(buffer, "\0");
	return strlen(buffer);
}

int makePpuSideRoundTripLogFilePath(
				const char *prefix,	uint32_t ppuToSpu, uint32_t spuToPpu)
{
	if (strlen(prefix) > FILE_PATH_SIZE - 64) {
		return -1;
	}

    strcpy(gLogFilePath, SYS_APP_HOME);
    strcat(gLogFilePath, "/");
    strcat(gLogFilePath, prefix);
	appendPpuToSpuCommunicationMethodName(ppuToSpu);
	appendSpuToPpuCommunicationMethodName(spuToPpu);
	strcat(gLogFilePath, ".csv\0");
	return strlen(gLogFilePath);
}

int makeSpuSideRoundTripLogFilePath(
				const char *prefix,	uint32_t ppuToSpu, uint32_t spuToPpu)
{
	if (strlen(prefix) > FILE_PATH_SIZE - 64) {
		return -1;
	}

    strcpy(gLogFilePath, SYS_APP_HOME);
    strcat(gLogFilePath, "/");
    strcat(gLogFilePath, prefix);
	appendSpuToPpuCommunicationMethodName(spuToPpu);
	appendPpuToSpuCommunicationMethodName(ppuToSpu);
	strcat(gLogFilePath, ".csv\0");
	return strlen(gLogFilePath);
}

void appendPpuToSpuCommunicationMethodName(int ppuToSpu)
{
	strcat(gLogFilePath, "_");

	switch (ppuToSpu) {
	case LLR_LOST_EVENT:
		strcat(gLogFilePath, "llrLost");
		break;
	case GETLLAR_POLLING:
		strcat(gLogFilePath, "Getllar");
		break;
	case SPU_INBOUND_MAILBOX:
		strcat(gLogFilePath, "InMbox");
		break;
	case SIGNAL_NOTIFICATION:
		strcat(gLogFilePath, "SNR");
		break;
	default:
		strcat(gLogFilePath, "???");
	}

	return;
}

void appendSpuToPpuCommunicationMethodName(int spuToPpu)
{
	strcat(gLogFilePath, "_");

	switch (spuToPpu) {
	case SPU_OUTBOUND_MAILBOX:
		strcat(gLogFilePath, "OutMbox");
		break;
	case SPU_OUTBOUND_INTERRUPT_MAILBOX:
		strcat(gLogFilePath, "OutIntrMbox");
		break;
	case SPU_OUTBOUND_INTERRUPT_MAILBOX_HANDLE:
		strcat(gLogFilePath, "OutIntrMboxHandle");
		break;
	case EVENT_QUEUE_SEND:
		strcat(gLogFilePath, "EventQueueSend");
		break;
	case EVENT_QUEUE_THROW:
		strcat(gLogFilePath, "EventQueueThrow");
		break;
	case DMA_PUT:
		strcat(gLogFilePath, "Put");
		break;
	case ATOMIC_PUTLLUC:
		strcat(gLogFilePath, "Putlluc");
		break;
	default:
		strcat(gLogFilePath, "???");
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
