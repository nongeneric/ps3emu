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
#include <sys/ppu_thread.h>


#include <cell/sync.h>
#include <spu_thread_group_utils.h>

#include "common.h"

#define MAX_PHYSICAL_SPU      4 
#define MAX_RAW_SPU           0

#define NUM_SPU_THREADS 2
#define SPU_THREAD_GROUP_PRIORITY 100

#define PPU_THR_PRIO          2000
#define PPU_THR_STACK_SIZE  0x4000

extern char _binary_spu_sample_sync_lfqueue_spu_elf_start[];

static int check_status(UtilSpuThreadGroupStatus *status)
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

static UtilSpuThreadGroupInfo groupInfo;
static UtilSpuThreadGroupArg  groupArg;

static void sender_entry(uint64_t arg)
{
	CellSyncLFQueue *ppu2spu_queue = (CellSyncLFQueue *)(uintptr_t)arg;
	int ret;

	for(int i = 0; i<ITERATION; i++) {
		uint32_t buf[4] __attribute__((aligned(16)));
		buf[0] = i;
		ret = cellSyncLFQueuePush(ppu2spu_queue, (const void *)buf);
		if(ret != CELL_OK){
			printf("cellSyncLFQueuePush : return code = %d\n", ret);
			printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
			sys_ppu_thread_exit(1);
		}
	}

	sys_ppu_thread_exit(0);
}

static void receiver_entry(uint64_t arg)
{
	CellSyncLFQueue *spu2ppu_queue = (CellSyncLFQueue *)(uintptr_t)arg;
	int ret;

	for(unsigned int i = 0; i<ITERATION; i++) {
		uint32_t buf[4] __attribute__((aligned(16)));
		ret = cellSyncLFQueuePop(spu2ppu_queue, (void *)buf);
		if(ret != CELL_OK){
			printf("cellSyncLFQueuePop : return code = %d\n", ret);
			printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
			sys_ppu_thread_exit(1);
		}
		if (buf[0] != i) {
			printf("data compare error expected=%d, actual=%d\n", i, buf[0]);
			printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
			sys_ppu_thread_exit(1);
		}
	}

	sys_ppu_thread_exit(0);
}

int main(void)
{
	UtilSpuThreadGroupStatus  groupStatus;


	int queue_depth = 4;
	int ret;
	int i;

	ret = sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);

	if(ret != CELL_OK){
		printf("sys_spu_initialize failed : return code = %d\n", ret);
		printf("...but contine!\n");
	}

	/* E 1. Initialize ppu2spu_queue,spu2spu_queue, and spu2ppu_queue */
	CellSyncLFQueue *ppu2spu_queue = (CellSyncLFQueue *)memalign(CELL_SYNC_LFQUEUE_ALIGN, CELL_SYNC_LFQUEUE_SIZE);
	__builtin_memset(ppu2spu_queue, 0, CELL_SYNC_LFQUEUE_SIZE);
	void *ppu2spu_work_space = memalign(128, 16 * queue_depth);
	ret = cellSyncLFQueueInitialize(ppu2spu_queue, ppu2spu_work_space, 16, queue_depth, CELL_SYNC_QUEUE_PPU2SPU, 0);
	if(ret != CELL_OK){
		printf("cellSyncLFQueueInitialize : return code = %d\n", ret);
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}
	//printf("ppu2spu_queue=0x%p\n",ppu2spu_queue);

	CellSyncLFQueue *spu2spu_queue = (CellSyncLFQueue *)memalign(CELL_SYNC_LFQUEUE_ALIGN, CELL_SYNC_LFQUEUE_SIZE);
	__builtin_memset(spu2spu_queue, 0, CELL_SYNC_LFQUEUE_SIZE);
	void *spu2spu_work_space = memalign(128, 16 * queue_depth);
	ret = cellSyncLFQueueInitialize(spu2spu_queue, spu2spu_work_space, 16, queue_depth, CELL_SYNC_QUEUE_SPU2SPU, 0);
	if(ret != CELL_OK){
		printf("cellSyncLFQueueInitialize : return code = %d\n", ret);
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}
	//printf("spu2spu_queue=0x%p\n",spu2spu_queue);

	CellSyncLFQueue *spu2ppu_queue = (CellSyncLFQueue *)memalign(CELL_SYNC_LFQUEUE_ALIGN, CELL_SYNC_LFQUEUE_SIZE);
	__builtin_memset(spu2ppu_queue, 0, CELL_SYNC_LFQUEUE_SIZE);
	void *spu2ppu_work_space = memalign(128, 16 * queue_depth);
	ret = cellSyncLFQueueInitialize(spu2ppu_queue, spu2ppu_work_space, 16, queue_depth, CELL_SYNC_QUEUE_SPU2PPU, 0);
	if(ret != CELL_OK){
		printf("cellSyncLFQueueInitialize : return code = %d\n", ret);
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}
	//printf("spu2ppu_queue=0x%p\n",spu2ppu_queue);

	/* E 2. Execute sample_sync_lfqueue_spu and passes queue1 and queu2 to it */
	groupArg.group_name = "sample_sync_lfqueue";
	groupArg.priority   = SPU_THREAD_GROUP_PRIORITY;
	groupArg.nThreads   = NUM_SPU_THREADS;
	for(i=0;i<NUM_SPU_THREADS; i++){
		if (i==0) {
			groupArg.thread[i].name  = "sample_sync_lfqueue_spu_1";
		} else {
			groupArg.thread[i].name  = "sample_sync_lfqueue_spu_2";
		}
		groupArg.thread[i].option = SYS_SPU_THREAD_OPTION_NONE;
		groupArg.thread[i].elf   = (sys_addr_t)_binary_spu_sample_sync_lfqueue_spu_elf_start;
		groupArg.thread[i].arg[0]= (uint64_t)i;
		if (i==0) {
			groupArg.thread[i].arg[1]= (uint64_t)(uintptr_t)ppu2spu_queue;
			groupArg.thread[i].arg[2]= (uint64_t)(uintptr_t)spu2spu_queue;
		} else {
			groupArg.thread[i].arg[1]= (uint64_t)(uintptr_t)spu2spu_queue;
			groupArg.thread[i].arg[2]= (uint64_t)(uintptr_t)spu2ppu_queue;
		}
		groupArg.thread[i].arg[3]= (uint64_t)0;
	}

	ret = utilInitializeSpuThreadGroupAll(&groupInfo, &groupArg);

	if(ret != CELL_OK){
		printf("utilInitializeSpuThreadGroupAll failed : return code = %d\n", ret);
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}
	ret = utilStartSpuThreadGroup(&groupInfo);

	if(ret != CELL_OK){
		printf("utilInitializeSpuThreadGroupAll failed : return code = %d\n", ret);
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}

	printf("SPU Start\n");

	sys_ppu_thread_t sender_thr, receiver_thr;
	ret = sys_ppu_thread_create(&sender_thr, sender_entry,
								(uintptr_t)ppu2spu_queue, PPU_THR_PRIO,
								PPU_THR_STACK_SIZE,
								SYS_PPU_THREAD_CREATE_JOINABLE,
								"PPU sender");
	if (ret != CELL_OK) {
		printf("sys_ppu_thread_create return non-zero value : %d\n", ret);
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}
	ret = sys_ppu_thread_create(&receiver_thr, receiver_entry,
								(uintptr_t)spu2ppu_queue, PPU_THR_PRIO,
								PPU_THR_STACK_SIZE,
								SYS_PPU_THREAD_CREATE_JOINABLE,
								"PPU receiver");
	if (ret != CELL_OK) {
		printf("sys_ppu_thread_create return non-zero value : %d\n", ret);
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}

	uint64_t exit_code;
	ret = sys_ppu_thread_join(sender_thr, &exit_code);
	if (ret != CELL_OK) {
		printf("ERROR: sys_ppu_thread_join returned %d\n", ret);
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}

	ret = sys_ppu_thread_join(receiver_thr, &exit_code);
	if (ret != CELL_OK) {
		printf("ERROR: sys_ppu_thread_join returned %d\n", ret);
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}
	
	ret = utilWaitSpuThreadGroup(&groupInfo, &groupStatus);
	if(ret != CELL_OK){
		printf("utilWaitSpuThreadGroup failed\n");
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}

	free(ppu2spu_queue);
	free(ppu2spu_work_space);

	free(spu2spu_queue);
	free(spu2spu_work_space);

	free(spu2ppu_queue);
	free(spu2ppu_work_space);

	ret = check_status(&groupStatus);
	if(ret != CELL_OK){
		printf("SPU Thread Group return non-zero value : %d\n", ret);
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}
	

	ret = utilFinalizeSpuThreadGroupAll(&groupInfo);
	if(ret != CELL_OK){
		printf("utilFinalizeSpuThreadGroupAll failed\n");
		printf("## libsync : sample_sync_lfqueue_ppu FAILED ##\n");
		exit(1);
	}
	
	printf("## libsync : sample_sync_lfqueue_ppu SUCCEEDED ##\n");

	exit(0);

}

/*
 * Local Variables:
 * mode:C
 * c-file-style: "stroustrup"
 * tab-width:4
 * End:
 * vim:ts=4:sw=4:
 */
