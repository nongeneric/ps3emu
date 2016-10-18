/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cell/spurs.h>
#include "sample_config.h"
#include "sample_queue.h"

/* embedded SPU ELF symbols */
extern const CellSpursTaskBinInfo _binary_task_producer_spu_elf_taskbininfo;
extern const CellSpursTaskBinInfo _binary_task_consumer_spu_elf_taskbininfo;

int sample_main(cell::Spurs::Spurs *spurs)
{
	int ret;

	/* allocate memory */
	cell::Spurs::Taskset2 *taskset   = (cell::Spurs::Taskset2*)memalign(cell::Spurs::Taskset2::kAlign, sizeof(cell::Spurs::Taskset2));
	cell::Spurs::Queue    *queue     = (cell::Spurs::Queue*)   memalign(cell::Spurs::Queue::kAlign,    sizeof(cell::Spurs::Queue));
	void *queueBuffer = memalign(128, QUEUE_ENTRY_SIZE * QUEUE_DEPTH);

	if (taskset == NULL || queue == NULL) {
		printf("memalign failed\n");
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* create taskset */
	cell::Spurs::TasksetAttribute2	attributeTaskset;
	cell::Spurs::TasksetAttribute2::initialize(&attributeTaskset);
	attributeTaskset.name = SAMPLE_NAME;
	attributeTaskset.maxContention = NUM_SPU;
	for (unsigned i = 0; i < NUM_SPU; ++i) {
		attributeTaskset.priority[i] = 8;
	}
	for (unsigned i = NUM_SPU; i < 8; ++i) {
		attributeTaskset.priority[i] = 0;
	}

	ret = cell::Spurs::Taskset2::create(spurs, taskset, &attributeTaskset);
	if (ret) {
		printf("cellSpursCreateTaskset2 failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* initialize queue */
	ret = cell::Spurs::Queue::initialize((CellSpursTaskset*)taskset, queue, queueBuffer, QUEUE_ENTRY_SIZE, QUEUE_DEPTH, CELL_SPURS_QUEUE_SPU2SPU);
	if (ret) {
		printf("cellSpursQueueInitialize failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* create tasks */
	CellSpursTaskId	idTask[NUM_TASK];
	void *context[NUM_TASK];

	for (int i = 0; i < NUM_TASK; i++) {
		/* initialize task attribute */
		const CellSpursTaskBinInfo *binInfo = NULL;
		if (i % 2) {
			binInfo = &_binary_task_producer_spu_elf_taskbininfo;
		}
		else {
			binInfo = &_binary_task_consumer_spu_elf_taskbininfo;
		}

		/* create task */
		CellSpursTaskArgument arg;
		arg.u64[0] = (uintptr_t)queue;
		context[i] = memalign(CELL_SPURS_TASK_CONTEXT_ALIGN, binInfo->sizeContext);

		ret = taskset->createTask2(&idTask[i], binInfo, &arg, context[i], "queue task");
		assert(ret == CELL_OK);
	}

	/* wait for completion of tasks */
	printf ("PPU: waiting for completion of tasks\n");

	bool isAborted = false;
	for (int i = 0; i < NUM_TASK; i++) {
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
		free(context[i]);
	}

	if (isAborted) {
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* finish taskset */
	ret = taskset->destroy();
	assert(ret == CELL_OK);

	/* free memory */
	free(taskset);
	free(queue);
	free(queueBuffer);

	return CELL_OK;
}
