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
#include <spu_mfcio.h>
#include <cell/dma.h>

CELL_SPU_LS_PARAM(16*1024, 16*1024);

volatile uint32_t line[0x80 / sizeof(uint32_t)] __attribute__((aligned(128)));

int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTask;
	(void)argTaskset;

	uint32_t res = spu_extract((vec_uint4)argTask, 0);

	for (int n = 0; n < 20; ++n){
		do {
			cellDmaGetllar(line, res, 0, 0);
			cellDmaWaitAtomicStatus();
			//for (int i = 0; i < 0x80 / sizeof(uint32_t); ++i) {
			//	line[i] += i + 1;
			//}
			line[0]++;
			cellDmaPutllc(line, res, 0, 0);
		} while (__builtin_expect(cellDmaWaitAtomicStatus(), 0));
	}

	return 0;
}

