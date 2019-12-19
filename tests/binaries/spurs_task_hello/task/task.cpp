/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

/* common headers */
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

	spu_printf("SPU: Hello world!\n");
	return 0;
}

