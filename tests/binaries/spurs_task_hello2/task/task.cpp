/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 360.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

/* common headers */
#include <stdint.h>
#include <stdlib.h>
#include <spu_intrinsics.h>
#include <cell/spurs.h>
#include <spu_printf.h>
#include <libsn_spu.h>
#include <cell/spurs/task.h>
#include <sys/spu_event.h>
#include "global.h"
#include "md5.h"


CELL_SPU_LS_PARAM(16*1024, 16*1024);


int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTask;
	(void)argTaskset;

	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char*)"abcde", 5);
	unsigned char digest[16] = { 0 };
	MD5Final(digest, &ctx);
	
	uint32_t multiplier = spu_extract((vec_uint4)argTask, 0);

	const int len = 99;
	char buf[len];
	for (int i = 0; i < len; ++i) {
		buf[i] = i;
	}
	
	int sum = 0;
	for (int i = 0; i < len; ++i) {
		sum += buf[i] * multiplier;
	}

	for (int i = 0; i < 16; ++i) {
		sum += digest[i];
	}

	return sum;
}

