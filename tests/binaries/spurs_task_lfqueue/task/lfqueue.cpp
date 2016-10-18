/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

/* common headers */
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <spu_intrinsics.h>
#include <spu_printf.h>

/* Lv2 OS headers */
#include <sys/spu_thread.h>

/* spurs */
#include <cell/spurs/types.h>
#include <cell/spurs/task.h>
#include <cell/spurs/lfqueue.h>

#include "../common.h"

CELL_SPU_LS_PARAM(16*1024, 16*1024);

#define NUM_PARALLEL 8

void transfer(uint64_t inQueueEa, uint64_t outQueueEa);

int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTaskset;
	CellSpursTaskId idTask = cellSpursGetTaskId();
	unsigned int idSpu = cellSpursGetCurrentSpuId();

	//spu_printf("[Task#%02u][SPU#%u] start\n", idTask, idSpu);

	uint64_t eaInQueue = spu_extract((vec_ullong2)argTask, 0);
	uint64_t eaOutQueue = spu_extract((vec_ullong2)argTask, 1);

	transfer(eaInQueue, eaOutQueue);

	//spu_printf("[Task#%02u][SPU#%u] exit\n", idTask, idSpu);

	return 0;
}

void transfer(uint64_t eaInQueue, uint64_t eaOutQueue)
{
	int ret;
	static uint8_t buffer[NUM_PARALLEL][QUEUE_ENTRY_SIZE] __attribute__((aligned(128)));

	cell::Spurs::LFQueueStub inQueue;
	cell::Spurs::LFQueueStub outQueue;
	inQueue.setObject(eaInQueue);
	outQueue.setObject(eaOutQueue);


	cell::Spurs::LFQueuePopContainer  pop [NUM_PARALLEL];
	cell::Spurs::LFQueuePushContainer push[NUM_PARALLEL];

	/* initialize push/pop containers */
	for (int i = 0; i < NUM_PARALLEL; i++) {
		cell::Spurs::LFQueuePopContainer::initialize(&pop[i], buffer[i], i /* DMA tag */); 
		cell::Spurs::LFQueuePushContainer::initialize(&push[i], buffer[i], i+NUM_PARALLEL /* DMA tag */); 
	}

	/* pop ITERATION items from input queue and push them into output queue, in parallel */

	int in1 = 0;  // count pop begin
	int in2 = 0;  // count pop end
	int out1 = 0; // count push begin
	int out2 = 0; // count push end

	do {
		/* initiate pop operations as many as possible */
		while ((in1 < ITERATION) && (in1 - out2 < NUM_PARALLEL)) {
			ret =inQueue.tryPopBegin(&pop[in1%NUM_PARALLEL]); // non-blocking interface
			if (ret == CELL_SPURS_TASK_ERROR_AGAIN) {
				/* the input queue is empty or reached maximum parallelity (16 outstanding operations) */
				int inOutstanding  = in1 - in2;   // number of pop operations in progress
				int outOutstanding = out1 - out2; // number of push operations in progress
				if (inOutstanding == 0 && outOutstanding == 0) {
					/* no operation in progress, we yield this SPU to someone */
					ret = inQueue.popBegin(&pop[in1%NUM_PARALLEL]); // blocking interface
					if (ret == CELL_SPURS_TASK_ERROR_AGAIN) {
						/* the input queue has too many waiters */
						spu_printf("cellSpursLFQueuePopEnd failed: %x\n", ret);
						abort();
					}
				} else {
					/* push/pop operations are in progress, we hold this SPU */
					break;
				}
			}
			else if (ret != CELL_OK) {
				/* unexcpected error */
				spu_printf("cellSpursLFQueueTryPopBegin failed: %x\n", ret);
				abort();
			}
			in1++;
		}

		/* finish one pop operation */
		if (in1 > in2) {
			ret = inQueue.popEnd(&pop[in2%NUM_PARALLEL]);
			if (ret) {
				spu_printf("cellSpursLFQueuePopEnd failed: %x\n", ret);
				abort();
			}
			in2++;
		
			/* do something with the data here ... */
		}

		/* initiate push operations as many as possible */
		while (in2 > out1) {
			ret = outQueue.tryPushBegin(&push[out1%NUM_PARALLEL]); // non-blocking interface
			if (ret == CELL_SPURS_TASK_ERROR_AGAIN) {
				/* the output queue is full or reached maximum parallelity (16 outstanding operations) */
				int inOutstanding = in1 - in2;    // number of pop operations in progress
				int outOutstanding = out1 - out2; // number of push operations in progress
				if (inOutstanding == 0 && outOutstanding == 0) {
					/* no operation in progress, we yield this SPU to someone */
					ret = outQueue.pushBegin(&push[out1%NUM_PARALLEL]); // blocking interface
					if (ret == CELL_SPURS_TASK_ERROR_AGAIN) {
						/* the output queue has too many waiters */
						spu_printf("cellSpursLFQueuePopEnd failed: %x\n", ret);
						abort();
					}
				} else {
					/* push/pop operations are in progress, we hold this SPU */
					break;
				}
			}
			out1++;
		}

		/* finish one push operation */
		if (out1 > out2) {
			ret = outQueue.pushEnd(&push[out2%NUM_PARALLEL]);
			if (ret) {
				spu_printf("cellSpursLFQueuePushEnd failed: %x\n", ret);
				abort();
			}
			out2++;
		}
	} while (out2 != ITERATION);

	return;
}


