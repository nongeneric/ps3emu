/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <spu_intrinsics.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/spu_thread.h>
#include <cell/spurs.h>
#include <spu_printf.h>

CELL_SPU_LS_PARAM(16*1024, 16*1024);

#define	NUM_YIELD	5


int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTaskset;
	int	ret;
	CellSpursTaskId idTask = cellSpursGetTaskId();
	unsigned int idSpu = cellSpursGetCurrentSpuId();

	//spu_printf("[Task#%02u][SPU#%u] start\n", idTask, idSpu);

	/* wait for creation of other tasks */
	uint64_t eaBarrier = spu_extract((vec_ullong2)argTask, 0);
	cell::Spurs::BarrierStub barrier;
	barrier.setObject(eaBarrier);

	ret = barrier.notify();
	if (ret) {
		spu_printf("[Task#%02u][SPU#%u] cellSpursYield failed: %x\n", idTask, idSpu, ret);
		abort();
	}

	ret = barrier.wait();
	if (ret) {
		spu_printf("[Task#%02u][SPU#%u] cellSpursYield failed: %x\n", idTask, idSpu, ret);
		abort();
	}

	/* start yielding */
	for (int i = 0; i < NUM_YIELD; i++) {
		//spu_printf("[Task#%02u][SPU#%u] yield this SPU\n", idTask, idSpu);
		ret = cellSpursYield();
		if (ret) {
			spu_printf("[Task#%02u][SPU#%u] cellSpursYield failed: %x\n", idTask, idSpu, ret);
			abort();
		}
		//spu_printf("[Task#%02u][SPU#%u] wake up\n", idTask, idSpu);
	}

	//spu_printf("[Task#%02u][SPU#%u] exit\n", idTask, idSpu);

	return 0;
}
