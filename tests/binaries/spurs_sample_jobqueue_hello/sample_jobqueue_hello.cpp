/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2010 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cell/spurs.h>

using namespace cell::Spurs::JobQueue;

static const unsigned int JOB_QUEUE_DEPTH = 16;
static const unsigned int NUM_SUBMIT_JOBS = 1;

extern const CellSpursJobHeader _binary_jqjob_job_hello_jobbin2_jobheader;


int sample_main(cell::Spurs::Spurs *spurs)
{
	int ret;

	//E create jobQueue
	JobQueue<JOB_QUEUE_DEPTH> *pJobQueue = (JobQueue<JOB_QUEUE_DEPTH>*)memalign(CELL_SPURS_JOBQUEUE_ALIGN, sizeof(JobQueue<JOB_QUEUE_DEPTH>));
	assert(pJobQueue != NULL);

	ret = JobQueue<JOB_QUEUE_DEPTH>::create(pJobQueue, spurs, "SampleJobQueue", /* numSpus=*/1, /* priority */8, /* maxGrab=*/4);
	assert(ret == CELL_OK);
	(void)ret;

	//E create job
	CellSpursJob256 job;
	__builtin_memset(&job, 0, sizeof(CellSpursJob256));
	job.header   = _binary_jqjob_job_hello_jobbin2_jobheader;

	PortWithDescriptorBuffer<CellSpursJob256, NUM_SUBMIT_JOBS> *pPort = new PortWithDescriptorBuffer<CellSpursJob256, NUM_SUBMIT_JOBS>();

	ret = pPort->initialize(pJobQueue);
	assert(ret == CELL_OK);

	//E submit job
	ret = pPort->copyPushJob(&job, sizeof(CellSpursJob256), 0, true);
	assert(ret == CELL_OK);

	ret = pPort->sync();
	assert(ret == CELL_OK);

	ret = pPort->finalize();
	assert(ret == CELL_OK);

	delete pPort;

	ret = pJobQueue->shutdown();
	assert(ret == CELL_OK);

	int	exitCode;
	ret = pJobQueue->join(&exitCode);
	assert(ret == CELL_OK && exitCode == CELL_OK);

	free(pJobQueue);

	return CELL_OK;
}
