/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 475.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sys_time.h>
#include <cell/spurs.h>
#include "sample_config.h"
#include "common.h"

#define OUT_DMA_TAG1 0
#define OUT_DMA_TAG2 1
#define JOB_COUNT 256

#define MAX_GRAB 16

extern const CellSpursJobHeader _binary_job_job_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_w_restart_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_event_send_jobbin2_jobheader;


static cell::Spurs::EventFlag sEventFlag;

int sample_main(cell::Spurs::Spurs *pSpurs)
{
	int ret;

	enum TestSet {
		DMA_IN_JOB_TEST = 0,
		DMA_IN_JOB_W_RESTART_TEST
	};
	const char *test_name[] = {
		"DMA in job TEST",
		"DMA in job w/Restart TEST"
	};

	ret = cell::Spurs::EventFlag::initializeIWL(pSpurs, &sEventFlag,
												cell::Spurs::EventFlag::kClearAuto, cell::Spurs::EventFlag::kSpu2Ppu);
	if (ret) {
		printf("cell::Spurs::EventFlag::initializeIWL failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}
	ret = sEventFlag.attachLv2EventQueue();
	if (ret) {
		printf("sEventFlag.attachLv2EventQueue failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}


	/*E create main jobchain */
	uint64_t *pCommandList;
	pCommandList = (uint64_t *)memalign(128, sizeof(uint64_t) * (JOB_COUNT+3));

	cell::Spurs::JobChain *pJobChain = (cell::Spurs::JobChain *)memalign(cell::Spurs::JobChain::kAlign, sizeof(cell::Spurs::JobChain));

	static CellSpursJob256  sJob, sJobEventSend;
	/*E job initialize */
	__builtin_memset(&sJob, 0, sizeof(CellSpursJob256));
	sJob.header.sizeInOrInOut = sizeof(InputData);

	static InputData sInput;
	static LinkedList sLinkedList[4];
	/*E initialize input data */
	__builtin_memset(&sInput, 0, sizeof(InputData));
	sLinkedList[0].mData  = 1;
	sLinkedList[0].mpNext = &sLinkedList[1];
	sLinkedList[1].mData  = 2;
	sLinkedList[1].mpNext = &sLinkedList[2];
	sLinkedList[2].mData  = 3;
	sLinkedList[2].mpNext = &sLinkedList[3];
	sLinkedList[3].mData  = 4;
	sLinkedList[3].mpNext = 0;
	sInput.mEaSecondData  = (uintptr_t)&sLinkedList[0];

	/*E generate DMA list */
	unsigned size = sizeof(InputData), tsize;
	int list_count = 0;
	for(uint32_t ea = (uintptr_t)&sInput;
		(tsize = (size < 16*1024) ? size : 16*1024);
		ea += tsize, size -= tsize)
	{
		sJob.workArea.dmaList[list_count++] = (uint64_t)ea | ((uint64_t)tsize << 32);
	}
	sJob.header.sizeDmaList = 8 * list_count;

	/*E jobEventSend initialize */
	__builtin_memset(&sJobEventSend, 0, sizeof(CellSpursJob256));
	__builtin_memcpy(sJobEventSend.header.binaryInfo, _binary_job_job_event_send_jobbin2_jobheader.binaryInfo,
					 sizeof(_binary_job_job_event_send_jobbin2_jobheader.binaryInfo));
	sJobEventSend.header.jobType = _binary_job_job_event_send_jobbin2_jobheader.jobType;
	sJobEventSend.workArea.userData[0] = (uintptr_t)&sEventFlag;

	/*E initialize command list */
	int cmd_i = 0;
	for(int i = 0 ; i < JOB_COUNT; i++) {
		pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_JOB(&sJob);
	}
	pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_SYNC;
	pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_JOB(&sJobEventSend);
	pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_END;

	cell::Spurs::JobChainAttribute attr;
	uint8_t priority[8];
	for (unsigned i = 0; i < NUM_SPU; ++i) {
		priority[i] = 8;
	}
	for (unsigned i = NUM_SPU; i < 8; ++i) {
		priority[i] = 0;
	}
	ret = cell::Spurs::JobChainAttribute::initialize( &attr,
												pCommandList,
												sizeof(CellSpursJob256),
												MAX_GRAB, priority,
												NUM_SPU, // max contention
												true, OUT_DMA_TAG1, OUT_DMA_TAG2,
												false, 256, 0);
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

	/*E execute jobs */
	for(int testSet = DMA_IN_JOB_TEST; testSet <= DMA_IN_JOB_W_RESTART_TEST; testSet+=1) {
		/*E setup job binary and job type */
		if (testSet == DMA_IN_JOB_TEST) {
			/*E job without stall and restart */
			__builtin_memcpy(sJob.header.binaryInfo, _binary_job_job_jobbin2_jobheader.binaryInfo,
							 sizeof(_binary_job_job_jobbin2_jobheader.binaryInfo));
			sJob.header.jobType  = _binary_job_job_jobbin2_jobheader.jobType;
		} else {
			/*E job with stall and restart */
			__builtin_memcpy(sJob.header.binaryInfo, _binary_job_job_w_restart_jobbin2_jobheader.binaryInfo,
							 sizeof(_binary_job_job_w_restart_jobbin2_jobheader.binaryInfo));
			sJob.header.jobType    = _binary_job_job_w_restart_jobbin2_jobheader.jobType | CELL_SPURS_JOB_TYPE_STALL_SUCCESSOR;
		}

		ret = cell::Spurs::JobChain::createWithAttribute(pSpurs, pJobChain, &attr);
		if (ret) {
			printf("cellSpursCreateJobChainWithAttribute failed: %x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}

		ret = pJobChain->run();
		if (ret) {
			printf("cellSpursRunJobChain failed: %x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}

		system_time_t start = sys_time_get_system_time();

		uint16_t ev_mask = 1;
		ret = sEventFlag.wait(&ev_mask, CELL_SPURS_EVENT_FLAG_AND);
		if (ret) {
			printf("cellSpursEventFlagWait failed: %x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}
		system_time_t end = sys_time_get_system_time();
		printf("%s\n"
			   "  elapsed time = %u(us)\n", test_name[testSet], 0/*(unsigned)(end - start)*/);

		ret = pJobChain->shutdown();
		if (ret) {
			printf("cellSpursShutdownJobChain failed: %x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}

		ret = pJobChain->join();
		if (ret) {
			printf("cellSpursJoinJobChain failed: %x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}
	}

	ret = sEventFlag.detachLv2EventQueue();
	if (ret) {
		printf("sEventFlag.detachLv2EventQueue failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	free(pJobChain);
	free(pCommandList);

	return CELL_OK;
}
