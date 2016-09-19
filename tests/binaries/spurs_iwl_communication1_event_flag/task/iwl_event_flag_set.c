/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <cell/dma.h>
#include <cell/spurs/task.h>
#include <cell/spurs/event_flag.h>

#include "../common.h"

#define	TAG_ID	0

#include <spu_printf.h>
#include <assert.h>

int
cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	int	ret;
	static uint8_t	buf[sizeof(Args)]__attribute__((aligned(128)));
	cellDmaGet (buf, argTaskset, sizeof(Args), 0, 0, 0);
	cellDmaWaitTagStatusAll(1 << 0);

	uint64_t	myNum = spu_extract ((vec_ullong2)argTask, 0);

	volatile Args*	args = (volatile Args*)buf;

	CellSpursTaskId taskid = cellSpursGetTaskId();
	unsigned int spuid = cellSpursGetCurrentSpuId ();

	uint64_t	event_flag_ppu_spu_ea = args->event_flag_ppu_spu_ea;
	uint64_t	event_flag_spu_spu_ea = args->event_flag_spu_spu_ea;
	uint64_t	event_flag_spu_ppu_ea = args->event_flag_spu_ppu_ea;

	//spu_printf ("SPU: iwl_event_flag_set started [%lld]... \n", myNum);

	/* send started signal */
	ret = cellSpursEventFlagSet (event_flag_spu_ppu_ea, (0x0001) << myNum);
	assert (ret == CELL_OK);

	/* */
	//spu_printf ("SPU: iwl_event_flag_set waiting for notification... \n");
	uint16_t	mask = (0x0001) << myNum;
	ret = cellSpursEventFlagWait (event_flag_ppu_spu_ea, &mask, CELL_SPURS_EVENT_FLAG_AND);
	assert (ret == CELL_OK);

	//spu_printf ("SPU: iwl_event_flag_set waked up... \n");

	/* to make sure, all waiting task becomes waiting */
	cellSpursYield ();

	/* waking up waiter tasks */
	ret = cellSpursEventFlagSet (event_flag_spu_spu_ea, (0x0005) << myNum);
	assert (ret == CELL_OK);

	/* send completion */
  	//spu_printf("SPU: task %d spu %d -I - complete\n", taskid, spuid);
	ret = cellSpursEventFlagSet (event_flag_spu_ppu_ea, (0x0001) << myNum);
	assert (ret == CELL_OK);

	/* this taskset completes its execution */
  	return 0;
}
