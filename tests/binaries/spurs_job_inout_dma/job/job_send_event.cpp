/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <cell/spurs/job_chain.h>
#include <cell/spurs/event_flag.h>
#include <spu_printf.h>

void cellSpursJobMain2(CellSpursJobContext2 *jobContext, CellSpursJob256 *job256)
{
	(void)jobContext;
	CellSpursJob128 *job = (CellSpursJob128 *)job256;
	uint64_t ea = job->workArea.userData[0];

	//spu_printf(" ****** event sender : ev=%llx\n", ea);
	cellSpursEventFlagSet(ea, 1);

	return;
}

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:ts=4:sw=4:
 */
