/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2009 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

/* common headers */
#include <stdint.h>
#include <stdlib.h>
#include <spu_intrinsics.h>
#include <cell/spurs.h>
#include <spu_printf.h>
#include <assert.h>

CELL_SPU_LS_PARAM(16*1024, 16*1024);


int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTask;
	(void)argTaskset;

	int ret;
	spu_printf("SPU: Task create task start!\n");

	cell::Spurs::Taskset2Stub taskset2;
	taskset2.setObject(cellSpursGetTasksetAddress());


	CellSpursTaskId tid_subTask1;
	CellSpursTaskId tid_subTask2;
	CellSpursTaskId tid_subTask3;

	
	uint32_t eaSubTask1BinInfo = CELL_SPURS_PPU_SYM(_binary_task_sub_task1_spu_elf_taskbininfo);
	uint32_t eaSubTask2BinInfo = CELL_SPURS_PPU_SYM(_binary_task_sub_task2_spu_elf_taskbininfo);
	uint32_t eaSubTask3BinInfo = CELL_SPURS_PPU_SYM(_binary_task_sub_task3_spu_elf_taskbininfo);

	qword arg = si_from_uint(0);
	int retval;

	ret = taskset2.createTask2(&tid_subTask1, eaSubTask1BinInfo, arg, 0, "subTask1");
	assert(ret == CELL_OK);
	ret = taskset2.joinTask2(tid_subTask1, &retval);
	assert((ret == CELL_OK) && (retval == CELL_OK));

	ret = taskset2.createTask2(&tid_subTask2, eaSubTask2BinInfo, arg, 0, "subTask2");
	assert(ret == CELL_OK);
	ret = taskset2.joinTask2(tid_subTask2, &retval);
	assert((ret == CELL_OK) && (retval == CELL_OK));

	ret = taskset2.createTask2(&tid_subTask3, eaSubTask3BinInfo, arg, 0, "subTask3");
	assert(ret == CELL_OK);
	ret = taskset2.joinTask2(tid_subTask3, &retval);
	assert((ret == CELL_OK) && (retval == CELL_OK));

	return 0;
}

