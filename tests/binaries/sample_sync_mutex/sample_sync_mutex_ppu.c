/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 360.001
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


#define MAX_PHYSICAL_SPU      5 
#define MAX_RAW_SPU           0

#define NUM_SPU_THREADS 4
#define SPU_THREAD_GROUP_PRIORITY 200

int count;
CellSyncMutex mutex;
char message[128] __attribute__ ((aligned (128)));
extern char _binary_spu_sample_sync_mutex_spu_elf_start[];

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
	UtilSpuThreadGroupInfo groupInfo;
	UtilSpuThreadGroupArg  groupArg;
	UtilSpuThreadGroupStatus  groupStatus;
	int i;
	int ret;

	ret = sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);

	if(ret != CELL_OK){
		printf("sys_spu_initialize failed : return code = %d\n", ret);
		printf("...but continue!\n");
	}
	
	/* 1. Initializes a new mutex object. */
	ret = cellSyncMutexInitialize(&mutex);
	if(ret != CELL_OK){
		printf("cellSyncMutexInitialize failed : return code = %d\n", ret);
		printf("## libsync : sample_sync_mutex_ppu FAILED ##\n");
		exit(1);
	}
	
	/* 2. Execute and wait sample_sync_mutex_spu */
	groupArg.group_name = "sample_sync_barrier";
	groupArg.priority   = SPU_THREAD_GROUP_PRIORITY;
	groupArg.nThreads   = NUM_SPU_THREADS;
	for(i=0;i<NUM_SPU_THREADS; i++){
		groupArg.thread[i].name  = "sample_sync_mutex_spu";
		groupArg.thread[i].option = SYS_SPU_THREAD_OPTION_NONE;
		groupArg.thread[i].elf   = (sys_addr_t)_binary_spu_sample_sync_mutex_spu_elf_start;
		groupArg.thread[i].arg[0]= (uint64_t)i;
		groupArg.thread[i].arg[1]= (uint64_t)&mutex;
		groupArg.thread[i].arg[2]= (uint64_t)&count;
		groupArg.thread[i].arg[3]= (uint64_t)&message;
	}

	ret = utilInitializeSpuThreadGroupAll(&groupInfo, &groupArg);

	if(ret != CELL_OK){
		printf("utilInitializeSpuThreadGroupAll failed : return code = %d\n", ret);
		printf("## libsync : sample_sync_mutex_ppu FAILED ##\n");
		exit(1);
	}
	ret = utilStartSpuThreadGroup(&groupInfo);

	if(ret != CELL_OK){
		printf("utilInitializeSpuThreadGroupAll failed : return code = %d\n", ret);
		printf("## libsync : sample_sync_mutex_ppu FAILED ##\n");
		exit(1);
	}

	
	ret = utilWaitSpuThreadGroup(&groupInfo, &groupStatus);
	if(ret != CELL_OK){
		printf("utilWaitSpuThreadGroup failed : return code = %d\n", ret);
		printf("## libsync : sample_sync_mutex_ppu FAILED ##\n");
		exit(1);
	}

	ret = check_status(&groupStatus);
	if(ret != CELL_OK){
		printf("SPU Thread Group return non-zero value : %d\n", ret);
		printf("## libsync : sample_sync_mutex_ppu FAILED ##\n");
		exit(1);
	}
	
	ret = utilFinalizeSpuThreadGroupAll(&groupInfo);
	if(ret != CELL_OK){
		printf("utilWaitSpuThreadGroup failed : return code = %d\n", ret);
		printf("## libsync : sample_sync_mutex_ppu FAILED ##\n");
		exit(1);
	}

	printf("count = %d\n", count);
	printf("message : %s\n", message);

	printf("## libsync : sample_sync_mutex_ppu SUCCEEDED ##\n");

	return 0;
}
