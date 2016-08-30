/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/spu_initialize.h>
#include <sys/spu_thread.h>
#include <sys/spu_thread_group.h>
#include <sys/spu_utility.h>
#include <sys/process.h>
SYS_PROCESS_PARAM(1001, 0x10000)


#include <cell/sync.h>
#include <spu_thread_group_utils.h>

#define MAX_PHYSICAL_SPU      4 
#define MAX_RAW_SPU           0

#define NUM_SPU_THREADS 1
#define SPU_THREAD_GROUP_PRIORITY 100

extern char _binary_spu_sample_sync_queue_spu_elf_start[];

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


	int queue_depth = 1000;
	int ret;
	int i;

	ret = sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);

	if(ret != CELL_OK){
		printf("sys_spu_initialize failed : return code = %d\n", ret);
		printf("...but contine!\n");
	}

	/* 1. Initialize queue1 and queue2. */
	void* buffer_queue1 = malloc(32 + 31);
	void* buffer_work_space1 = malloc(128 * queue_depth + 127);
	CellSyncQueue *queue1 = (CellSyncQueue *)(((uintptr_t)buffer_queue1 + 31) & ~31);
	void *work_space1 = (void*)(((uintptr_t)buffer_work_space1 + 127) & ~127);

	void* buffer_queue2 = malloc(32 + 31);
	void* buffer_work_space2 = malloc(128 * queue_depth + 127);
	CellSyncQueue *queue2 = (CellSyncQueue *)(((uintptr_t)buffer_queue2 + 31) & ~31);
	void *work_space2 = (void*)(((uintptr_t)buffer_work_space2 + 127) & ~127);

	ret = cellSyncQueueInitialize(queue1, work_space1, 128, queue_depth);
	if(ret != CELL_OK){
		printf("cellSyncQueueInitialize : return code = %d\n", ret);
		printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
		exit(1);
	}
	ret = cellSyncQueueInitialize(queue2, work_space2, 128, queue_depth);
	if(ret != CELL_OK){
		printf("cellSyncQueueInitialize : return code = %d\n", ret);
		printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
		exit(1);
	}
	/*printf("queue1=%p\n",queue1);
	printf("queue2=%p\n",queue2);*/

	/* 2. Execute sample_sync_queue_spu and passes queue1 and queu2 to it */
	groupArg.group_name = "sample_sync_queue";
	groupArg.priority   = SPU_THREAD_GROUP_PRIORITY;
	groupArg.nThreads   = NUM_SPU_THREADS;
	for(i=0;i<NUM_SPU_THREADS; i++){
		groupArg.thread[i].name  = "sample_sync_queue_spu";
		groupArg.thread[i].option = SYS_SPU_THREAD_OPTION_NONE;
		groupArg.thread[i].elf   = (sys_addr_t)_binary_spu_sample_sync_queue_spu_elf_start;
		groupArg.thread[i].arg[0]= (uint64_t)i;
		groupArg.thread[i].arg[1]= (uint64_t)(uintptr_t)queue1;
		groupArg.thread[i].arg[2]= (uint64_t)(uintptr_t)queue2;
		groupArg.thread[i].arg[3]= (uint64_t)0;
	}

	ret = utilInitializeSpuThreadGroupAll(&groupInfo, &groupArg);

	if(ret != CELL_OK){
		printf("utilInitializeSpuThreadGroupAll failed : return code = %d\n", ret);
		printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
		exit(1);
	}
	ret = utilStartSpuThreadGroup(&groupInfo);

	if(ret != CELL_OK){
		printf("utilInitializeSpuThreadGroupAll failed : return code = %d\n", ret);
		printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
		exit(1);
	}



	printf("SPU Start\n");
	
	/* 3. Push commands to queue1.
	 *    On the other hand, if there are any messages in queue2, print these. 
	 */
	char buffer[128];
	for(int a=0;a<1; a++){
		//printf("size(queue1)=%d\n",cellSyncQueueSize(queue2));
		sprintf(buffer,"S");
		ret = cellSyncQueueTryPush(queue1, buffer);
		printf("Send S\n");
		if(ret != CELL_OK){
			printf("cellSyncQueuePush : return code = %d\n", ret);
			printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
			exit(1);
		}
		sprintf(buffer,"C");
		ret = cellSyncQueueTryPush(queue1, buffer);
		printf("Send C\n");
		if(ret != CELL_OK){
			printf("cellSyncQueuePush : return code = %d\n", ret);
			printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
			exit(1);
		}
		sprintf(buffer,"E");
		ret = cellSyncQueueTryPush(queue1, buffer);
		printf("Send E\n");
		if(ret != CELL_OK){
			printf("cellSyncQueuePush : return code = %d\n", ret);
			printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
			exit(1);
		}
		sprintf(buffer,"I");
		ret = cellSyncQueueTryPush(queue1, buffer);
		printf("Send I\n");

		if(ret != CELL_OK){
			printf("cellSyncQueuePush : return code = %d\n", ret);
			printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
			exit(1);
		}
		//do {
		//	//printf("size(queue2)=%d\n",cellSyncQueueSize(queue2));
		//	ret = cellSyncQueueTryPop(queue2, buffer);
		//	if(ret == CELL_OK){
		//		printf("PPU Pop\n");
		//		printf("%s", buffer);
		//	}
		//} while( ret == CELL_OK);
		//if(ret != CELL_SYNC_ERROR_BUSY){
		//	printf("cellSyncQueuePush : return code = %d\n", ret);
		//	printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
		//	exit(1);
		//}

	}

	/* 4. Push termination command to queue1. */
	sprintf(buffer,"#");
	ret = cellSyncQueuePush(queue1, buffer);
	if(ret != CELL_OK){
		printf("cellSyncQueuePush : return code = %d\n", ret);
		printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
		exit(1);
	}
	
	__builtin_memset(buffer, 0, 128);
	
	/* 5. Wait for termination message from SPU Thread */
	while(1){
		ret = cellSyncQueuePop(queue2, buffer);
		if(ret == CELL_OK){
			if(buffer[0] == '#'){
				break;
			}else{
				printf("receive %s\n",buffer);
			}
		}else{
			printf("cellSyncQueuePush : return code = %d\n", ret);
			printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
			exit(1);
		}
	}

	ret = utilWaitSpuThreadGroup(&groupInfo, &groupStatus);
	if(ret != CELL_OK){
		printf("utilWaitSpuThreadGroup failed\n");
		printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
		exit(1);
	}

	
	free(buffer_queue1);
	free(buffer_work_space1);

	free(buffer_queue2);
	free(buffer_work_space2);


	ret = check_status(&groupStatus);
	if(ret != CELL_OK){
		printf("SPU Thread Group return non-zero value : %d\n", ret);
		printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
		exit(1);
	}
	

	ret = utilFinalizeSpuThreadGroupAll(&groupInfo);
	if(ret != CELL_OK){
		printf("utilFinalizeSpuThreadGroupAll failed\n");
		printf("## libsync : sample_sync_queue_ppu FAILED ##\n");
		exit(1);
	}

	printf("## libsync : sample_sync_queue_ppu SUCCEEDED ##\n");

	exit(0);

}
