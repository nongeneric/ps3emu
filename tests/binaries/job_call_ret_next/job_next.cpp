/* SCE CONFIDENTIAL
   PlayStation(R)3 Programmer Tool Runtime Library 475.001
   * Copyright (C) 2007 Sony Computer Entertainment Inc.
   * All Rights Reserved.
   */

#include <cell/spurs.h>
#include <spu_printf.h>

void cellSpursJobMain2(CellSpursJobContext2* stInfo, CellSpursJob256 *job)
{
	(void)stInfo;
	spu_printf("job next\n");
	uint64_t ea = job->workArea.userData[0];
	/* E notification of the end of job chain */
	cell::Spurs::EventFlagStub ev;
	ev.setObject(ea);
	ev.set(1);
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */

	
