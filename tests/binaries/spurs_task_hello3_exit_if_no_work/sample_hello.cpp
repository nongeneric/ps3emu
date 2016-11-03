/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 360.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <cell/spurs.h>
#include <sys/timer.h>
#include "sample_config.h"

/* embedded SPU ELF symbols */
extern const CellSpursTaskBinInfo _binary_task_task_hello_spu_elf_taskbininfo;

void dumpTaskSet(cell::Spurs::Taskset2* taskset) {
	CellSpursTasksetInfo info;
	cellSpursGetTasksetInfo(taskset, &info);
	printf("taskset info:\n");
	printf("  argument: %llx\n", info.argument);
	printf("  idWorkload: %x\n", info.idWorkload);
	printf("  idLastScheduledTask: %x\n", info.idLastScheduledTask);
	printf("  size: %x\n", info.sizeTaskset);
	for (int i = 0; i < CELL_SPURS_MAX_TASK; ++i) {
		if (!info.taskInfo[i].eaElf)
			continue;
		printf("task %x (%d)\n", i, i);
		printf("  lsPattern: %016llx\n", info.taskInfo[i].lsPattern.u64[0]);
		printf("  lsPattern: %016llx\n", info.taskInfo[i].lsPattern.u64[1]);
		printf("  eaContext: %08x\n", (uint32_t)info.taskInfo[i].eaContext);
		printf("  sizeContext: %08x\n", info.taskInfo[i].sizeContext);
		printf("  state: %02x\n", info.taskInfo[i].state);
		printf("  hasSignal: %02x\n", info.taskInfo[i].hasSignal);
		uint64_t* exitCode = (uint64_t*)info.taskInfo[i].eaTaskExitCode;
		if (!exitCode)
			continue;
		/*printf("  exitCode: %016llx\n", exitCode[0]);
		printf("  exitCode: %016llx\n", exitCode[1]);*/
	}
}

void stub(uint32_t arg) {}

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

	const int tasks = CELL_SPURS_MAX_TASK;
	
	static uint32_t res[0x80 / sizeof(uint32_t)] __attribute__((aligned(0x80))) = {0};

	CellSpursTaskId	tid[tasks];
	for (int i = 0; i < tasks; ++i) {
		CellSpursTaskArgument arg;
		arg.u32[0] = (uint32_t)res;
		const CellSpursTaskBinInfo* bin = &_binary_task_task_hello_spu_elf_taskbininfo;
		ret = taskset->createTask2(&tid[i], &_binary_task_task_hello_spu_elf_taskbininfo, &arg, NULL, "hello task");
		assert(ret == CELL_OK);
		sys_timer_usleep(300 * 1000);
	}

	//dumpTaskSet(taskset);

	/* join task */
	printf ("PPU: wait for completion of the task\n");
	int exitCode;
	for (int i = 0; i < tasks; ++i) {
		ret = taskset->joinTask2(tid[i], &exitCode);
		if (ret != CELL_OK) {
			printf("Task#%u has been aborted\n", tid[i]);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}
		//sys_timer_sleep(1);
	}

	printf("res = %d\n", res[0]);

	/* finish taskset */
	ret = taskset->destroy();
	assert(ret == CELL_OK);

	printf("PPU: taskset completed\n");

	/* free memory */
	free(taskset);

	return 0;
}
