/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2009 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <spu_printf.h>
#include <cell/spurs/job_queue.h>

void cellSpursJobQueueMain(CellSpursJobContext2 *pContext, CellSpursJob256 *pJob)
{
	(void)pJob; (void)pContext;

	spu_printf("Hello, jobqueue!\n");
}
