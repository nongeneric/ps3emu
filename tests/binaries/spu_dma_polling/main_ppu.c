/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

/* Standard C Libraries */
#include <stdint.h>
#include <stdio.h>						/* printf */
#include <stdlib.h>						/* exit */

/* Cell OS SDK */
#include <sys/spu_initialize.h>
#include <sys/process.h>
SYS_PROCESS_PARAM(1001, 0x10000)

/* sample libraries */
#include <spu_thread_group_utils.h>

#define N_SPU_THREAD 1
#define SPU_THREAD_GROUP_PRIORITY 250

/*
 * three shared buffers
 */
#define BUF_SIZE (1<<20)				/* 1MB */
volatile uint32_t buf[3][BUF_SIZE/sizeof(uint32_t)] __attribute__((aligned(128)));

/* embedded SPU ELF symbols */
extern char _binary_polling_spu_elf_start[];

static void
initGroupArg(UtilSpuThreadGroupArg *groupArg)
{
	groupArg->group_name="A sample SPU thread group";	/* name of SPU thread group */
	groupArg->priority=SPU_THREAD_GROUP_PRIORITY;					/* priority */
	groupArg->nThreads=N_SPU_THREAD;				/* number of threads */
	groupArg->thread[0].name="put to triple buffers";
	groupArg->thread[0].option = SYS_SPU_THREAD_OPTION_NONE;
	groupArg->thread[0].elf = (sys_addr_t)_binary_polling_spu_elf_start;
	groupArg->thread[0].arg[0] =(uintptr_t)buf[0];
	groupArg->thread[0].arg[1] =(uintptr_t)buf[1];
	groupArg->thread[0].arg[2] = (uintptr_t)buf[2];
	groupArg->thread[0].arg[3] = BUF_SIZE;
};

int main(void)
{
	int ret;
	UtilSpuThreadGroupInfo groupInfo;
	UtilSpuThreadGroupStatus groupStatus;
	UtilSpuThreadGroupArg groupArg;
	initGroupArg(&groupArg);

	ret = sys_spu_initialize(6, 0);
	if (ret != CELL_OK) {
		printf("sys_spu_initialize failed: %d\n", ret);
		printf("...but continue!\n");
	}

	ret = utilInitializeSpuThreadGroupAll(&groupInfo, &groupArg);
	if (ret != CELL_OK) {
		printf("utilInitializeSpuThreadGroupAll failed: %d\n", ret);
		printf("## libdma : sample_polling FAILED ##\n");
		exit(ret);
	}
	ret = utilStartSpuThreadGroup(&groupInfo);
	if (ret != CELL_OK) {
		printf("utilStartSpuThreadGroup failed: %d\n", ret);
		printf("## libdma : sample_polling FAILED ##\n");
		exit(ret);
	}
	ret = utilWaitSpuThreadGroup(&groupInfo, &groupStatus);
	if (ret != CELL_OK) {
		printf("utilWaitSpuThreadGroup failed: %d\n", ret);
		printf("## libdma : sample_polling FAILED ##\n");
		exit(ret);
	}


	ret = utilFinalizeSpuThreadGroupAll(&groupInfo);
	if (ret != CELL_OK) {
		printf("utilFinalizeSpuThreadGroupAll failed: %d\n", ret);
		printf("## libdma : sample_polling FAILED ##\n");
		exit(ret);
	}

	/*
	 * check the result
	 */
	for (unsigned int i = 0; i < 3; i++) {
		for (unsigned int j = 0; j < BUF_SIZE/sizeof(uint32_t); j++) {
			uint32_t expected = (j >> 5) + 1;		/* par 128 bytes */
			if (buf[i][j] != expected) {
				fprintf(stderr, 
						"FAILURE: buf[%d][%d]:0x%08x != expected:0x%08x\n", 
						i, j, buf[i][j], expected);
				printf("## libdma : sample_dma_polling FAILED ##\n");
				return 1;
			}
		}
	}

	printf("## libdma : sample_dma_polling SUCCEEDED ##\n");
	return 0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
