/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <cell/spurs.h>
#include "sample_config.h"

/* embedded SPU ELF symbols */
extern const CellSpursTaskBinInfo _binary_task_task_hello_spu_elf_taskbininfo;

int sample_main(cell::Spurs::Spurs *spurs)
{
	int ret;

	/* allocate memory */
	cell::Spurs::Taskset2 *taskset = (cell::Spurs::Taskset2*)memalign(CELL_SPURS_TASKSET2_ALIGN, CELL_SPURS_TASKSET2_SIZE);
	if (taskset == NULL) {
		printf("memalign failed\n");
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* initialize taskset attribute */
	cell::Spurs::TasksetAttribute2 attributeTaskset;
	cell::Spurs::TasksetAttribute2::initialize(&attributeTaskset);
	attributeTaskset.name = SAMPLE_NAME;

	/* create taskset */
	ret = cell::Spurs::Taskset2::create(spurs, taskset, &attributeTaskset);
	if (ret) {
		printf("cell::Spurs::Taskset2::create() failed: %d\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* create task */
	CellSpursTaskId	tid;
	CellSpursTaskArgument arg;
	ret = taskset->createTask2(&tid, &_binary_task_task_hello_spu_elf_taskbininfo, &arg, NULL, "hello task");
	assert(ret == CELL_OK);

	/* join task */
	//printf ("PPU: wait for completion of the task\n");
	int exitCode;
	ret = taskset->joinTask2(tid, &exitCode);
	assert(ret == CELL_OK);

	if (ret != CELL_OK) {
		printf("Task#%u has been aborted\n", tid);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	//printf("Task#%u exited with code %d\n", tid, exitCode);

	/* finish taskset */
	ret = taskset->destroy();
	assert(ret == CELL_OK);

	//printf("PPU: taskset completed\n");

	/* free memory */
	free(taskset);

	return 0;
}
