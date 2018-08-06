/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 475.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <cell/spurs.h>
#include "sample_config.h"

#define NJOB 100
#define JOB_TYPE CellSpursJob128

/* embedded SPU job binary symbols */
extern const CellSpursJobHeader _binary_job_job_hello_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_send_event_jobbin2_jobheader;


static void
jobHelloInit(JOB_TYPE *job)
{
	__builtin_memset(job,0,sizeof(JOB_TYPE));

	job->header     = _binary_job_job_hello_jobbin2_jobheader;
}

static void
jobSendEventInit(JOB_TYPE *job, CellSpursEventFlag *ev)
{
	__builtin_memset(job,0,sizeof(JOB_TYPE));

	job->header    = _binary_job_job_send_event_jobbin2_jobheader;
	job->workArea.userData[0] = (uintptr_t)ev;
}


int sample_main(cell::Spurs::Spurs *spurs)
{
	int ret;

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

	JOB_TYPE *jobs         = (JOB_TYPE *)memalign(128, sizeof(JOB_TYPE) * NJOB);
	for (int i = 0; i < NJOB; i++) {
		jobHelloInit(&jobs[i]);
	}

	cell::Spurs::JobList *joblist = (cell::Spurs::JobList*)memalign(16, sizeof(cell::Spurs::JobList));
	joblist->numJobs   = NJOB;
	joblist->sizeOfJob = sizeof(JOB_TYPE);
	joblist->eaJobList = (uintptr_t)jobs;

	JOB_TYPE *jobSendEvent = (JOB_TYPE *)memalign(128, sizeof(JOB_TYPE));
	jobSendEventInit(jobSendEvent, &ev);

	static uint64_t command_list[4];
	command_list[0] = CELL_SPURS_JOB_COMMAND_JOBLIST(joblist);
	command_list[1] = CELL_SPURS_JOB_COMMAND_SYNC;
	command_list[2] = CELL_SPURS_JOB_COMMAND_JOB(jobSendEvent);
	command_list[3] = CELL_SPURS_JOB_COMMAND_END;

	cell::Spurs::JobChainAttribute attr;
	uint8_t priority[8];
	for (unsigned i = 0; i < NUM_SPU; ++i) {
		priority[i] = 8;
	}
	for (unsigned i = NUM_SPU; i < 8; ++i) {
		priority[i] = 0;
	}
	ret = cell::Spurs::JobChainAttribute::initialize( &attr,
							   command_list,
							   sizeof(JOB_TYPE),
							   16, priority,
							   NUM_SPU,	// max contention
							   true,0,1,
							   false,
							   sizeof(JOB_TYPE) > 256 ? sizeof(JOB_TYPE) : 256,
							   0 );
	if (ret) {
		printf("cellSpursJobChainAttributeInitialize failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = attr.setName(SAMPLE_NAME);
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

	ret = jobChain->run();
	if (ret) {
		printf("cellSpursRunJobChain failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	printf("Waiting for the event flag\n");
	uint16_t ev_mask = 1;
	ret = ev.wait(&ev_mask, cell::Spurs::EventFlag::kAnd);
	if (ret) {
		printf("cellSpursEventFlagWait failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = jobChain->shutdown();
	if (ret) {
		printf("cellSpursShutdownJobChain failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = jobChain->join();
	if (ret) {
		printf("cellSpursJoinJobChain failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = ev.detachLv2EventQueue();
	if (ret) {
		printf("cellSpursEventFlagDetachLv2EventQueue failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	free(jobs);
	free(joblist);
	free(jobSendEvent);
	free(jobChain);

	return CELL_OK;
}
