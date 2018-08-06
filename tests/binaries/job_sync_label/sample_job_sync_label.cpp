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

#define OUT_DMA_TAG1 0
#define OUT_DMA_TAG2 1
#define ITERATION 10
#define JOB_COUNT 32

#define MAX_CONTENTION 2
#define MAX_GRAB 4
#define MAX(x,y) (((x)>(y))?(x):(y))
//#define SYNC_COUNT (MAX_CONTENTION*MAX(5+2,MAX_GRAB))
#define CEIL(x)  ((int)((x)+0.9999f))
#define SYNC_COUNT MAX_CONTENTION*(CEIL(4.0f/(float)MAX_GRAB)*MAX_GRAB+MAX_GRAB)
const uint8_t WORKLOAD_PRIORITIES[8] = {7,7,0,0,  0,0,0,0};
const uint8_t ALT_WORKLOAD_PRIORITIES[8] = {8,8,0,0,  0,0,0,0};


extern const CellSpursJobHeader _binary_job_job_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_alt_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_event_send_jobbin2_jobheader;

static cell::Spurs::EventFlag sEventFlag;

static CellSpursJob64  sJob, sJobAlt, sJobEventSend;

static void
initJobs(void) {
	/*E job initialize */
	__builtin_memset(&sJob, 0, sizeof(CellSpursJob64));
	sJob.header            = _binary_job_job_jobbin2_jobheader;

	/*E jobAlt initialize */
	__builtin_memset(&sJobAlt, 0, sizeof(CellSpursJob64));
	sJobAlt.header         = _binary_job_job_alt_jobbin2_jobheader;

	/*E jobEventSend initialize */
	__builtin_memset(&sJobEventSend, 0, sizeof(CellSpursJob64));
	sJobEventSend.header   = _binary_job_job_event_send_jobbin2_jobheader;
	sJobEventSend.workArea.userData[0] = (uintptr_t)&sEventFlag;
}

int sample_main(cell::Spurs::Spurs *pSpurs)
{
	int ret;

	initJobs();

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

	/*E creat alternative jobchain */
	/*  inifinite job chain loop */
	static uint64_t spAltCommandList[16];
	for(int i = 0; i < 15; i++) {
		spAltCommandList[i] = CELL_SPURS_JOB_COMMAND_JOB(&sJobAlt);
	}
	spAltCommandList[15] = CELL_SPURS_JOB_COMMAND_NEXT(&spAltCommandList[0]);

	cell::Spurs::JobChainAttribute attr;
	ret = cell::Spurs::JobChainAttribute::initialize( &attr,
												spAltCommandList,
												sizeof(CellSpursJob64),
												1, ALT_WORKLOAD_PRIORITIES,
												MAX_CONTENTION,
												true, OUT_DMA_TAG1, OUT_DMA_TAG2,
												false, 256, 0);
	if (ret) {
		printf("cellSpursJobChainAttributeInitialize failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = attr.setName(SAMPLE_NAME "_ALT");
	if (ret) {
		printf("cellSpursJobChainAttributeSetName failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	cell::Spurs::JobChain *pAltJobChain = (cell::Spurs::JobChain *)memalign(cell::Spurs::JobChain::kAlign, sizeof(cell::Spurs::JobChain));

	ret = cell::Spurs::JobChain::createWithAttribute(pSpurs, pAltJobChain, &attr);
	if (ret) {
		printf("cellSpursCreateJobChainWithAttribute failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/*E start alternative jobchain */
	ret = pAltJobChain->run();
	if (ret) {
		printf("cellSpursRunJobChain failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/*E create main jobchain */
	uint64_t *pCommandList;
	pCommandList = (uint64_t *)memalign(128, sizeof(uint64_t) * (ITERATION*(JOB_COUNT+3)+2));
	ret = cell::Spurs::JobChainAttribute::initialize( &attr,
														pCommandList,
														sizeof(CellSpursJob64),
														MAX_GRAB, WORKLOAD_PRIORITIES,
														MAX_CONTENTION,
														true, OUT_DMA_TAG1, OUT_DMA_TAG2,
														false, 256, 0);
	if (ret) {
		printf("cellSpursJobChainAttributeInitialize failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = attr.setName(SAMPLE_NAME "_MAIN");
	if (ret) {
		printf("cellSpursJobChainAttributeSetName failed: %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	cell::Spurs::JobChain *pJobChain = (cell::Spurs::JobChain *)memalign(cell::Spurs::JobChain::kAlign, sizeof(cell::Spurs::JobChain));

	enum TestSet {
		SYNC_TEST = 0,
		SYNC_LABEL_TEST
	};
	const char *test_name[] = {
		"TEST with SYNC command",
		"TEST with SYNC_LABEL command"
	};

	for(int testSet = SYNC_TEST; testSet <= SYNC_LABEL_TEST; testSet++) {
		/*E build command list */
		int cmd_i = 0;
		if (testSet == SYNC_TEST) {
			/*E command list with SYNC command */
			for(int i = 0; i < ITERATION; i++) {
				for(int j = 0; j < JOB_COUNT; j++) {
					pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_JOB(&sJob);
				}
				pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_SYNC;
			}
		} else {
			/*E command list with SYNC_LABEL command */
			for(int i = 0; i < ITERATION; i++) {
				for(int j = 0; j < JOB_COUNT; j++) {
					if (i != 0 && j == SYNC_COUNT) {
						pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_SYNC_LABEL(0);
					}
					pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_JOB(&sJob);
				}
				pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_SET_LABEL(0);
			}
			pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_SYNC_LABEL(0);
		}
		pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_JOB(&sJobEventSend);
		pCommandList[cmd_i++] = CELL_SPURS_JOB_COMMAND_END;

		ret = cell::Spurs::JobChain::createWithAttribute(pSpurs, pJobChain, &attr);
		if (ret) {
			printf("cellSpursCreateJobChainWithAttribute failed: %x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}

		pJobChain->run();
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

	/*E shutdown alternative jobchain */
	ret = pAltJobChain->shutdown();
	if (ret) {
		printf("cellSpursShutdownJobChain failed: %xn", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = pAltJobChain->join();
	if (ret) {
		printf("cellSpursJoinJobChain failed: %xn", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = sEventFlag.detachLv2EventQueue();
	if (ret) {
		printf("sEventFlag.detachLv2EventQueue failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	free(pJobChain);
	free(pAltJobChain);
	free(pCommandList);

	return CELL_OK;
}
