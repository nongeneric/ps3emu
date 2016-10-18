/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdint.h>
#include <stdlib.h>
#include <spu_intrinsics.h>
#include <spu_printf.h>
#include <cell/spurs.h>
#include <cell/dma.h>
#include "../sample_queue.h"

#define DMA_TAG 0

CELL_SPU_LS_PARAM(16*1024, 16*1024);

static
void do_push(uint64_t eaQueue)
{
	int ret;
	static uint8_t data[QUEUE_ENTRY_SIZE] __attribute__((aligned(16)));

	cell::Spurs::QueueStub queue;
	queue.setObject(eaQueue);

	CellSpursTaskId idTask = cellSpursGetTaskId();
	unsigned int idSpu = cellSpursGetCurrentSpuId();

	//spu_printf("[Task#%02u][SPU#%u] Do push %d times\n", idTask, idSpu, NUM_PUSH);

	for (int i = 0; i < NUM_PUSH; i++) {
		ret = queue.pushBegin(data, DMA_TAG);
		if (ret) {
			spu_printf("[Task#%02u][SPU#%u] cellSpursQueuePushBegin failed: %x\n", idTask, idSpu, ret);
			abort();
		}

		ret = queue.pushEnd(DMA_TAG);
		if (ret) {
			spu_printf("[Task#%02u][SPU#%u] cellSpursQueuePushEnd failed: %x\n", idTask, idSpu, ret);
			abort();
		}

		//spu_printf("[Task#%02u][SPU#%u] cellSpursQueuePush done %d times\n", idTask, idSpu, i);
	}
}

int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTaskset;
	CellSpursTaskId idTask = cellSpursGetTaskId();
	unsigned int idSpu = cellSpursGetCurrentSpuId();

	//spu_printf("[Task#%02u][SPU#%u] producer start\n", idTask, idSpu);

	uint64_t eaQueue = spu_extract((vec_ullong2)argTask, 0);
	do_push(eaQueue);

	//spu_printf("[Task#%02u][SPU#%u] exit\n", idTask, idSpu);
	return 0;
}
