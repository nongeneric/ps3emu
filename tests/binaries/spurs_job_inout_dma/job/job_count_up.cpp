/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <cell/spurs/job_chain.h>
#include <stdint.h>
#include <spu_printf.h>
#include <cell/dma.h>
#include "libsn_spu.h"

/*  counter variable */
typedef struct {
	uint64_t n;
	uint64_t dummy;
} __attribute__((aligned(16))) Counter;

void cellSpursJobMain2(CellSpursJobContext2 *jobContext, CellSpursJob256 *job256)
{
	
	/*
	 *  Obtain the effective address of a shared counter valiable (streamCounter)
	 */
	CellSpursJob128 *job = (CellSpursJob128 *)job256;
	uint64_t eaDst = job->workArea.userData[1];

	Counter* counter = (Counter*)jobContext->ioBuffer;
	counter->n++;

	cellDmaPut(counter, eaDst, sizeof(Counter), jobContext->dmaTag, 0,0);
}

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:ts=4:sw=4:
 */
