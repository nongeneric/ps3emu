/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>						// memset
#include <assert.h>
#include <sys/ppu_thread.h>
#include <cell/spurs.h>
#include "sample_config.h"
#include "common.h"

#define PPU_THR_PRIO					2000
#define PPU_THR_STACK_SIZE				0x4000

/* embedded SPU ELF symbols */
extern CellSpursTaskBinInfo _binary_task_task_lfqueue_spu_elf_taskbininfo;

static
void sender_entry(uint64_t arg)
{
	int ret;
	cell::Spurs::LFQueue *ppu2spu_queue = (cell::Spurs::LFQueue *)(uintptr_t)arg;

	//printf("PPU: SPURS lock free queue data stream started\n");

	for (unsigned int i = 0; i < ITERATION; i++) {
		uint32_t buf[QUEUE_ENTRY_SIZE/sizeof(uint32_t)] __attribute__((aligned(16)));
		buf[0] = i;
		ret = ppu2spu_queue->push(buf);
		if (ret) {
			printf("cellSpursLFQueuePush failed: %x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}
	}

	sys_ppu_thread_exit(0);
}

static
void receiver_entry(uint64_t arg)
{
	int ret;
	cell::Spurs::LFQueue *spu2ppu_queue = (cell::Spurs::LFQueue*)(uintptr_t)arg;

	for (unsigned int i = 0; i < ITERATION; i++) {
		uint32_t buf[QUEUE_ENTRY_SIZE/sizeof(uint32_t)] __attribute__((aligned(16)));
		ret = spu2ppu_queue->pop(buf);
		if (ret) {
			printf("cellSpursLFQueuePop failed: %x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}
		if (buf[0] != i) {
			printf("data compare error expected=%d, actual=%d\n", i, buf[0]);
			printf("## libspurs(task) : sample_spurs_lfqueue FAILED ##\n");
			sys_ppu_thread_exit(1);
		}
	}

	//printf("PPU: all data successfully verified\n");

	sys_ppu_thread_exit(0);
}

int sample_main(cell::Spurs::Spurs *spurs)
{
	int ret;

	/* allocate memory */
	cell::Spurs::Taskset2 *taskset       = (cell::Spurs::Taskset2*)memalign(cell::Spurs::Taskset2::kAlign, sizeof(cell::Spurs::Taskset2));
	cell::Spurs::LFQueue  *ppu2spu_queue = (cell::Spurs::LFQueue*) memalign(cell::Spurs::LFQueue::kAlign,  sizeof(cell::Spurs::LFQueue));
	cell::Spurs::LFQueue  *spu2spu_queue = (cell::Spurs::LFQueue*) memalign(cell::Spurs::LFQueue::kAlign,  sizeof(cell::Spurs::LFQueue));
	cell::Spurs::LFQueue  *spu2ppu_queue = (cell::Spurs::LFQueue*) memalign(cell::Spurs::LFQueue::kAlign,  sizeof(cell::Spurs::LFQueue));
	void *ppu2spu_queue_buffer      = memalign(128, QUEUE_ENTRY_SIZE * QUEUE_DEPTH);
	void *spu2spu_queue_buffer      = memalign(128, QUEUE_ENTRY_SIZE * QUEUE_DEPTH);
	void *spu2ppu_queue_buffer      = memalign(128, QUEUE_ENTRY_SIZE * QUEUE_DEPTH);
	void *context[NUM_TASK];

	if ( taskset == NULL ||
	     ppu2spu_queue == NULL || ppu2spu_queue_buffer == NULL ||
	     spu2spu_queue == NULL || spu2spu_queue_buffer == NULL ||
	     spu2ppu_queue == NULL || spu2ppu_queue_buffer == NULL ) {
		printf("malloc failed\n");
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* create taskset */
	cell::Spurs::TasksetAttribute2 attributeTaskset;
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

	/* initialize lock-free queues */
	memset(ppu2spu_queue, 0, sizeof(cell::Spurs::LFQueue)); // NOTE: CellSpursLFQueue must be zero-cleared!
	ret = cell::Spurs::LFQueue::initialize((CellSpursTaskset*)taskset, ppu2spu_queue, ppu2spu_queue_buffer,
										   QUEUE_ENTRY_SIZE, QUEUE_DEPTH, CELL_SPURS_LFQUEUE_PPU2SPU);
	if (ret) {
		printf("cellSpursLFQueueInitialize failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = ppu2spu_queue->attachLv2EventQueue(); // PPU thread needs to wait for this queue when it is full
	assert(ret == CELL_OK);

	memset(spu2spu_queue, 0, sizeof(cell::Spurs::LFQueue)); // NOTE: CellSpursLFQueue must be zero-cleared!
	ret = cell::Spurs::LFQueue::initialize((CellSpursTaskset*)taskset, spu2spu_queue, spu2spu_queue_buffer,
									 QUEUE_ENTRY_SIZE, QUEUE_DEPTH, CELL_SPURS_LFQUEUE_SPU2SPU);
	if (ret) {
		printf("cellSpursLFQueueInitialize : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	memset(spu2ppu_queue, 0, sizeof(cell::Spurs::LFQueue)); // NOTE: CellSpursLFQueue must be zero-cleared!
	ret = cell::Spurs::LFQueue::initialize((CellSpursTaskset*)taskset, spu2ppu_queue, spu2ppu_queue_buffer,
									 QUEUE_ENTRY_SIZE, QUEUE_DEPTH, CELL_SPURS_LFQUEUE_SPU2PPU);
	if(ret != CELL_OK){
		printf("cellSpursLFQueueInitialize : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = spu2ppu_queue->attachLv2EventQueue(); // PPU thread needs to wait for this queue when it is empty
	assert(ret == CELL_OK);

	const CellSpursTaskBinInfo &binInfo = _binary_task_task_lfqueue_spu_elf_taskbininfo;
	/* create tasks */
	CellSpursTaskId idTask[NUM_TASK];

	/* create task#1 */
	CellSpursTaskArgument taskArg;
	taskArg.u64[0] = (uintptr_t)ppu2spu_queue; // input queue
	taskArg.u64[1] = (uintptr_t)spu2spu_queue; // output queue
	context[0] = memalign(CELL_SPURS_TASK_CONTEXT_ALIGN, binInfo.sizeContext);

	ret = taskset->createTask2(&idTask[0], &binInfo,  &taskArg, context[0], "lfqueue task0");
	assert(ret == CELL_OK);

	/* create task#2 */
	taskArg.u64[0] = (uintptr_t)spu2spu_queue; // input queue
	taskArg.u64[1] = (uintptr_t)spu2ppu_queue; // output queue
	context[1] = memalign(CELL_SPURS_TASK_CONTEXT_ALIGN, binInfo.sizeContext);

	ret = taskset->createTask2(&idTask[1], &binInfo,  &taskArg, context[1], "lfqueue task1");
	assert(ret == CELL_OK);

	/* create PPU threads */
	sys_ppu_thread_t ppu_sender, ppu_receiver;
	ret = sys_ppu_thread_create(&ppu_sender, sender_entry, (uintptr_t)ppu2spu_queue,
				    PPU_THR_PRIO, PPU_THR_STACK_SIZE, SYS_PPU_THREAD_CREATE_JOINABLE,
				    "PPU sender");
	assert(ret == CELL_OK);

	ret = sys_ppu_thread_create(&ppu_receiver, receiver_entry, (uintptr_t)spu2ppu_queue,
				    PPU_THR_PRIO, PPU_THR_STACK_SIZE, SYS_PPU_THREAD_CREATE_JOINABLE,
				    "PPU receiver");
	assert(ret == CELL_OK);

	/* wait for completion of PPU threads */
	printf ("PPU: waiting for completion of sender/receiver PPU threads\n");
	uint64_t exit_code;
	ret = sys_ppu_thread_join(ppu_sender, &exit_code);
	assert(ret == CELL_OK);
	assert(exit_code == 0);

	ret = sys_ppu_thread_join(ppu_receiver, &exit_code);
	assert(ret == CELL_OK);
	assert(exit_code == 0);

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

	printf ("PPU: taskset completed\n");

	/* free resources */
	ret = ppu2spu_queue->detachLv2EventQueue();
	assert(ret == CELL_OK);

	ret = spu2ppu_queue->detachLv2EventQueue();
	assert(ret == CELL_OK);

	free(taskset);
	free(spu2ppu_queue);
	free(spu2spu_queue);
	free(ppu2spu_queue);
	free(spu2ppu_queue_buffer);
	free(spu2spu_queue_buffer);
	free(ppu2spu_queue_buffer);

	printf("PPU: sample_spurs_lfqueue finished.\n");

	return 0;
}
