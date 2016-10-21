/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cell/spurs.h>
#include "sample_config.h"
#include "sample_barrier.h"

/* embedded SPU ELF symbols */
extern CellSpursTaskBinInfo _binary_task_barrier_task_spu_elf_taskbininfo;

int sample_main(cell::Spurs::Spurs *spurs)
{
	int ret;

	/* allocate memory */
	cell::Spurs::Taskset2 *taskset     = (cell::Spurs::Taskset2*)memalign(CELL_SPURS_TASKSET2_ALIGN, sizeof(cell::Spurs::Taskset2));
	cell::Spurs::Barrier *barrier      = (cell::Spurs::Barrier*) memalign(CELL_SPURS_BARRIER_ALIGN, CELL_SPURS_BARRIER_SIZE);

	void *context[NUM_TASK];

	if (taskset == NULL || barrier == NULL) {
		printf("memalign failed\n");
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* create taskset */
	cell::Spurs::TasksetAttribute2 attributeTaskset;
	cell::Spurs::TasksetAttribute2::initialize(&attributeTaskset);
	attributeTaskset.name = SAMPLE_NAME;
	for (unsigned i = 0; i < NUM_SPU; ++i) {
		attributeTaskset.priority[i] = 8;
	}
	for (unsigned i = NUM_SPU; i < 8; ++i) {
		attributeTaskset.priority[i] = 0;
	}
	attributeTaskset.maxContention = NUM_SPU;

	ret = cell::Spurs::Taskset2::create(spurs, taskset, &attributeTaskset);
	if (ret) {
		printf("cell::Spurs::Taskset2::create() failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* initialize barrier */
	ret = cell::Spurs::Barrier::initialize((CellSpursTaskset*)taskset, barrier, NUM_TASK);
	assert(ret == CELL_OK);

	/* create tasks */
	CellSpursTaskId idTask[NUM_TASK];

	for (unsigned i = 0; i < NUM_TASK; ++i) {
		/* initialize task argument */
		CellSpursTaskArgument taskArg;
		taskArg.u64[0] = (uintptr_t)barrier;
		taskArg.u64[1] = 0;

		const CellSpursTaskBinInfo &binInfo = _binary_task_barrier_task_spu_elf_taskbininfo;
		context[i] = memalign(CELL_SPURS_TASK_CONTEXT_ALIGN, binInfo.sizeContext);
		assert(context[i] != NULL);

		ret = taskset->createTask2(&idTask[i], &binInfo, &taskArg, context[i], "barrier_task");
		assert(ret == CELL_OK);

		/* initialize task attribute */
	}

	/* wait for completion of tasks */
	printf ("PPU: waiting for completion of tasks\n");

	bool isAborted = false;
	for (unsigned i = 0; i < NUM_TASK; ++i) {
		int exitCode;
		ret = taskset->joinTask2(idTask[i], &exitCode);
		assert(ret == CELL_OK || ret == CELL_SPURS_TASK_ERROR_ABORT);
		if (ret == CELL_SPURS_TASK_ERROR_ABORT) {
			printf("Task#%u has been aborted\n", idTask[i]);
			isAborted = true;
		}
		if(ret == CELL_OK){
			printf("Task#%u exited with code %d\n", idTask[i], exitCode);
		}
	}

	if (isAborted) {
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* finish taskset */
	printf("PPU: destroy taskset\n");
	ret = taskset->destroy();
	assert(ret == CELL_OK);


	/* free memory */
	free(taskset);
	free(barrier);
	for (unsigned i = 0; i < NUM_TASK; ++i) {
		free(context[i]);
	}

	return CELL_OK;
}