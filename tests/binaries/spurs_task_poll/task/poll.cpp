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

#include <cell/dma.h>

#include "../common.h"

CELL_SPU_LS_PARAM(16*1024, 16*1024);

#include <spu_printf.h>
#ifdef ENABLE_SPU_PRINTF
#define	SAMPLE_PRINTF(...)	spu_printf(__VA_ARGS__)
#else
#define	SAMPLE_PRINTF(...)
#endif


int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTaskset;
	uint64_t	eaSpuQueue = spu_extract((vec_ullong2)argTask, 0);
	cell::Spurs::QueueStub spuQueue;
	spuQueue.setObject(eaSpuQueue);

#ifdef ENABLE_SPU_PRINTF
	uint64_t	eaTaskset = cellSpursGetTasksetAddress();
	CellSpursTaskId idTask = cellSpursGetTaskId();
	unsigned int idSpu = cellSpursGetCurrentSpuId();
#endif

	SAMPLE_PRINTF("[Taskset:%llx][Task#%02u][SPU#%u] start\n", eaTaskset, idTask, idSpu);
	int	ret;
	int	loop = 1;
	static	uint32_t	buffer[BUFFER_AREA_SIZE / sizeof(uint32_t)] __attribute__((aligned(128)));
	do {
		Command	cmd __attribute__((aligned(128)));
		if (cellSpursTaskPoll() & CELL_SPURS_TASK_POLL_FOUND_WORKLOAD) {
			/* high priority taskset becomes active */
			/* try to yield SPU */
			SAMPLE_PRINTF("[Taskset:%llx][Task#%02u][SPU#%u] high priority taskset becomes active: yield SPU\n", eaTaskset, idTask, idSpu);
			cellSpursYield();
		}
		ret = spuQueue.popBegin(&cmd, 0);
		if (ret) {
			SAMPLE_PRINTF("[Taskset:%llx][Task#%02u][SPU#%u] cellSpursQueuePopBegin failed : %x\n", eaTaskset, idTask, idSpu, ret);
			abort();
		}
		ret = spuQueue.popEnd(0);
		if (ret) {
			SAMPLE_PRINTF("[Taskset:%llx][Task#%02u][SPU#%u] cellSpursQueuePopEnd failed : %x\n", eaTaskset, idTask, idSpu, ret);
			abort();
		}
		switch (cmd.type) {
		case CommandIncrement:
			{
				cellDmaGet(buffer, cmd.eaSrc, BUFFER_AREA_SIZE, 0, 0, 0);
				cellDmaWaitTagStatusAll(1 << 0);
				for (unsigned i = 0; i < BUFFER_AREA_SIZE / sizeof(uint32_t); i++) {
					buffer[i] = buffer[i] + cmd.value;
				}
				cellDmaPut(buffer, cmd.eaDst, BUFFER_AREA_SIZE, 0, 0, 0);
				cellDmaWaitTagStatusAll(1 << 0);
				break;
			}
		case CommandCheckValue:
			{
				uint64_t	ea[2];
				ea[0] = cmd.eaSrc;
				ea[1] = cmd.eaDst;
				SAMPLE_PRINTF("[Taskset:%llx][Task#%02u][SPU#%u] Check Value:", eaTaskset, idTask, idSpu);
				for (unsigned i = 0; i < 2; i++) {
					cellDmaGet(buffer, ea[i], BUFFER_AREA_SIZE, 0, 0, 0);
					cellDmaWaitTagStatusAll(1 << 0);
					SAMPLE_PRINTF("[ea %llx:%08x]", ea[i], buffer[0]);
				}
				SAMPLE_PRINTF("\n");
				break;
			}
		case CommandTerminate:
			{
				loop = 0;
				break;
			}
		};
	} while (loop);

	SAMPLE_PRINTF("[Taskset:%llx][Task#%02u][SPU#%u] exit\n", eaTaskset, idTask, idSpu);
	return 0;
}

