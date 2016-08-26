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

/* embedded SPU ELF symbols */
extern CellSpursTaskBinInfo _binary_task_signal_spu_elf_taskbininfo;

int sample_main(cell::Spurs::Spurs *spurs)
{
	int ret;

	/* allocate memory */
	cell::Spurs::Taskset2 *taskset       = (cell::Spurs::Taskset2*)memalign(CELL_SPURS_TASKSET2_ALIGN, sizeof(CellSpursTaskset2));

	if (taskset == NULL) {
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
		printf("cellSpursCreateTasksetWithAttribute failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}


	/* create task */
	CellSpursTaskId	tid;
	CellSpursTaskArgument arg;
	const CellSpursTaskBinInfo &binInfo = _binary_task_signal_spu_elf_taskbininfo;
	void *context = memalign(CELL_SPURS_TASK_CONTEXT_ALIGN, binInfo.sizeContext);
	ret = taskset->createTask2(&tid, &binInfo, &arg, context, "signal task");
	assert(ret == CELL_OK);

	/* send signal to the task */
	ret = taskset->sendSignal(tid);
	assert(ret == CELL_OK);

	/* join task */
	//printf ("PPU: wait for task completion\n");
	int exitCode;

	ret = taskset->joinTask2(tid, &exitCode);
	assert(ret == CELL_OK || ret == CELL_SPURS_TASK_ERROR_ABORT);
	free(context);

	if(ret == CELL_OK){
		//printf("Task#%u exited with code %d\n", tid, exitCode);
	}else{
		printf("Task#%u has been aborted\n", tid);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* join taskset */
	ret = taskset->destroy();
	assert(ret == CELL_OK);

	/* free memory */
	free(taskset);

	return 0;
}
