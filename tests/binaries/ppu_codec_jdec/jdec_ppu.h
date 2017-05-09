/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef __JDECPPU_H__
#define __JDECPPU_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/paths.h>
#include <sys/timer.h>

/*E This header is needed by JPEG Decoder. */
#include <cell/codec/jpgdec.h>

/*E include for prx */
#include <cell/sysmodule.h>
#include <sys/process.h>

/*E frame max size. */
#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720
#define DISPLAY_WIDTH FRAME_WIDTH
#define DISPLAY_HEIGHT FRAME_HEIGHT

/*E The name of the file that to decipher it is defined. */
#define INPUT_STREAM_NAME			SYS_APP_HOME "/SampleStream.jpg"

#define EMSG0(x, ...)	printf(x, __func__, ##__VA_ARGS__)
#define EMSG(...)		EMSG0("* %s: ERROR! " __VA_ARGS__)
#define EINFO(x)		EMSG("exit. (ret=0x%08X, file=%s:%d)\n", (unsigned int)x, __FILE__, __LINE__)
#define DP0(x, ...)		printf(x, __func__, ##__VA_ARGS__)
#define DP(...)			DP0("* %s: " __VA_ARGS__)

/*E Set the argument for malloc callback function. */
typedef struct {
	uint32_t mallocCallCounts;
	uint32_t freeCallCounts;
} CtrlCbArg;

/*E JPEG Decoder needs those parameters. */
typedef struct{
	CellJpgDecMainHandle		mainHandle;
	CellJpgDecSubHandle			subHandle;
	CtrlCbArg					ctrlCbArg;
}SJpgDecCtlInfo;

#endif /* __JDECPPU_H__ */
