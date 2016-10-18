/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <spu_intrinsics.h>
#include <stdint.h>
#include <stdlib.h>
#include <spu_printf.h>
#include <cell/spurs.h>
#include <cell/dma.h>

#include "../sample_barrier.h"

CELL_SPU_LS_PARAM(16*1024, 16*1024);


int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTaskset;
	int ret;
	CellSpursTaskId idTask = cellSpursGetTaskId();
	unsigned int idSpu = cellSpursGetCurrentSpuId();

	//spu_printf("[Task#%02u][SPU#%u] started\n", idTask, idSpu);

	uint64_t eaBarrier = spu_extract((vec_ullong2)argTask, 0);
	cell::Spurs::BarrierStub barrier;
	barrier.setObject(eaBarrier);

	for (int i = 0; i < NUM_ITERATION; i++) {
		/* let other tasks know I am ready for barrier synchronization */
		//spu_printf("[Task#%02u][SPU#%u] calling cellSpursBarrierNotify ...\n", idTask, idSpu);
		ret = barrier.notify();
		if (ret) {
			spu_printf("[Task#%02u][SPU#%u] barrier.notify() failed : %x\n", idTask, idSpu, ret);
			abort();
		}

		//spu_printf("[Task#%02u][SPU#%u] barrier.notify() done\n", idTask, idSpu);

		/* wait for completion of barrier synchronization */
		//spu_printf("[Task#%02u][SPU#%u] calling cellSpursBarrierWait ...\n", idTask, idSpu);
		ret = barrier.wait();
		if (ret) {
			spu_printf("[Task#%02u][SPU#%u] barrier.wait() failed : %x\n", idTask, idSpu, ret);
			abort();
		}

		//spu_printf("[Task#%02u][SPU#%u] barrier.wait() done\n", idTask, idSpu);
	}

	//spu_printf("[Task#%02u][SPU#%u] exit\n", idTask, idSpu);
	return 0;
}

