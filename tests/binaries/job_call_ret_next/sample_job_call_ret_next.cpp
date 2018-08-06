/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 475.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <cell/spurs.h>
#include "sample_config.h"

/* embedded SPU job binary symbols */
extern const CellSpursJobHeader _binary_job_job_main_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_sub1_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_sub2_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_next_jobbin2_jobheader;

int sample_main(cell::Spurs::Spurs* spurs)
{
	int ret;

	/* allocate memory */
	cell::Spurs::JobChain* jobChain = (cell::Spurs::JobChain*)memalign(cell::Spurs::JobChain::kAlign, cell::Spurs::JobChain::kSize);
	CellSpursJob128 *jobMain = (CellSpursJob128*)memalign(128, sizeof(CellSpursJob128));
	CellSpursJob128 *jobSub1 = (CellSpursJob128*)memalign(128, sizeof(CellSpursJob128));
	CellSpursJob128 *jobSub2 = (CellSpursJob128*)memalign(128, sizeof(CellSpursJob128));
	CellSpursJob128 *jobNext = (CellSpursJob128*)memalign(128, sizeof(CellSpursJob128));
	cell::Spurs::EventFlag *eventFlag = (cell::Spurs::EventFlag*)memalign(cell::Spurs::EventFlag::kAlign, cell::Spurs::EventFlag::kSize);
	uint64_t *commandListMain = (uint64_t*)memalign(8, sizeof(uint64_t) * 3);
	uint64_t *commandListSub1 = (uint64_t*)memalign(8, sizeof(uint64_t) * 2);
	uint64_t *commandListSub2 = (uint64_t*)memalign(8, sizeof(uint64_t) * 3);
	uint64_t *commandListNext = (uint64_t*)memalign(8, sizeof(uint64_t) * 3);

	if (jobMain == NULL || jobSub1 == NULL || jobSub2 == NULL || jobNext == NULL ||
	    eventFlag == NULL || commandListMain == NULL || commandListSub1 == NULL ||
	    commandListSub2 == NULL || commandListNext == NULL) {
		printf("memalign failed\n");
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* initialize event flag*/
	ret = cell::Spurs::EventFlag::initializeIWL(spurs, eventFlag, cell::Spurs::EventFlag::kClearAuto, cell::Spurs::EventFlag::kSpu2Ppu);
	if (ret) {
		printf("cellSpursEventFlagInitializeIWL failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = eventFlag->attachLv2EventQueue();
	if (ret) {
		printf("cellSpursEventFlagAttachLv2EventQueue failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* initialize job descriptors */
	__builtin_memset(jobMain, 0, sizeof(CellSpursJob128));
	jobMain->header  = _binary_job_job_main_jobbin2_jobheader;

	__builtin_memset(jobSub1, 0, sizeof(CellSpursJob128));
	jobSub1->header  = _binary_job_job_sub1_jobbin2_jobheader;

	__builtin_memset(jobSub2, 0, sizeof(CellSpursJob128));
	jobSub2->header  = _binary_job_job_sub2_jobbin2_jobheader;

	__builtin_memset(jobNext, 0, sizeof(CellSpursJob128));
	jobNext->header  = _binary_job_job_next_jobbin2_jobheader;
	jobNext->workArea.userData[0] = (uintptr_t)eventFlag;

	/* initialize command lists */
	commandListMain[0] = CELL_SPURS_JOB_COMMAND_JOB(jobMain);
	commandListMain[1] = CELL_SPURS_JOB_COMMAND_CALL(commandListSub1);
	commandListMain[2] = CELL_SPURS_JOB_COMMAND_NEXT(commandListNext);

	commandListSub1[0] = CELL_SPURS_JOB_COMMAND_JOB(jobSub1);
	commandListSub1[1] = CELL_SPURS_JOB_COMMAND_CALL(commandListSub2);
	commandListSub1[2] = CELL_SPURS_JOB_COMMAND_RET;

	commandListSub2[0] = CELL_SPURS_JOB_COMMAND_JOB(jobSub2);
	commandListSub2[1] = CELL_SPURS_JOB_COMMAND_RET;

	commandListNext[0] = CELL_SPURS_JOB_COMMAND_SYNC;
	commandListNext[1] = CELL_SPURS_JOB_COMMAND_JOB(jobNext);
	commandListNext[2] = CELL_SPURS_JOB_COMMAND_END;

	/* initialize job chain attribute */
	const uint8_t priority[8] = { 8, 8, 8, 8, 8, 8, 8, 8 };
	cell::Spurs::JobChainAttribute attr;
	ret = cell::Spurs::JobChainAttribute::initialize(
				   &attr,
				   commandListMain,
				   sizeof(CellSpursJob128), // default job descriptor size
				   16,		// max grabbed job
				   priority,
				   1,		// max contention
				   true,	// auto ready count
				   0, 1,	// DMA tags for job streaming
				   false,	// fixed memory allocation
				   256,		// max job descriptor size
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

	/* wait for notification from the jobNext */
	uint16_t ev_mask = 1;
	ret = eventFlag->wait(&ev_mask, cell::Spurs::EventFlag::kAnd);
	if (ret) {
		printf("cellSpursEventFlagWait failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* finish job chain */
	ret = jobChain->shutdown();
	if (ret) {
		printf("cellSpursEventFlagWait failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = jobChain->join();
	if (ret) {
		printf("cellSpursJoinJobChain failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* free resources */
	ret = eventFlag->detachLv2EventQueue();
	if (ret) {
		printf("cellSpursEventFlagDetachLv2EventQueue failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	free(jobChain);
	free(jobMain);
	free(jobSub1);
	free(jobSub2);
	free(jobNext);
	free(eventFlag);
	free(commandListMain);
	free(commandListSub1);
	free(commandListSub2);
	free(commandListNext);

	return CELL_OK;
}
