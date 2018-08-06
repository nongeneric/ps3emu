/* SCE CONFIDENTIAL
   PlayStation(R)3 Programmer Tool Runtime Library 475.001
   * Copyright (C) 2007 Sony Computer Entertainment Inc.
   * All Rights Reserved.
   */

#include <cell/spurs/job_chain.h>
#include <cell/spurs/event_flag.h>
#include <cell/atomic.h>
#include <spu_printf.h>

void cellSpursJobMain2(CellSpursJobContext2* stInfo, CellSpursJob256 *job)
{
	(void)stInfo;
	spu_printf("notify\n");
	uint64_t counter_ea = job->workArea.userData[0];
	uint64_t event_ea = job->workArea.userData[1];
	struct {
		uint32_t val;
		uint8_t pad[128 - sizeof(uint32_t)];
	} counter __attribute__((aligned(128)));
	uint32_t old = cellAtomicIncr32(&counter.val, counter_ea);
	spu_printf(" ****** event sender : ev=%llx, counter=%d\n", 0, old);
	cellSpursEventFlagSet(event_ea, 1 << old);
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
