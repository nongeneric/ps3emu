/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 360.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cell/spurs.h>
#include "sample_config.h"

/* embedded SPU ELF symbols */
extern const CellSpursTaskBinInfo _binary_task_event_flag_task_alpha_spu_elf_taskbininfo;
extern const CellSpursTaskBinInfo _binary_task_event_flag_task_beta_spu_elf_taskbininfo;

int sample_main(cell::Spurs::Spurs *spurs)
{
	int ret;

	/* allocate memory */
	cell::Spurs::Taskset2  *taskset    = (cell::Spurs::Taskset2*) memalign(cell::Spurs::Taskset2::kAlign, sizeof(cell::Spurs::Taskset2));
	cell::Spurs::EventFlag *eventflag0 = (cell::Spurs::EventFlag*)memalign(cell::Spurs::EventFlag::kAlign, sizeof(cell::Spurs::EventFlag));
	cell::Spurs::EventFlag *eventflag1 = (cell::Spurs::EventFlag*)memalign(cell::Spurs::EventFlag::kAlign, sizeof(cell::Spurs::EventFlag));
	void *context[NUM_TASK];

	if (taskset == NULL || eventflag0 == NULL || eventflag1 == NULL) {
		printf("memalign failed\n");
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	for (unsigned i = 0; i < NUM_TASK; ++i) {
		context[i] = memalign(CELL_SPURS_TASK_CONTEXT_ALIGN, CELL_SPURS_TASK_CONTEXT_SIZE_ALL);
		if (context[i] == NULL) {
			printf("memalign failed\n");
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}
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
		printf("cellSpursCreateTasksetWithAttribute failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* initialize event flags */
	ret = cell::Spurs::EventFlag::initialize((CellSpursTaskset*)taskset, eventflag0, CELL_SPURS_EVENT_FLAG_CLEAR_AUTO, CELL_SPURS_EVENT_FLAG_SPU2SPU);
	assert(ret == CELL_OK);

	ret = cell::Spurs::EventFlag::initialize((CellSpursTaskset*)taskset, eventflag1, CELL_SPURS_EVENT_FLAG_CLEAR_AUTO, CELL_SPURS_EVENT_FLAG_SPU2SPU);
	assert(ret == CELL_OK);

	//printf("eaEventFlag = %p, %p\n", eventflag0, eventflag1);

	/* create tasks */
	CellSpursTaskId	idTask[NUM_TASK];

	for (unsigned i = 0; i < NUM_TASK; ++i) {
		/* initialize task argument */
		CellSpursTaskArgument taskArg;
		taskArg.u32[0] = i - 1;
		taskArg.u32[1] = 0;
		taskArg.u32[2] = (uintptr_t)eventflag0;
		taskArg.u32[3] = (uintptr_t)eventflag1;

		const CellSpursTaskBinInfo* binInfo = NULL;
		if (i == 0) {
			// create this task only once
			binInfo = &_binary_task_event_flag_task_alpha_spu_elf_taskbininfo;
		}
		else {
			binInfo = &_binary_task_event_flag_task_beta_spu_elf_taskbininfo;
		}

		/* create task */
		context[i] = memalign(CELL_SPURS_TASK_CONTEXT_ALIGN, binInfo->sizeContext);
		ret = taskset->createTask2(&idTask[i], binInfo, &taskArg, context[i], "event flag task");
		assert(ret == CELL_OK);
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
	ret = taskset->destroy();
	assert(ret == CELL_OK);


	/* free memory */
	free(taskset);
	free(eventflag0);
	free(eventflag1);
	for (unsigned i = 0; i < NUM_TASK; ++i) {
		free(context[i]);
	}

	return 0;
}
