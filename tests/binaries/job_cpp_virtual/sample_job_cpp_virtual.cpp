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

/* this cannot be less than 256 */
#define MAX_JOB_DESCRIPTOR_SIZE			(sizeof(JobType) > 256 ? sizeof(JobType) : 256)

/* embedded SPU job binary symbols */
extern const CellSpursJobHeader _binary_job_job_virtual_spu_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_shutdown_spu_jobbin2_jobheader;

static
void jobVirtualInit(JobType *job)
{
	/* initialize job descriptor */
	__builtin_memset(job, 0, sizeof(JobType));
	job->header  = _binary_job_job_virtual_spu_jobbin2_jobheader;
}

static
void jobShutdownInit(JobType *job, CellSpursJobChain *jobChain)
{
	/* initialize job descriptor */
	__builtin_memset(job, 0, sizeof(JobType));
	job->header    = _binary_job_job_shutdown_spu_jobbin2_jobheader;
	job->workArea.userData[0] = (uintptr_t)jobChain;
}

int sample_main(cell::Spurs::Spurs *spurs)
{
	int ret;
	static const int COMMAND_LIST_LENGTH = 4;

	/* allocate memory */
	cell::Spurs::JobChain *jobChain = (cell::Spurs::JobChain*)memalign(cell::Spurs::JobChain::kAlign, sizeof(cell::Spurs::JobChain));
	JobType *jobVirtual    = (JobType*)memalign(128, sizeof(JobType));
	JobType *jobShutdown = (JobType*)memalign(128, sizeof(JobType));
	uint64_t* commandList = (uint64_t*)memalign(8, sizeof(uint64_t) * COMMAND_LIST_LENGTH);

	if (jobChain == NULL || jobVirtual == NULL || jobShutdown == NULL || commandList == NULL) {
		printf("memalign failed\n");
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* initialize job descriptors */
	jobVirtualInit(jobVirtual);
	jobShutdownInit(jobShutdown, jobChain);

	/* initialize command list */
	commandList[0] = CELL_SPURS_JOB_COMMAND_JOB(jobVirtual);
	commandList[1] = CELL_SPURS_JOB_COMMAND_SYNC;
	commandList[2] = CELL_SPURS_JOB_COMMAND_JOB(jobShutdown);
	commandList[3] = CELL_SPURS_JOB_COMMAND_END;

	/* initialize job chain attribute */
	const uint8_t priority[8] = { 8, 8, 8, 8, 8, 8, 8, 8 };
	cell::Spurs::JobChainAttribute attr;
	ret = cell::Spurs::JobChainAttribute::initialize(
				   &attr,
				   commandList,
				   sizeof(JobType), // default job descriptor size
				   16,		// max grabbed job
				   priority,
				   1,		// max contention
				   true,	// auto ready count
				   0, 1,	// DMA tags for job streaming
				   true,	// fixed memory allocation
				   MAX_JOB_DESCRIPTOR_SIZE,
				   0		// initial ready count (ignored if auto ready count is true)
			   );
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

	/* create job chain */
	ret = cell::Spurs::JobChain::createWithAttribute(spurs, jobChain, &attr);
	if (ret) {
		printf("cellSpursCreateJobChainWithAttribute failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* let the job chain start */
	ret = jobChain->run();
	if (ret) {
		printf("cellSpursRunJobChain failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* wait for completion of the job chain */
	ret = jobChain->join();
	if (ret) {
		printf("cellSpursJoinJobChain failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* free memory */
	free(jobChain);
	free(jobVirtual);
	free(jobShutdown);
	free(commandList);

	return CELL_OK;
}
