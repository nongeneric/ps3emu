/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 475.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timer.h>
#include <cell/spurs.h>
#include "sample_config.h"

#define NUM_URGENT_CALL 4

/* E job binaries definitions */
extern const CellSpursJobHeader _binary_job_job_main_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_urgent_jobbin2_jobheader;
extern const CellSpursJobHeader _binary_job_job_notify_jobbin2_jobheader;

/* E job descriptor definitions */
CellSpursJob128 JobMain;
CellSpursJob128 JobUrgent;
CellSpursJob128 JobNotify;

int sample_main(cell::Spurs::Spurs *spurs)
{
	int ret;

	/* E create CellSpursEventFlag */
	cell::Spurs::EventFlag *ev = (cell::Spurs::EventFlag *)memalign(cell::Spurs::EventFlag::kAlign,
																	sizeof(cell::Spurs::EventFlag));
	ret = cell::Spurs::EventFlag::initializeIWL(spurs, ev,
												cell::Spurs::EventFlag::kClearAuto,
												cell::Spurs::EventFlag::kSpu2Ppu);
	if (ret) {
		printf("cellSpursEventFlagInitializeIWL failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	ret = ev->attachLv2EventQueue();
	if (ret) {
		printf("cellSpursEventFlagAttachLv2EventQueue failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* E JobMain descriptor setup */
	__builtin_memset(&JobMain, 0, sizeof(CellSpursJob128));
	JobMain.header     = _binary_job_job_main_jobbin2_jobheader;

	/* E JobNotify descriptor setup */
	__builtin_memset(&JobNotify,0,sizeof(CellSpursJob128));
	JobNotify.header     = _binary_job_job_notify_jobbin2_jobheader;

	static volatile struct {
		uint32_t val;
		uint8_t pad[128 - sizeof(uint32_t)];
	} counter __attribute__((aligned(128)));
	counter.val = 0;
	JobNotify.workArea.userData[0] = (uintptr_t)&counter.val;
	JobNotify.workArea.userData[1] = (uintptr_t)ev;
	/* E command list main definition */
	static uint64_t command_list_main[5];
	command_list_main[0] = CELL_SPURS_JOB_COMMAND_JOB(&JobMain);
	command_list_main[1] = CELL_SPURS_JOB_COMMAND_NEXT(command_list_main);
	command_list_main[2] = CELL_SPURS_JOB_COMMAND_SYNC;
	command_list_main[3] = CELL_SPURS_JOB_COMMAND_JOB(&JobNotify);
	command_list_main[4] = CELL_SPURS_JOB_COMMAND_END;

	/* E JobUrgent descriptor setup */
	__builtin_memset(&JobUrgent, 0, sizeof(CellSpursJob128));
	JobUrgent.header     = _binary_job_job_urgent_jobbin2_jobheader;

	/* E urgent command list definition */
	static uint64_t command_list_urgent[4];
	command_list_urgent[0] = CELL_SPURS_JOB_COMMAND_JOB(&JobUrgent);
	command_list_urgent[1] = CELL_SPURS_JOB_COMMAND_SYNC;
	command_list_urgent[2] = CELL_SPURS_JOB_COMMAND_JOB(&JobNotify);
	command_list_urgent[3] = CELL_SPURS_JOB_COMMAND_RET;

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
								  command_list_main,
								  sizeof(CellSpursJob128),
								  16, priority,
								  NUM_SPU, false, 0, 1,
								  false, 256, NUM_SPU
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

	cell::Spurs::JobChain *jobChain = (cell::Spurs::JobChain *)memalign(cell::Spurs::JobChain::kAlign, sizeof(cell::Spurs::JobChain));

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

	/* E wait for a while */
	sys_timer_usleep(1000000);

	/* E add urgent job list call */
	for (int i = 0; i < NUM_URGENT_CALL; i++) {
		ret = jobChain->addUrgentCall(command_list_urgent);
		if (ret == CELL_SPURS_JOB_ERROR_BUSY) {
			i--;
		} else if (ret) {
			printf("cellSpursAddUrgentCall failed : %x\n", ret);
			printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
			abort();
		}
	}
	uint16_t ev_mask = (1 << NUM_URGENT_CALL) - 1;
	ret = ev->wait(&ev_mask, CELL_SPURS_EVENT_FLAG_AND);
	if (ret) {
		printf("cellSpursEventFlagWait failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* E break endless loop */
	command_list_main[1] = CELL_SPURS_JOB_COMMAND_NOP;

	/* E wait for job chain completion */
	ev_mask = 1 << NUM_URGENT_CALL;
	ret = ev->wait(&ev_mask, CELL_SPURS_EVENT_FLAG_AND);
	if (ret) {
		printf("cellSpursEventFlagWait failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}
	//printf("Event received (jobNotify in command_list_main)\n");
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

	free(jobChain);

	/* E detach event queue from CellSpursEventFlag */
	ret = ev->detachLv2EventQueue();
	if (ret) {
		printf("cellSpursEventFlagDetachLv2EventQueue failed : %x\n", ret);
		printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}
	free(ev);

	return CELL_OK;
}
