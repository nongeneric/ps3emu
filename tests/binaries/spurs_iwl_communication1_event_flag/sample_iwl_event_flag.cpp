/* SCE CONFIDENTIAL
 PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2010 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <cell/spurs.h>
#include "sample_config.h"
#include "common.h"

static const char* nameTaskset[] = {
	"sample_spurs_iwl_event_flag_taskset1",
	"sample_spurs_iwl_event_flag_taskset2",
	"sample_spurs_iwl_event_flag_taskset3",
	"sample_spurs_iwl_event_flag_taskset4"
};

/* embedded SPU ELF symbols */
extern char _binary_task_iwl_event_flag_set_elf_start[];
extern char _binary_task_iwl_event_flag_wait_elf_start[];

uint8_t context_area[4][CELL_SPURS_TASK_CONTEXT_SIZE_ALL] __attribute__((aligned(CELL_SPURS_TASK_CONTEXT_ALIGN)));

cell::Spurs::Taskset2 taskset[4];
cell::Spurs::EventFlag event_flag[3];

enum {
	PPU2SPU_EVENTFLAG = 0,
	SPU2SPU_EVENTFLAG,
	SPU2PPU_EVENTFLAG
};

Args args __attribute__((aligned(128))) = {
	(uintptr_t)&event_flag[PPU2SPU_EVENTFLAG],
	(uintptr_t)&event_flag[SPU2SPU_EVENTFLAG],
	(uintptr_t)&event_flag[SPU2PPU_EVENTFLAG],
	{0}
};

int sample_main(cell::Spurs::Spurs* spurs)
{
	using namespace cell::Spurs;

	int ret;

	/* register taskset */
	TasksetAttribute2 tasksetAttribute;
	TasksetAttribute2::initialize(&tasksetAttribute);
	tasksetAttribute.argTaskset = (uintptr_t)&args;
	for (unsigned i = 0; i < NUM_SPU; ++i) {
		tasksetAttribute.priority[i] = 8;
	}
	for (unsigned i = NUM_SPU; i < 8; ++i) {
		tasksetAttribute.priority[i] = 0;
	}
	tasksetAttribute.maxContention = NUM_SPU;
	for (unsigned i = 0; i < 4; ++i) {
		tasksetAttribute.name = nameTaskset[i];
		ret = Taskset2::create(spurs, &taskset[i], &tasksetAttribute);
		if (ret) {
			printf("Error: cell::Spurs::Taskset2::create(): %#x\n", ret);
			return ret;
		}
	}

	/* initialize event flag */
	ret = EventFlag::initializeIWL(
		spurs, &event_flag[PPU2SPU_EVENTFLAG],
		EventFlag::kClearAuto, EventFlag::kPpu2Spu);
	assert(ret == CELL_OK);

	ret = EventFlag::initializeIWL(
		spurs, &event_flag[SPU2SPU_EVENTFLAG],
		EventFlag::kClearAuto, EventFlag::kSpu2Spu);
	assert(ret == CELL_OK);

	ret = EventFlag::initializeIWL(
		spurs, &event_flag[SPU2PPU_EVENTFLAG],
		EventFlag::kClearAuto, EventFlag::kSpu2Ppu);
	assert(ret == CELL_OK);

	ret = event_flag[SPU2PPU_EVENTFLAG].attachLv2EventQueue();
	assert(ret == CELL_OK);

	/* creat task */
	TaskAttribute2 taskAttribute;
	TaskAttribute2::initialize(&taskAttribute);
	taskAttribute.sizeContext = CELL_SPURS_TASK_CONTEXT_SIZE_ALL;
	taskAttribute.lsPattern = gCellSpursTaskLsAll;
	CellSpursTaskId id[4];
	for (unsigned i = 0; i < 4; ++i) {
		void* eaElf = i < 2
			? _binary_task_iwl_event_flag_set_elf_start
			: _binary_task_iwl_event_flag_wait_elf_start;
		CellSpursTaskArgument taskArg;
		taskArg.u64[0] = i;
		taskArg.u64[1] = 0;
		taskAttribute.eaContext = (uintptr_t)context_area[i];
		ret = taskset[i].createTask2(&id[i], eaElf, &taskArg, &taskAttribute);
		assert(ret == CELL_OK);
	}

	/* wait until all task becomes ready */

	printf("PPU: wait until all task becomes ready\n");
	uint16_t bits = 0x000f;
	ret = event_flag[SPU2PPU_EVENTFLAG].wait(&bits, EventFlag::kAnd);
	assert(ret == CELL_OK);

	printf("PPU: wake up setter tasks\n");

	/* start setter tasks */
	ret = event_flag[PPU2SPU_EVENTFLAG].set(0x0003);
	assert(ret == CELL_OK);

	/* wait for received signal */
	bits = 0x000c;
	ret = event_flag[SPU2PPU_EVENTFLAG].wait(&bits, CELL_SPURS_EVENT_FLAG_AND);
	assert(ret == CELL_OK);

	printf("PPU: joining all tasks\n");
	for (unsigned i = 0; i < 4; ++i) {
		int exitCode;
		ret = taskset[i].joinTask2(id[i], &exitCode);
		assert(ret == CELL_OK);
	}

	ret = event_flag[SPU2PPU_EVENTFLAG].detachLv2EventQueue();
	assert(ret == CELL_OK);

	printf("PPU: destroying taskset\n");
	for (unsigned i = 0; i < 4; ++i) {
		ret = taskset[i].destroy();
		assert(ret == CELL_OK);
	}

	return 0;
}
