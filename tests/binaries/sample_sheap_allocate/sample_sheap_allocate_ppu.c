/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spu_printf.h>
#include <sys/spu_initialize.h>
#include <sys/spu_thread.h>
#include <sys/spu_thread_group.h>
#include <sys/spu_utility.h>
#include <sys/process.h>
SYS_PROCESS_PARAM(1001, 0x10000)

#include <cell/sheap.h>

/* sample libraries */
#include <spu_thread_group_utils.h>

#define MAX_PHYSICAL_SPU      6 
#define MAX_RAW_SPU           0

#define NUM_SPU_THREADS 1
#define SPU_THREAD_GROUP_PRIORITY 100


#define N_PARRAY 512
#define S_BUFFER (1024 * 10)

char buffer_sheap[S_BUFFER ]__attribute__ ((aligned (128)));
unsigned int ans __attribute__ ((aligned (128)));

#define TAG 8

int check_status(UtilSpuThreadGroupStatus *status);

int check_status(UtilSpuThreadGroupStatus *status)
{
	switch(status->cause) {
	case SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT:
		return status->status;
	case SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT:
		for (int i = 0; i < status->nThreads; i++) {
			if (status->threadStatus[i] != CELL_OK) {
				return status->threadStatus[i];
			}
		}
		return CELL_OK;
	case SYS_SPU_THREAD_GROUP_JOIN_TERMINATED:
		return status->status;
	}
	return -1;
}

extern char _binary_spu_sample_sheap_allocate_spu_elf_start[];

int main(void)
{
	UtilSpuThreadGroupInfo groupInfo;
	UtilSpuThreadGroupArg  groupArg;
	UtilSpuThreadGroupStatus  groupStatus;

	int ret;

	ret = spu_printf_initialize(0, NULL);
	if (ret != CELL_OK) {
		printf("## libsheap : spu_printf_initialize FAILED ##\n");
		exit(ret);
	}

	ret = cellSheapInitialize(buffer_sheap, S_BUFFER, TAG);
	if(ret != CELL_OK){
		printf("cellSheapInitialize failed : return code = %d\n", ret);
		printf("## libsheap : sample_sheap_allocate_ppu FAILED ##\n");
		exit(1);
	}
	
	
	groupArg.group_name = "sample_sheap_allocate";
	groupArg.priority   = SPU_THREAD_GROUP_PRIORITY;
	groupArg.nThreads   = NUM_SPU_THREADS;
	groupArg.thread[0].name  = "sample_sheap_allocate_spu";
	groupArg.thread[0].option = SYS_SPU_THREAD_OPTION_NONE;
	groupArg.thread[0].elf   = (sys_addr_t)_binary_spu_sample_sheap_allocate_spu_elf_start;
	groupArg.thread[0].arg[0]= (uint64_t)(uintptr_t)buffer_sheap;
	groupArg.thread[0].arg[1]= (uint64_t)N_PARRAY;
	groupArg.thread[0].arg[2]= (uint64_t)(uintptr_t)&ans;
	

	ret = utilInitializeSpuThreadGroupAll(&groupInfo, &groupArg);
	if (ret != CELL_OK) {
		printf("utilInitializeSpuThreadGroupAll failed: %d\n", ret);
		printf("## libsheap : sample_sheap_allocate_ppu FAILED ##\n");
		exit(ret);
	}

	ret = spu_printf_attach_group(groupInfo.group_id);
	if (ret != CELL_OK) {
		printf("## libsheap : spu_printf_attach_group FAILED ##\n");
		exit(ret);
	}

	ret = utilStartSpuThreadGroup(&groupInfo);
	if (ret != CELL_OK) {
		printf("utilStartSpuThreadGroup failed: %d\n", ret);
		printf("## libsheap : sample_sheap_allocate_ppu FAILED ##\n");
		exit(ret);
	}

	
	ret = utilWaitSpuThreadGroup(&groupInfo, &groupStatus);
	if (ret != CELL_OK) {
		printf("utilWaitSpuThreadGroup failed: %d\n", ret);
		printf("## libsheap : sample_sheap_allocate_ppu FAILED ##\n");
		exit(ret);
	}


	ret = utilFinalizeSpuThreadGroupAll(&groupInfo);
	if (ret != CELL_OK) {
		printf("utilFinalizeSpuThreadGroupAll failed: %d\n", ret);
		printf("## libsheap : sample_sheap_allocate_ppu FAILED ##\n");
		exit(ret);
	}

	printf("%d th PRIME NUMBER is %d\n",N_PARRAY, ans);

	printf("\n");
	printf("## libsheap : sample_sheap_allocate_ppu SUCCEEDED ##\n");

	ret = spu_printf_finalize();
	if (ret != CELL_OK) {
		printf("## libsheap : spu_printf_finalize FAILED ##\n");
		exit(ret);
	}

	printf("Exit PPU\n");
	return 0;
}
