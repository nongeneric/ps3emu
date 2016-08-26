/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdint.h>
#include <stdlib.h>
#include <spu_intrinsics.h>
#include <cell/spurs.h>
#include <spu_printf.h>

CELL_SPU_LS_PARAM(16*1024, 16*1024);

int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTask;
	(void)argTaskset;
	int ret;

	spu_printf("SPU: Signal task start!\n");

	spu_printf("SPU: Waiting for a signal....\n");
	ret = cellSpursWaitSignal();
	if (ret) {
		spu_printf ("SPU: Receiving a signal failed.\n");
		cellSpursTaskExit(-1);
	}

	spu_printf("SPU: Receiving a signal succeeded.\n");
	
	return 0;
}

