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

#define N_JOBS 128


/*  counter variable */
typedef struct {
	uint64_t n;
	uint64_t dummy;
} __attribute__((aligned(16))) Counter;

/*  Read/Write Buffer for a SPURS Job (JobDataTransfer)  */
static Counter srcBuffer[N_JOBS]  __attribute__((aligned(16)));
static Counter dstBuffer[N_JOBS]  __attribute__((aligned(16)));


/*  SPURS Job Binaries  */
extern const CellSpursJobHeader _binary_job_job_count_up_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_send_event_jobbin2_jobheader;

/*  SPURS Job Descriptor  */
static CellSpursJob128 JobSendEvent;


/*
 *    SPURS Job Descriptor Setting Functions
 */

static int
initializeJobCountUp(CellSpursJob128 *job, Counter* src, Counter* dst)
{
	assert(((uintptr_t)dst % 16) == 0);
	__builtin_memset(job, 0, sizeof(CellSpursJob128));

	job->header   = _binary_job_job_count_up_jobbin2_jobheader;

	job->header.useInOutBuffer = 1;
	job->header.sizeInOrInOut = sizeof(Counter);
	job->header.sizeDmaList  = sizeof(uint64_t) * 1;

	int ret;
	ret = cellSpursJobGetInputList(&job->workArea.dmaList[0], sizeof(Counter), (uintptr_t)src);
	job->workArea.userData[1] = (uint64_t)(uintptr_t)dst;

	return ret;
}


static void
initializeJobSendEvent(CellSpursJob128 *job, cell::Spurs::EventFlag *ev)
{
	__builtin_memset(job, 0, sizeof(CellSpursJob128));

	job->header      = _binary_job_job_send_event_jobbin2_jobheader;


	job->workArea.userData[0] = (uintptr_t)ev;
}


/*
 *    sample SPURS Job In/Out DMA
 */

int sample_main(cell::Spurs::Spurs *spurs)
{
	int ret;

	/*
	 *  Create an CellSpursEventFlag
	 *  to receive events from SPURS jobs.
	 */
	static cell::Spurs::EventFlag ev;

	ret =cell::Spurs::EventFlag::initializeIWL(spurs, &ev,
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

	/*
	 *  Create SPURS Job Command Lists
	 */
	initializeJobSendEvent(&JobSendEvent, &ev);



	/*
	 *  Create a SPURS Command List
	 *  to finish the execution of the SPURS Job Chain
	 */
	static CellSpursJob128 countupJobArray[N_JOBS];
	for(int i=0;i<N_JOBS;i++){
		srcBuffer[i].n = i;
		ret = initializeJobCountUp(&countupJobArray[i], &srcBuffer[i], &dstBuffer[i]);
		if (ret) {
			printf("Invalid InOut DMA list specified : %#x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}
	}

	static cell::Spurs::JobList sampleJobList;
	sampleJobList.eaJobList = (uintptr_t)countupJobArray;
	sampleJobList.sizeOfJob = sizeof(CellSpursJob128);
	sampleJobList.numJobs    = N_JOBS;

	static uint64_t command_list[4];
	int c=0;
	command_list[c++] = CELL_SPURS_JOB_COMMAND_JOBLIST(&sampleJobList);
	command_list[c++] = CELL_SPURS_JOB_COMMAND_SYNC;
	command_list[c++] = CELL_SPURS_JOB_COMMAND_JOB(&JobSendEvent);
	command_list[c++] = CELL_SPURS_JOB_COMMAND_END;

	assert(c==4);
	/*
	 *  Create SPURS Job Chain
	 */
	cell::Spurs::JobChainAttribute attr;
	uint8_t priority[8];
	for (unsigned i = 0; i < NUM_SPU; ++i) {
		priority[i] = 8;
	}
	for (unsigned i = NUM_SPU; i < 8; ++i) {
		priority[i] = 0;
	}
	ret = cell::Spurs::JobChainAttribute::initialize(
								  &attr,
								  command_list,
								  sizeof(CellSpursJob128),
								  16, priority,
								  NUM_SPU, // max contention
								  true, 0, 1,
								  false, 256, 0
								 );
	if (ret) {
		printf("cellSpursJobChainAttributeInitialize failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = attr.setName( SAMPLE_NAME);
	if (ret) {
		printf("cellSpursJobChainAttributeSetName failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	cell::Spurs::JobChain *jobChain = (cell::Spurs::JobChain*)memalign(cell::Spurs::JobChain::kAlign, sizeof(cell::Spurs::JobChain));
	ret = cell::Spurs::JobChain::createWithAttribute(spurs, jobChain, &attr);
	if (ret) {
		printf("cellSpursCreateJobChainWithAttribute failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/*
	 *  Ready count has no meaning since auto set ready count is specified
	 */
	ret = jobChain->run();
	if (ret) {
		printf("cellSpursRunJobChain failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/*
	 *  Wait for an event from job_send_event
	 *  to confirm the completion of the job streaming.
	 */
	uint16_t ev_mask = 1;
	ret = ev.wait(&ev_mask,cell::Spurs::EventFlag::kAnd);
	if (ret){
		printf("cellSpursEventFlagWait failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/*
	 *  print out results;
	 */
	for (int i = 0; i < N_JOBS; i++){
		printf("%08x dstBuffer[%d]=%llx\n", &dstBuffer[i], i, dstBuffer[i].n);
	}

	/*
	 *  Shutdown SPURS Job Chain
	 */
	ret = jobChain->shutdown();
	if (ret) {
		printf("cellSpursShutdownJobChain failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/*
	 *  Join SPURS Job Chain
	 *  Although cellSpursShutdownJobChain() is called,
	 *  a job streaming is not finished immediately.
	 *  Thus, join a chain until it finishes.
	 */
	ret = jobChain->join();
	if (ret) {
		printf("cellSpursJoinJobChain failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	free(jobChain);

	ret = ev.detachLv2EventQueue();
	if (ret) {
		printf("cellSpursEventFlagDetachLv2EventQueue failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	return CELL_OK;
}
