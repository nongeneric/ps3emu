/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2010 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/ppu_thread.h>
#include <cell/spurs.h>

#include "sample_config.h"
#include "sample_sync_command.h"

#define HANDLER_THREAD_PRIORITY 1000

extern const CellSpursJobHeader _binary_jqjob_job_sync_command_jobbin2_jobheader;

using namespace cell::Spurs::JobQueue;
using namespace sample_sync_command;

static void
workerEntry(uint64_t arg)
{
	int	ret;
	CellSpursLFQueue*	queue = (CellSpursLFQueue*)(uintptr_t)arg;
	PpuRequest	request;
	do {
		ret = cellSpursLFQueuePop(queue, &request);
		assert(ret == CELL_OK);
		(void)ret;
		if(request.type == REQUEST_TYPE_SIGNAL) {
			//printf("request[%d] received from %x\n", request.type, request.idRequester);
			CellSpursJobQueueWaitingJob*	job = (CellSpursJobQueueWaitingJob*)
				(uintptr_t)request.idRequester;
			ret = cellSpursJobQueueSendSignal(job);
			assert(ret == CELL_OK);
		}
	} while (request.type != REQUEST_TYPE_EXIT);
	sys_ppu_thread_exit(0);
}

static int push_sync_jobs(PortWithDescriptorBuffer<SampleSyncJobDescriptor, NUM_TOTAL_JOBS>* pPort, CellSpursLFQueue* queue, bool useSyncCommand)
{
	int	ret;
	uint8_t*	jobSuspendData = 0;
	jobSuspendData = (uint8_t*)memalign(128, CELL_SPURS_JOBQUEUE_SUSPENDED_JOB_SIZE * 2 * NUM_TAGS);
	assert(jobSuspendData);
	for (unsigned i = 0; i < 2 * NUM_TAGS; i++) {
		__builtin_memset(jobSuspendData + CELL_SPURS_JOBQUEUE_SUSPENDED_JOB_SIZE * i, 0 , 128);
	}

	//E create job

	// setting workarea
	WorkAreaEntry*	workArea = (WorkAreaEntry*)memalign(16, sizeof(WorkAreaEntry) * NUM_SYNC_JOBS * NUM_SYNC_JOBS * 2 * NUM_TAGS);
	assert(workArea);
	__builtin_memset(workArea, 0, sizeof(WorkAreaEntry) * NUM_SYNC_JOBS * NUM_SYNC_JOBS * 2 * NUM_TAGS);

	//E prepareing job descriptors
	SampleSyncJobDescriptor	job[NUM_TAGS][2][NUM_SYNC_JOBS];
	for(unsigned tag = 0; tag < NUM_TAGS; tag++) {
		for(unsigned way = 0; way < 2; way++) {
			for(unsigned i= 0; i < NUM_SYNC_JOBS; i++) {
				__builtin_memset(&job[tag][way][i], 0, sizeof(SampleSyncJobDescriptor));
				CellSpursJobHeader *header = &job[tag][way][i];
				*header  = _binary_jqjob_job_sync_command_jobbin2_jobheader;
				job[tag][way][i].sizeStack = (16 * 1024) >> 4;
				job[tag][way][i].useInOutBuffer = 1;
				job[tag][way][i].sizeInOrInOut = sizeof(WorkAreaEntry) * NUM_SYNC_JOBS;
				job[tag][way][i].sizeOut = 8 * NUM_SYNC_JOBS;
				job[tag][way][i].sizeDmaList = sizeof(SampleSyncJobInputList);

				/* setting up input DMA */
				job[tag][way][i].input.data.asInputList.size = sizeof(WorkAreaEntry) * NUM_SYNC_JOBS;
				job[tag][way][i].input.data.asInputList.eal = (uintptr_t)(workArea + NUM_SYNC_JOBS * (NUM_SYNC_JOBS * (way + tag * 2) + i));
				/* setting up user data */
				job[tag][way][i].myNumber = i;

				/* only the first job has eaSuspendData */
				job[tag][way][i].eaSuspendData = (i == 0) ? (uintptr_t)jobSuspendData + CELL_SPURS_JOBQUEUE_SUSPENDED_JOB_SIZE * (way + tag * 2) : 0;
				job[tag][way][i].eaRequestQueue = (uintptr_t)queue;
				job[tag][way][i].eaOutputWorkArea = (uintptr_t)(workArea + NUM_SYNC_JOBS * NUM_SYNC_JOBS * ((1 - way) + tag * 2));
			}
		}
	}

	/* execute jobs */
	if (useSyncCommand) {
		for (unsigned iteration = 0; iteration < NUM_SYNC_ITERATION; iteration++) {
			for(unsigned way = 0; way < 2; way++) {
				for (unsigned tag = 0; tag < NUM_TAGS; tag++) {
					//E push job
					for(unsigned i= 0; i < NUM_SYNC_JOBS; i++) {
						ret = pPort->copyPushJob(&job[tag][way][i], sizeof(SampleSyncJobDescriptor), (tag + 1), 1);
						assert(ret == CELL_OK);
						(void)ret;
					}
					/* insert SYNC command to guarantee the completion */
					ret = pPort->pushSync((0x01 << (tag + 1)));
					assert(ret == CELL_OK);
					//way = (way == 0) ? 1 : 0;
				}
			}
		}
		ret = pPort->sync();
		assert(ret == CELL_OK);
	} else {
		for (unsigned iteration = 0; iteration < NUM_SYNC_ITERATION; iteration++) {
			for(unsigned way = 0; way < 2; way++) {
				//E push job
				for (unsigned tag = 0; tag < NUM_TAGS; tag++) {
					for(unsigned i= 0; i < NUM_SYNC_JOBS; i++) {
						ret = pPort->copyPushJob(&job[tag][way][i], sizeof(SampleSyncJobDescriptor), 0, 1);
						assert(ret == CELL_OK);
					}
				}
				/* waiting for completion before submitting next jobs */
				ret = pPort->sync();
				assert(ret == CELL_OK);
				//way = (way == 0) ? 1 : 0;
			}
		}
	}

	/* release resources */
	free(workArea);
	free(jobSuspendData);

	return CELL_OK;
}

int sample_main(cell::Spurs::Spurs* spurs)
{
	int	ret;

	//E create jobQueue
	JobQueue<JOB_QUEUE_DEPTH> *pJobQueue;
	pJobQueue = (JobQueue<JOB_QUEUE_DEPTH> *)memalign(CELL_SPURS_JOBQUEUE_ALIGN, sizeof(JobQueue<JOB_QUEUE_DEPTH>));
	assert(pJobQueue);
	ret = JobQueue<JOB_QUEUE_DEPTH>::create(pJobQueue,  spurs, "SampleJobQueue", NUM_JQ_SPU, CELL_SPURS_MAX_PRIORITY-1, NUM_MAX_GRAB);
	assert(ret == CELL_OK);
	(void)ret;

	PortWithDescriptorBuffer<SampleSyncJobDescriptor, NUM_TOTAL_JOBS> *pPort = new PortWithDescriptorBuffer<SampleSyncJobDescriptor, NUM_TOTAL_JOBS>();
	pPort->initialize(pJobQueue);

	/* create request queue */
	CellSpursLFQueue*	queue =
		(CellSpursLFQueue*)memalign(CELL_SPURS_LFQUEUE_ALIGN, CELL_SPURS_LFQUEUE_SIZE);
	assert(queue);
	memset(queue, 0, 128);

	uint8_t*	queueBuffer  = (uint8_t*)memalign(16, 16 * 16);
	assert(queueBuffer);
	ret = cellSpursLFQueueInitializeIWL(spurs, queue, queueBuffer, 16, 16, CELL_SPURS_LFQUEUE_ANY2ANY);
	assert(ret == CELL_OK);

	ret = cellSpursLFQueueAttachLv2EventQueue(queue);
	assert(ret == CELL_OK);

	/* create server ppu thread */
	sys_ppu_thread_t	thread;
	ret = sys_ppu_thread_create(&thread, workerEntry, (uintptr_t)queue, HANDLER_THREAD_PRIORITY, 4 * 1024, SYS_PPU_THREAD_CREATE_JOINABLE, "PPU SERVER");
	assert(ret == CELL_OK);

	/* 1. sync using sync() method */
	printf("*** sync with sync() method \n");
	push_sync_jobs(pPort, queue, false);

	/* 2. sync using SYNC command */
	printf("*** sync with SYNC command with multiple tags\n");
	push_sync_jobs(pPort, queue, true);

	/* send exit request to the worker thread */
	PpuRequest	request;
	request.type = REQUEST_TYPE_EXIT;
	ret = cellSpursLFQueuePush(queue, &request);
	assert(ret == CELL_OK);

	/* join ppu thread */
	uint64_t	retVal;
	ret = sys_ppu_thread_join(thread, &retVal);
	assert(ret == CELL_OK);
	ret = cellSpursLFQueueDetachLv2EventQueue(queue);
	assert(ret == CELL_OK);
	free(queueBuffer);
	free(queue);

	ret = pPort->finalize();
	assert(ret == CELL_OK);
	delete pPort;

	//E shutdown jobQueue
	ret = pJobQueue->shutdown();
	assert(ret == CELL_OK);
	int	exitCode;
	ret = pJobQueue->join(&exitCode);
	assert(ret == CELL_OK);
	assert(exitCode == CELL_OK);
	free(pJobQueue);

	return CELL_OK;
}
