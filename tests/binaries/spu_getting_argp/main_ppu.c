/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

/* Standard C Libraries */
#include <stdint.h>
#include <stdio.h>						/* printf */
#include <stdlib.h>						/* exit */
#include <string.h>						/* memcmp */

/* Cell OS SDK */
#include <sys/spu_initialize.h>
#include <sys/process.h>
SYS_PROCESS_PARAM(1001, 0x10000)

/* sample libraries */
#include <spu_thread_group_utils.h>

#define N_SPU_THREAD 1
#define SPU_THREAD_GROUP_PRIORITY 250

/*
 * test buffers
 */
#define BUF_SIZE (1<<20)				/* 1MB */
char buf0[BUF_SIZE] __attribute__((aligned(128)));
char buf1[BUF_SIZE] __attribute__((aligned(128)));

/* struct argument */
#include "shared_types.h"

volatile struct argument fromSpu;

static void
initToSpu(struct argument *toSpu)
{
	toSpu->returnAddress = (uintptr_t)&fromSpu;
	toSpu->frameBuffer0  = (uintptr_t)buf0;
	toSpu->frameBuffer1  = (uintptr_t)buf1;
	toSpu->frameSize     = BUF_SIZE;
	toSpu->nDecoder      = N_SPU_THREAD;
	toSpu->control       = 0;
}

/* embedded SPU ELF symbols */
extern char _binary_getting_argp_spu_elf_start[];

static void
initGroupArg(UtilSpuThreadGroupArg *groupArg, struct argument *toSpu)
{
	groupArg->group_name="A sample SPU thread group";	/* name of SPU thread group */
	groupArg->priority=SPU_THREAD_GROUP_PRIORITY;					/* priority */
	groupArg->nThreads=N_SPU_THREAD;				/* number of threads */

	groupArg->thread[0].name="get argp, then put argp";
	groupArg->thread[0].option = SYS_SPU_THREAD_OPTION_NONE;
	groupArg->thread[0].elf =(sys_addr_t)_binary_getting_argp_spu_elf_start;
	groupArg->thread[0].arg[0] =(uintptr_t)toSpu;
	groupArg->thread[0].arg[1] =0;
	groupArg->thread[0].arg[2] =0;
	groupArg->thread[0].arg[3] =0;
};

int main(void)
{
	int ret;
	UtilSpuThreadGroupInfo groupInfo;
	UtilSpuThreadGroupStatus groupStatus;
	UtilSpuThreadGroupArg groupArg;
	static struct argument toSpu;

	initToSpu(&toSpu);
	initGroupArg(&groupArg,&toSpu);
	

	ret = sys_spu_initialize(6, 0);
	if (ret != CELL_OK) {
		printf("sys_spu_initialize failed: %d\n", ret);
		printf("...but continue!\n");
	}

	ret = utilInitializeSpuThreadGroupAll(&groupInfo, &groupArg);
	if (ret != CELL_OK) {
		printf("utilInitializeSpuThreadGroupAll failed: %d\n", ret);
		printf("## libdma : sample_getting_argp FAILED ##\n");
		exit(ret);
	}
	ret = utilStartSpuThreadGroup(&groupInfo);
	if (ret != CELL_OK) {
		printf("utilStartSpuThreadGroup failed: %d\n", ret);
		printf("## libdma : sample_getting_argp FAILED ##\n");
		exit(ret);
	}
	ret = utilWaitSpuThreadGroup(&groupInfo, &groupStatus);
	if (ret != CELL_OK) {
		printf("utilWaitSpuThreadGroup failed: %d\n", ret);
		printf("## libdma : sample_getting_argp FAILED ##\n");
		exit(ret);
	}


	ret = utilFinalizeSpuThreadGroupAll(&groupInfo);
	if (ret != CELL_OK) {
		printf("utilFinalizeSpuThreadGroupAll failed: %d\n", ret);
		printf("## libdma : sample_getting_argp FAILED ##\n");
		exit(ret);
	}

	/*
	 * check the result
	 */
	if (memcmp(&toSpu, (const void*)&fromSpu, sizeof(struct argument)) == 0) {
		printf("SUCCESS: fromSpu is the same as toSpu\n");
		printf("## libdma : sample_dma_getting_argp SUCCEEDED ##\n");
	} else {
		printf("FAILURE: fromSpu is different from toSpu\n");

		printf("\n"
			   "toSpu.returnAddress=0x%llx\n"
			   "toSpu.frameBuffer0 =0x%llx\n"
			   "toSpu.frameBuffer1 =0x%llx\n"
			   "toSpu.frameSize    =0x%x\n"
			   "toSpu.nDecoder     =%d\n"
			   "toSpu.control      =%d\n",
			   toSpu.returnAddress,
			   toSpu.frameBuffer0,
			   toSpu.frameBuffer1,
			   toSpu.frameSize,
			   toSpu.nDecoder,
			   toSpu.control);

		printf("\n"
			   "fromSpu.returnAddress=0x%llx\n"
			   "fromSpu.frameBuffer0 =0x%llx\n"
			   "fromSpu.frameBuffer1 =0x%llx\n"
			   "fromSpu.frameSize    =0x%x\n"
			   "fromSpu.nDecoder     =%d\n"
			   "fromSpu.control      =%d\n",
			   fromSpu.returnAddress,
			   fromSpu.frameBuffer0,
			   fromSpu.frameBuffer1,
			   fromSpu.frameSize,
			   fromSpu.nDecoder,
			   fromSpu.control);
		printf("## libdma : sample_dma_getting_argp FAILED ##\n");
	}

	return 0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */