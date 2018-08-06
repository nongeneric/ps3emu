/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 475.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <cell/spurs.h>
#include "sample_config.h"

typedef CellSpursJob128 JobType;

/* embedded SPU job binary symbols */
extern const CellSpursJobHeader _binary_job_job_hello_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_hello_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_send_event_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_send_event_jobbin2_jobheader;


static void
jobHelloInit(JobType *job)
{
	__builtin_memset(job,0,sizeof(JobType));

	job->header    = _binary_job_job_hello_jobbin2_jobheader;
}

static void
jobSendEventInit(JobType *job, CellSpursEventFlag *ev)
{
	__builtin_memset(job,0,sizeof(JobType));

	job->header      = _binary_job_job_send_event_jobbin2_jobheader;
	job->workArea.userData[0] = (uintptr_t)ev;
}

typedef struct Worker
{
	CellSpurs* spurs;
	sys_event_queue_t queue;
	cell::Spurs::EventFlag *ev;
	JobType *hello;
	JobType *sendEvent;
	uint64_t *command_list;
	cell::Spurs::JobGuard* jobGuard;
	cell::Spurs::JobChain *jobChain;
} Worker;


static
int initializeWorker(Worker* self, CellSpurs* spurs)
{
	int ret;

	self->spurs = spurs;

	self->ev = (cell::Spurs::EventFlag *)memalign(cell::Spurs::EventFlag::kAlign, sizeof(cell::Spurs::EventFlag));
	ret = cell::Spurs::EventFlag::initializeIWL(spurs, self->ev,
												cell::Spurs::EventFlag::kClearAuto,
												cell::Spurs::EventFlag::kSpu2Ppu);
	if (ret) {
		printf("cellSpursEventFlagInitializeIWL : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = self->ev->attachLv2EventQueue();
	if (ret) {
		printf("cellSpursEventFlagAttachLv2EventQueue : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	self->hello    = (JobType*)memalign(16, sizeof(JobType));
	jobHelloInit(self->hello);

	self->sendEvent = (JobType*)memalign(16, sizeof(JobType));
	jobSendEventInit(self->sendEvent, self->ev);

	self->jobGuard = (cell::Spurs::JobGuard*)memalign(128, sizeof(CellSpursJobGuard));

	self->command_list  = (uint64_t*)memalign(8, sizeof(uint64_t) * 5 );

	self->command_list[0] = CELL_SPURS_JOB_COMMAND_GUARD(self->jobGuard);
	self->command_list[1] = CELL_SPURS_JOB_COMMAND_JOB(self->hello);
	self->command_list[2] = CELL_SPURS_JOB_COMMAND_SYNC;
	self->command_list[3] = CELL_SPURS_JOB_COMMAND_JOB(self->sendEvent);
	self->command_list[4] = CELL_SPURS_JOB_COMMAND_NEXT(&self->command_list[0]);

	cell::Spurs::JobChainAttribute attr;
	uint8_t priority[8];
	for (unsigned i = 0; i < NUM_SPU; ++i) {
		priority[i] = 8;
	}
	for (unsigned i = NUM_SPU; i < 8; ++i) {
		priority[i] = 0;
	}
	ret = cell::Spurs::JobChainAttribute::initialize(&attr,
													 self->command_list,
													 sizeof(JobType),
													 16, priority,
													 NUM_SPU, // max contention
													 true,0,1,
													 false, sizeof(JobType) > 256 ? sizeof(JobType) : 256, 0
		);

	ret = attr.setName(SAMPLE_NAME);
	if (ret) {
		printf("cellSpursJobChainAttributeSetName : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	self->jobChain = (cell::Spurs::JobChain *)memalign(128, sizeof(CellSpursJobChain));

	ret = cell::Spurs::JobChain::createWithAttribute(self->spurs, self->jobChain, &attr);
	if (ret) {
		printf("cellSpursCreateJobChainWithAttribute : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = cell::Spurs::JobGuard::initialize(self->jobChain,	self->jobGuard, 1, NUM_SPU, 1);
	if (ret) {
		printf("cellSpursJobGuardInitialize : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = self->jobChain->run();
	if (ret) {
		printf("cellSpursRunJobChain : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	return 0;
}


static
int kickWorker(Worker* self)
{
	int ret;

	ret = self->jobGuard->notify();
	if (ret) {
		printf("cellSpursJobGuardNotify : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}
	return 0;
}


static
int waitWorker(Worker* self)
{
	int ret;
	uint16_t ev_mask = 1;
	ret = self->ev->wait(&ev_mask, cell::Spurs::EventFlag::kAnd);
	if (ret) {
		printf("cellSpursEventFlagWait : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}
	return 0;
}


static
int finalizeWorker(Worker* self)
{
	int ret;

	ret = self->jobChain->shutdown();
	if (ret) {
		printf("cellSpursShutdownJobChain : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = self->jobChain->join();
	if (ret) {
		printf("cellSpursJoinJobChain : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = self->ev->detachLv2EventQueue();
	if (ret) {
		printf("cellSpursEventFlagDetachLv2EventQueue : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	free(self->ev);
	free(self->command_list);
	free(self->hello);
	free(self->sendEvent);
	free(self->jobGuard);
	free(self->jobChain);

	return 0;
}


int sample_main(cell::Spurs::Spurs* spurs)
{
	int ret;

	static Worker worker;
	ret = initializeWorker(&worker, spurs);
	if (ret) {
		printf("initializeWorker failed\n");
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	for (int i = 0; i < 20; i++){
		ret = kickWorker(&worker);
		if (ret) {
			printf("kickWorker failed\n");
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}

		ret = waitWorker(&worker);
		if (ret) {
			printf("waitWorker failed\n");
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}
	}

	ret = finalizeWorker(&worker);
	if (ret) {
		printf("finalizeWorker failed\n");
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	return CELL_OK;
}
