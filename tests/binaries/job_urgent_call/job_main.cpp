/* SCE CONFIDENTIAL
   PlayStation(R)3 Programmer Tool Runtime Library 475.001
   * Copyright (C) 2007 Sony Computer Entertainment Inc.
   * All Rights Reserved.
   */

#include <cell/spurs/job_chain.h>
#include <spu_printf.h>

void cellSpursJobMain2(CellSpursJobContext2* stInfo, CellSpursJob256 *job)
{
	(void)stInfo;
	(void)job;
	for (int i = 0; i < 4000; i += 2) {
		i -= 1;
	}
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */

	
