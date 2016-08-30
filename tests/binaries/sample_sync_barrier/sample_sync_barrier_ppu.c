/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#include <sys/process.h>
SYS_PROCESS_PARAM(1001, 0x10000)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/spu_initialize.h>
#include <sys/spu_thread.h>
#include <sys/spu_thread_group.h>
#include <sys/spu_utility.h>


#include <spu_thread_group_utils.h>

#include <cell/sync.h>


#define MAX_PHYSICAL_SPU      4 
#define MAX_RAW_SPU           0

#define NUM_SPU_THREADS 4
#define SPU_THREAD_GROUP_PRIORITY 100


#define BUFFER_SIZE 128

CellSyncBarrier barrier;
extern char _binary_spu_sample_sync_barrier_spu_elf_start[];

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


int main(void)
{
	int ret;
	int i;
	UtilSpuThreadGroupInfo groupInfo;
	UtilSpuThreadGroupArg  groupArg;
	UtilSpuThreadGroupStatus  groupStatus;

	ret = sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);

	if(ret != CELL_OK){
		printf("sys_spu_initialize failed : return code = %d\n", ret);
		printf("...ignored!\n");
	}
	
	/* 1. Initializes a new barrier object */
	ret = cellSyncBarrierInitialize(&barrier, NUM_SPU_THREADS);
	if(ret != CELL_OK){
		printf("cellSyncBarrierInitialize : return code = %d\n", ret);
		printf("## libsync : sample_sync_barrier_ppu FAILED ##\n");
		exit(1);
	}

	/* 2. Allocates work area for each SPU Thread */
	void* buffer = malloc(BUFFER_SIZE * NUM_SPU_THREADS * 2+ 127);
	char* aligned_buffer = (char*)(((uintptr_t)buffer + 127) & ~127);
	

	
	groupArg.group_name = "sample_sync_barrier";
	groupArg.priority   = SPU_THREAD_GROUP_PRIORITY;
	groupArg.nThreads   = NUM_SPU_THREADS;
	for(i=0;i<NUM_SPU_THREADS; i++){
		groupArg.thread[i].name  = "sample_sync_barrier_spu";
		groupArg.thread[i].option = SYS_SPU_THREAD_OPTION_NONE;
		groupArg.thread[i].elf   = (sys_addr_t)_binary_spu_sample_sync_barrier_spu_elf_start;
		groupArg.thread[i].arg[0]= (uint64_t)i;
		groupArg.thread[i].arg[1]= (uint64_t)(uintptr_t)&barrier;
		groupArg.thread[i].arg[2]= (uint64_t)NUM_SPU_THREADS;
		groupArg.thread[i].arg[3]= (uint64_t)(uintptr_t)aligned_buffer;
	}

	/* 3. Executes SPU Thread Group */
	ret = utilInitializeSpuThreadGroupAll(&groupInfo, &groupArg);
	if(ret != CELL_OK){
		printf("utilInitializeSpuThreadGroupAll failed\n");
		printf("## libsync : sample_sync_barrier_ppu FAILED ##\n");
		exit(1);
	}

	ret = utilStartSpuThreadGroup(&groupInfo);
	if(ret != CELL_OK){
		printf("utilStartSpuThreadGroup failed\n");
		printf("## libsync : sample_sync_barrier_ppu FAILED ##\n");
		exit(1);
	}

	ret = utilWaitSpuThreadGroup(&groupInfo, &groupStatus);
	if(ret != CELL_OK){
		printf("utilWaitSpuThreadGroup failed\n");
		printf("## libsync : sample_sync_barrier_ppu FAILED ##\n");
		exit(1);
	}
	
	ret = check_status(&groupStatus);
	if(ret != CELL_OK){
		printf("SPU Thread Group failed. Non-zero value was returned : %d.\n", ret);
		printf("## libsync : sample_sync_barrier_ppu FAILED ##\n");
		exit(1);
	}

	ret = utilFinalizeSpuThreadGroupAll(&groupInfo);
	if(ret != CELL_OK){
		printf("utilFinalizeSpuThreadGroupAll failed\n");
		printf("## libsync : sample_sync_barrier_ppu FAILED ##\n");
		exit(1);
	}

	/* 4. Prints results. */
	for(i=0; i<NUM_SPU_THREADS; i++){
		printf("result(%d): %s\n", i, &aligned_buffer[(i + NUM_SPU_THREADS)*BUFFER_SIZE]);
	}
	
	free(buffer);

	printf("## libsync : sample_sync_barrier_ppu SUCCEEDED ##\n");


	return 0;
}
