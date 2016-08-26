/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <spu_intrinsics.h>
#include <stdint.h>
#include <stdlib.h>
#include <spu_printf.h>
#include <cell/dma.h>
#include <cell/spurs.h>
#include "../sample_semaphore.h"

CELL_SPU_LS_PARAM(16*1024, 16*1024);

#define DMA_TAG 0


int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTaskset;
	int ret;
	CellSpursTaskId idTask = cellSpursGetTaskId();
	unsigned int idSpu = cellSpursGetCurrentSpuId();

	//spu_printf("[Task#%02u][SPU#%u] start\n", idTask, idSpu);

	uint64_t eaSemaphore = spu_extract((vec_ullong2)argTask, 0);
	cell::Spurs::SemaphoreStub semaphore;
	semaphore.setObject(eaSemaphore);
	
	for (int i = 0; i < 10; i++) {
		/* acquire resource */
		//spu_printf("[Task#%02u][SPU#%u] cellSpursSemaphoreP\n", idTask, idSpu);
		ret = semaphore.p();
		if (ret) {
			spu_printf("[Task#%02u][SPU#%u] cellSpursSemaphoreP failed : %x\n", idTask, idSpu, ret);
			abort();
		}

		/* do something with the resource ... */

		/* release the resource */
		//spu_printf("[Task#%02u][SPU#%u] cellSpursSemaphoreV\n", idTask, idSpu);
		ret = semaphore.v();
		if (ret) {
			spu_printf("[Task#%02u][SPU#%u] cellSpursSemaphoreV failed : %x\n", idTask, idSpu, ret);
			abort();

		}
	}

	//spu_printf("[Task#%02u][SPU#%u] exit\n", idTask, idSpu);
	return 0;
}

