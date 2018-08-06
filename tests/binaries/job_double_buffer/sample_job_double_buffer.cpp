/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 475.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cell/spurs.h>
#include "sample_config.h"

#define OUT_DMA_TAG1 0
#define OUT_DMA_TAG2 1
#define SIZE_OF_JOB 64
#define SIZE_OF_JOBLIST 16
#define SIZE_OF_COMLIST 16
#define NUM_JOBS 4
#define SIZE_OF_UNIFORM 16*1024
#define SIZE_OF_DATA_BUFFER 16*1024*1024
#define ITERATION 16

#define JOB_ADD 0
#define JOB_SUB 1
#define JOB_MUL 2
#define JOB_MOV 3

extern const CellSpursJobHeader _binary_job_job_add_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_sub_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_mul_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_mov_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_send_event_jobbin2_jobheader;

static CellSpursJob128  jobs       [2][SIZE_OF_JOB];
static CellSpursJobList jobList    [2][SIZE_OF_JOBLIST];
static uint64_t         commandList[2][SIZE_OF_COMLIST+5];
static uint8_t          dataBuffers[2][SIZE_OF_DATA_BUFFER] __attribute__((aligned(16)));

static CellSpursJob128 JobSendEvent;
static int32_t uniformData[SIZE_OF_UNIFORM/4] __attribute__((aligned(16)));

static void
updateUniform(void) {
	srand(rand());
	for(int i = 0; i < SIZE_OF_UNIFORM/4; i++) {
		uniformData[i] = rand();
	}
}

static void
printUniform(void) {
	int r = 0;
	for(int i = 0; i < SIZE_OF_UNIFORM/4; i++) {
		r ^= uniformData[i] ^ i;
	}
	printf("uniform = %x\n", r);
}

static void
jobAddInit(CellSpursJob128 *job,
		   uint8_t *in, uint8_t *out, unsigned int size)
{
	__builtin_memset(job, 0, sizeof(CellSpursJob128));

	job->header                  = _binary_job_job_add_jobbin2_jobheader;
	job->header.sizeDmaList      = 8;
	job->header.eaHighInput      = (uint64_t)(uintptr_t)in >> 32;
	job->header.sizeInOrInOut    = size;
	job->header.sizeOut          = size;
	job->header.eaHighCache      = (uint64_t)(uintptr_t)&uniformData >> 32;
	job->header.sizeCacheDmaList = 8;

	// input
	cellSpursJobGetInputList(&job->workArea.dmaList[0], size, (uintptr_t)in);

	// cache
	cellSpursJobGetCacheList(&job->workArea.dmaList[1], SIZE_OF_UNIFORM, (uintptr_t)&uniformData);

	// output
	cellSpursJobGetInputList(&job->workArea.userData[2], size, (uintptr_t)out);
}

static void
jobSubInit(CellSpursJob128 *job,
		   uint8_t *in, uint8_t *out, unsigned int size)
{
	__builtin_memset(job, 0, sizeof(CellSpursJob128));

	job->header                  = _binary_job_job_sub_jobbin2_jobheader;
	job->header.sizeDmaList      = 8;
	job->header.eaHighInput      = (uint64_t)(uintptr_t)in >> 32;
	job->header.sizeInOrInOut    = size;
	job->header.sizeOut          = size;
	job->header.eaHighCache      = (uint64_t)(uintptr_t)&uniformData >> 32;
	job->header.sizeCacheDmaList = 8;

	// input
	cellSpursJobGetInputList(&job->workArea.dmaList[0], size, (uintptr_t)in);

	// cache
	cellSpursJobGetCacheList(&job->workArea.dmaList[1], SIZE_OF_UNIFORM, (uintptr_t)&uniformData);

	// output
	cellSpursJobGetInputList(&job->workArea.userData[2], size, (uintptr_t)out);
}

static void
jobMulInit(CellSpursJob128 *job,
		   uint8_t *in, uint8_t *out, unsigned int size)
{
	__builtin_memset(job, 0, sizeof(CellSpursJob128));

	job->header                  = _binary_job_job_mul_jobbin2_jobheader;
	job->header.sizeDmaList      = 8;
	job->header.eaHighInput      = (uint64_t)(uintptr_t)in >> 32;
	job->header.sizeInOrInOut    = size;
	job->header.sizeOut          = size;
	job->header.eaHighCache      = (uint64_t)(uintptr_t)&uniformData >> 32;
	job->header.sizeCacheDmaList = 8;

	// input
	cellSpursJobGetInputList(&job->workArea.dmaList[0], size, (uintptr_t)in);

	// cache
	cellSpursJobGetCacheList(&job->workArea.dmaList[1], SIZE_OF_UNIFORM, (uintptr_t)&uniformData);

	// output
	cellSpursJobGetInputList(&job->workArea.userData[2], size, (uintptr_t)out);
}

static void
jobMovInit(CellSpursJob128 *job,
		   uint8_t *in, uint8_t *out, unsigned int size)
{
	__builtin_memset(job, 0, sizeof(CellSpursJob128));

	job->header                  = _binary_job_job_mov_jobbin2_jobheader;
	job->header.sizeDmaList      = 8;
	job->header.eaHighInput      = (uint64_t)(uintptr_t)in >> 32;
	job->header.sizeInOrInOut    = size;
	job->header.sizeOut          = size;

	// input
	cellSpursJobGetInputList(&job->workArea.dmaList[0], size, (uintptr_t)in);

	// output
	cellSpursJobGetInputList(&job->workArea.userData[2], size, (uintptr_t)out);
}

static void
jobSendEventInit(CellSpursJob128 *job, CellSpursEventFlag *ev)
{
	__builtin_memset(job,0,sizeof(CellSpursJob128));

	job->header               = _binary_job_job_send_event_jobbin2_jobheader;
	job->workArea.userData[0] = (uintptr_t)ev;
}

static void
initializeDataBuffers(void) {
	__builtin_memset(dataBuffers, 0, SIZE_OF_DATA_BUFFER*2);
}

/* E
 * Create four type jobs in random
 */
static int
createJobs(const int index, const int jobIndex, unsigned *p_dataIndex) {
	int i;
	for(i = jobIndex; i < SIZE_OF_JOB && rand() % 100 > 5 && *p_dataIndex < SIZE_OF_DATA_BUFFER; i++) {
		int job = rand() % NUM_JOBS;
		int size = (rand() % 0x4000)& ~0xf;
		if ((*p_dataIndex)+size > SIZE_OF_DATA_BUFFER) {
			size = SIZE_OF_DATA_BUFFER - (*p_dataIndex);
		}
		switch(job) {
		case JOB_ADD:
			jobAddInit(&jobs[index][i],
					   &dataBuffers[index][*p_dataIndex],
					   &dataBuffers[(index+1)%2][*p_dataIndex], size);
			break;
		case JOB_SUB:
			jobSubInit(&jobs[index][i],
					   &dataBuffers[index][*p_dataIndex],
					   &dataBuffers[(index+1)%2][*p_dataIndex], size);
			break;
		case JOB_MUL:
			jobMulInit(&jobs[index][i],
					   &dataBuffers[index][*p_dataIndex],
					   &dataBuffers[(index+1)%2][*p_dataIndex], size);
			break;
		case JOB_MOV:
			jobMovInit(&jobs[index][i],
					   &dataBuffers[index][*p_dataIndex],
					   &dataBuffers[(index+1)%2][*p_dataIndex], size);
			break;
		}
		*p_dataIndex += size;
	}

	return i;
}

/* E
 * Create joblist in random
 */
static bool
createJobList(const int index, const int joblistIndex, int *p_jobIndex, unsigned *p_dataIndex) {
	int i = createJobs(index, *p_jobIndex, p_dataIndex);
	if (i == *p_jobIndex) return true; /* failed to create joblist */
	jobList[index][joblistIndex].numJobs = i-*p_jobIndex;
	jobList[index][joblistIndex].sizeOfJob = sizeof(CellSpursJob128);
	jobList[index][joblistIndex].eaJobList = (uintptr_t)&jobs[index][*p_jobIndex];
	*p_jobIndex = i;

	return false;
}

/* E
 * Add job or joblist in random
 */
static void
updateCommandList(const int index) {
	int comlistIndex = 4; /* E skip over initial four commands */
	int joblistIndex = 0;
	int jobIndex = 0;
	unsigned dataIndex = 0;
	bool endOfComlist = false;

	do {
		int jobOrJoblist = rand()%2;
		switch(jobOrJoblist) {
		case 0:
			/* E add chunk of jobs */
			if (jobIndex == SIZE_OF_JOB) {
				endOfComlist = true;
				break;
			}
			{
				int nextJobIndex = createJobs(index, jobIndex, &dataIndex);
				if (nextJobIndex == jobIndex) {
					endOfComlist = true;
					break;
				}
				while(jobIndex != nextJobIndex && comlistIndex != SIZE_OF_COMLIST) {
					/* E Add created jobs to command list */
					commandList[index][comlistIndex++] = CELL_SPURS_JOB_COMMAND_JOB(&jobs[index][jobIndex++]);
				}
			}
			if (comlistIndex != SIZE_OF_COMLIST) {
				/* E Add SYNC command to isolate chunk of jobs */
				commandList[index][comlistIndex++] = CELL_SPURS_JOB_COMMAND_SYNC;
			}
			break;
		case 1:
			/* E create joblist */
			if (joblistIndex == SIZE_OF_JOBLIST) {
				endOfComlist = true;
				break;
			}
			endOfComlist = createJobList(index, joblistIndex, &jobIndex, &dataIndex);
			if (!endOfComlist) {
				commandList[index][comlistIndex] = CELL_SPURS_JOB_COMMAND_JOBLIST(&jobList[index][joblistIndex]);
				joblistIndex++;
				comlistIndex++;
			}
			break;
		}
		if (comlistIndex == SIZE_OF_COMLIST) endOfComlist = true;
	} while(rand() % 100 > 5 && !endOfComlist);
	commandList[index][comlistIndex++] = CELL_SPURS_JOB_COMMAND_NEXT(&commandList[(index+1)%2][0]);
}

int sample_main(cell::Spurs::Spurs *spurs)
{
	srand(13);

	int ret;

	initializeDataBuffers();

	static cell::Spurs::EventFlag ev;

	ret = cell::Spurs::EventFlag::initializeIWL(spurs, &ev,
												cell::Spurs::EventFlag::kClearAuto,
												cell::Spurs::EventFlag::kSpu2Ppu);
	if (ret) {
		printf("cellSpursEventFlagInitializeIWL failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = ev.attachLv2EventQueue();
	if (ret) {
		printf("cellSpursEventFlagAttachLv2EventQueue failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	jobSendEventInit(&JobSendEvent, &ev);
	static cell::Spurs::JobGuard guard;
	// JobList-0
	commandList[0][0] = CELL_SPURS_JOB_COMMAND_SYNC;
	commandList[0][1] = CELL_SPURS_JOB_COMMAND_JOB(&JobSendEvent);
	commandList[0][2] = CELL_SPURS_JOB_COMMAND_FLUSH;
	commandList[0][3] = CELL_SPURS_JOB_COMMAND_GUARD(&guard);
	// JobList-1
	commandList[1][0] = CELL_SPURS_JOB_COMMAND_SYNC;
	commandList[1][1] = CELL_SPURS_JOB_COMMAND_JOB(&JobSendEvent);
	commandList[1][2] = CELL_SPURS_JOB_COMMAND_FLUSH;
	commandList[1][3] = CELL_SPURS_JOB_COMMAND_GUARD(&guard);

	cell::Spurs::JobChainAttribute attr;
	uint8_t priority[8];
	for (unsigned i = 0; i < NUM_SPU; ++i) {
		priority[i] = 8;
	}
	for (unsigned i = NUM_SPU; i < 8; ++i) {
		priority[i] = 0;
	}
	ret = cell::Spurs::JobChainAttribute::initialize( &attr,
													  &commandList[0][4],
													  sizeof(CellSpursJob128),
													  16, priority,
													  NUM_SPU, // max contention
													  true, OUT_DMA_TAG1, OUT_DMA_TAG2,
													  false, 256, 0
		);
	if (ret) {
		printf("cellSpursJobChainAttributeInitialize failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = attr.setName(SAMPLE_NAME);
	if (ret) {
		printf("cellSpursJobChainAttributeSetName failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	cell::Spurs::JobChain *jobChain = (cell::Spurs::JobChain*)memalign(cell::Spurs::JobChain::kAlign, sizeof(cell::Spurs::JobChain));

	ret = cell::Spurs::JobChain::createWithAttribute(spurs, jobChain, &attr);
	if (ret) {
		printf("cellSpursCreateJobChainWithAttribute failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	cell::Spurs::JobGuard::initialize(jobChain, &guard, 1, 0, 1 /* E autoReset enabled */);

	updateCommandList(0);
	updateUniform();
	printUniform();
	ret = jobChain->run();
	if (ret) {
		printf("cellSpursRunJobChain failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}
	for (int i = 0; i < ITERATION; i++) {
		if (i) {
			ret = guard.notify();
			if (ret) {
				printf("cellSpursJobGuardNotify failed: %x\n", ret);
				printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
				abort();
			}
			//printf("%d: guardNotify\n", i);
		}

		if (i != ITERATION-1) {
			/* E create next jobs */
			updateCommandList((i+1)%2);
		}

		uint16_t ev_mask = 1;
		ret = ev.wait(&ev_mask, cell::Spurs::EventFlag::kAnd);
		if (ret) {
			printf("cellSpursEventFlagWait failed: %x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}

		/* E
		 * Uniform is updated after notification from jobChain
		 * to make sure all jobs which need uniform data complete
		 */
		updateUniform();
	}

	ret = jobChain->shutdown();
	if (ret) {
		printf("cellSpursShutdownJobChain failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = jobChain->join();
	if (ret) {
		printf("cellSpursJoinJobChain failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = ev.detachLv2EventQueue();
	if (ret) {
		printf("cellSpursEventFlagDetachLv2EventQueue failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	printUniform();

	free(jobChain);

	return CELL_OK;
}
