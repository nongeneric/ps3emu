/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 360.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <cell/dma.h>
#include <cell/spurs/task.h>
#include <cell/spurs/event_flag.h>
#include <spu_printf.h>
#include <stdlib.h>

#define	DMA_TAG	0

CELL_SPU_LS_PARAM(16*1024, 16*1024);


int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTaskset;
	int ret;
	CellSpursTaskId idTask = cellSpursGetTaskId();
	unsigned int idSpu = cellSpursGetCurrentSpuId();

	//spu_printf("[Task#%02u][SPU#%u] start\n", idTask, idSpu);

	uint32_t eaEventFlag0 = spu_extract((vec_uint4)argTask, 2);
	uint32_t eaEventFlag1 = spu_extract((vec_uint4)argTask, 3);
	cell::Spurs::EventFlagStub eventFlag0;
	cell::Spurs::EventFlagStub eventFlag1;
	eventFlag0.setObject(eaEventFlag0);
	eventFlag1.setObject(eaEventFlag1);

	/* sleep until all beta tasks set the event flags */
	uint16_t mask = 0xffff;
	//spu_printf("[Task#%02u][SPU#%u] waiting for event flag#0, mask = 0x%04x\n", idTask, idSpu, mask);
	ret = eventFlag0.wait(&mask, CELL_SPURS_EVENT_FLAG_AND);
	if (ret) {
		spu_printf("[Task#%02u][SPU#%u] eventFlag0.wait() failed : %x\n", idTask, idSpu, ret);
		abort();
	}

	//spu_printf("[Task#%02u][SPU#%u] waked up\n", idTask, idSpu);

	/* notify to all beta tasks */
	mask = 0xffff;
	//spu_printf("[Task#%02u][SPU#%u] set event flag#1, mask = 0x%04x\n", idTask, idSpu, mask);
	ret = eventFlag1.set(mask);
	if (ret) {
		spu_printf("[Task#%02u][SPU#%u] eventFlag1.set(mask) failed : %x\n", idTask, idSpu, ret);
		abort();
	}

	/* wait for acknowledgement */
	mask = 0xffff;
	//spu_printf("[Task#%02u][SPU#%u] waiting for event flag#0, mask = 0x%04x\n", idTask, idSpu, mask);
	ret = eventFlag0.wait(&mask, CELL_SPURS_EVENT_FLAG_AND);
	if (ret) {
		spu_printf("[Task#%02u][SPU#%u] eventFlag0.wait(&mask, CELL_SPURS_EVENT_FLAG_AND) failed : %x\n", idTask, idSpu, ret);
		abort();
	}

	//spu_printf("[Task#%02u][SPU#%u] waked up\n", idTask, idSpu);

	//spu_printf("[Task#%02u][SPU#%u] exit\n", idTask, idSpu);
  	return 0;
}
