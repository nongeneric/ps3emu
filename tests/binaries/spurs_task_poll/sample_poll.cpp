/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/timer.h>
#include <sys/ppu_thread.h>
#include <cell/spurs.h>
#include "sample_config.h"
#include "common.h"

#define SPU_QUEUE_DEPTH	16
#define SPU_QUEUE_SIZE	(sizeof(Command))

/* embedded SPU ELF symbols */
extern CellSpursTaskBinInfo _binary_task_task_poll_spu_elf_taskbininfo;

typedef struct WorkerArg {
	cell::Spurs::Queue*	spuQueue;
	uint8_t*		buffer0;
	uint8_t*		buffer1;
} WorkerArg;

static
void hi_prio_work_thr_entry(uint64_t arg)
{
	int	ret;

	WorkerArg* workerArg = (WorkerArg*)(uintptr_t)arg;
	Command	cmd;
	for (int i = 0; i < 16; i++) {
		sys_timer_usleep(1024 * 200);
		cmd.type = CommandCheckValue;
		cmd.eaSrc = (uintptr_t)workerArg->buffer0;
		cmd.eaDst = (uintptr_t)workerArg->buffer1;
		cmd.value = 0;
		ret = workerArg->spuQueue->push(&cmd);
		assert(ret == CELL_OK);
	}
	cmd.type = CommandTerminate;
	cmd.eaSrc = 0;
	cmd.eaDst = 0;
	cmd.value = 0;
	ret = workerArg->spuQueue->push(&cmd);
	assert(ret == CELL_OK);
	sys_ppu_thread_exit(0);
	(void)ret;
}

int sample_main(cell::Spurs::Spurs* spurs)
{
	int ret;

	/* allocate memory */
	cell::Spurs::Taskset2*	taskset[2];
	cell::Spurs::Queue*		spuQueue[2];
	uint8_t*			spuQueueBuffer[2];

	void *context[NUM_TASK];

	for (int i = 0; i < 2; i++) {
		taskset[i] = (cell::Spurs::Taskset2*)memalign(cell::Spurs::Taskset2::kAlign, sizeof(cell::Spurs::Taskset2));
		spuQueue[i] = (cell::Spurs::Queue*)memalign(cell::Spurs::Queue::kAlign, sizeof(cell::Spurs::Queue));

		spuQueueBuffer[i] = (uint8_t*)memalign(128, SPU_QUEUE_SIZE * SPU_QUEUE_DEPTH);

		if (taskset[i] == NULL || spuQueue[i] == NULL) {
			printf("memalign failed\n");
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}
	}


	/* create taskset */
	uint8_t prios[2][8] = {
							{7, 7, 7, 7, 7, 7, 7, 7},
							{8, 8, 8, 8, 8, 8, 8, 8}
						};
	for (unsigned i = NUM_SPU; i < 8; ++i) {
		prios[0][i] = prios[1][i] = 0;
	}
	for (int i = 0; i < 2; i++) {
		cell::Spurs::TasksetAttribute2	attributeTaskset;
		cell::Spurs::TasksetAttribute2::initialize(&attributeTaskset);
		attributeTaskset.name = SAMPLE_NAME;
		attributeTaskset.maxContention = NUM_SPU;
		memcpy(attributeTaskset.priority, prios[i], sizeof(prios[i]));

		ret = cell::Spurs::Taskset2::create(spurs, taskset[i], &attributeTaskset);
		if (ret) {
			printf("cellSpursCreateTaskset2 failed: %x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}

		/* initialize queue for each taskset */
		ret = cell::Spurs::Queue::initialize((CellSpursTaskset*)taskset[i], spuQueue[i], spuQueueBuffer[i],
											 SPU_QUEUE_SIZE, SPU_QUEUE_DEPTH, CELL_SPURS_QUEUE_PPU2SPU);

		ret = spuQueue[i]->attachLv2EventQueue();
	}

	/* create tasks */
	CellSpursTaskId	idTask[NUM_TASK];

	for (int i = 0; i < NUM_TASK; i++) {
		const CellSpursTaskBinInfo &binInfo = _binary_task_task_poll_spu_elf_taskbininfo;

		/* create task */
		CellSpursTaskArgument	arg;
		arg.u64[0] = (uint64_t)(uintptr_t)spuQueue[i];
		arg.u64[1] = 0;
		context[i] = memalign(CELL_SPURS_TASK_CONTEXT_ALIGN, binInfo.sizeContext);

		ret = taskset[i]->createTask2(&idTask[i], &binInfo,  &arg, context[i], "poll task");
		assert(ret == CELL_OK);
	}

	/* start testing */
	uint8_t	*buffer[2];
	buffer[0] = (uint8_t*)memalign(128, BUFFER_AREA_SIZE);
	buffer[1] = (uint8_t*)memalign(128, BUFFER_AREA_SIZE);

	if (buffer[0] == NULL || buffer[1] == NULL) {
		printf("memalign failed\n");
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}
	memset(buffer[0], 0, BUFFER_AREA_SIZE);

	/* create ppu thread for taskset0 */
	sys_ppu_thread_t	hi_prio_work_thr_id;
	WorkerArg	workerArg;
	workerArg.spuQueue = spuQueue[0];
	workerArg.buffer0 = buffer[0];
	workerArg.buffer1 = buffer[1];

	ret = sys_ppu_thread_create(&hi_prio_work_thr_id,
							&hi_prio_work_thr_entry,
							(uintptr_t)&workerArg,
							0, 0x4000,
							SYS_PPU_THREAD_CREATE_JOINABLE,
							"hi_prio_work_thread");
	assert(ret == CELL_OK);

	Command	cmd __attribute__((aligned(16)));
	for (int i = 0; i < 1024; i++) {
		cmd.type = CommandIncrement;
		cmd.eaSrc = (uintptr_t)buffer[i % 2];
		cmd.eaDst = (uintptr_t)buffer[1 - (i % 2)];
		cmd.value = 1;

		ret = spuQueue[1]->push(&cmd);
		assert(ret == CELL_OK);
	}
	cmd.type = CommandTerminate;
	cmd.eaSrc = 0;
	cmd.eaDst = 0;
	cmd.value = 0;
	ret = spuQueue[1]->push(&cmd);
	assert(ret == CELL_OK);

	/* wait for completion of tasks */
	printf ("PPU: waiting for completion of tasks\n");

	uint64_t	ppuThreadExitCode;
	ret = sys_ppu_thread_join(hi_prio_work_thr_id, &ppuThreadExitCode);

	bool isAborted = false;
	for (int i = 0; i < NUM_TASK; i++) {
		int exitCode;
		ret = taskset[i]->joinTask2(idTask[i], &exitCode);
		assert(ret == CELL_OK || ret == CELL_SPURS_TASK_ERROR_ABORT);

		if(ret == CELL_OK){
			printf("Task#%u exited with code %d\n", idTask[i], exitCode);
		}else{
			printf("Task#%u has been aborted\n", idTask[i]);
			isAborted = true;
		}
	}

	if (isAborted) {
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* finish taskset */
	for (int i = 0; i < 2; i++) {
		ret = spuQueue[i]->detachLv2EventQueue();
		assert(ret == CELL_OK);

		printf ("PPU: destroy taskset\n");
		ret = taskset[i]->destroy();
		assert(ret == CELL_OK);
	}

	/* free memory */
	free(buffer[0]);
	free(buffer[1]);

	for (int i = 0; i < 2; i++) {
		free(taskset[i]);
		free(spuQueueBuffer[i]);
		free(spuQueue[i]);
	}
	for (int i = 0; i < NUM_TASK; i++) {
		free(context[i]);
	}

	return CELL_OK;
}
