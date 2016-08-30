/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#include <stdio.h>
#include <stdlib.h>
#include <spu_printf.h>
#include <sys/spu_initialize.h>
#include <sys/process.h>
SYS_PROCESS_PARAM(1001, 0x10000)

#include <cell/sheap/key_sheap_buffer.h>

/* sample libraries */
#include <spu_thread_group_utils.h>


#define MAX_PHYSICAL_SPU      6 
#define MAX_RAW_SPU           0

#define NUM_SPU_THREADS 4
#define SPU_THREAD_GROUP_PRIORITY 100

#define S_BUFFER (1024 * 10)
char sheap_buffer[S_BUFFER] __attribute__ ((aligned (128)));

#define TAG 3



extern char _binary_spu_sample_sheap_key_mutex_spu_elf_start[];

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
	UtilSpuThreadGroupInfo    groupInfo;
	UtilSpuThreadGroupArg     groupArg;
	UtilSpuThreadGroupStatus  groupStatus;

	int i;
	int ret;

	ret = spu_printf_initialize(0, NULL);
	if (ret != CELL_OK) {
		printf("## libsheap : spu_printf_initialize FAILED ##\n");
		exit(ret);
	}


	/* 1. Initilize KeySheap */
	ret = cellKeySheapInitialize(sheap_buffer, S_BUFFER, TAG);
	if(ret != CELL_OK){
		printf("cellKeySheapInitialize failed : ret = %d", ret);
		printf("## libsheap : sample_sheap_key_mutex_ppu FAILED ##\n");
		exit(1);
	}
	
	static int count=0;
	int n_count=1000;
	
	/* 4. Execute sample_sheap_key_mutex_spu */
	groupArg.group_name = "sample_sheap_key_mutex";
	groupArg.priority   = SPU_THREAD_GROUP_PRIORITY;
	groupArg.nThreads   = NUM_SPU_THREADS;
	groupArg.thread[0].name  = "sample_sheap_key_mutex_spu";
	groupArg.thread[0].option = SYS_SPU_THREAD_OPTION_NONE;
	groupArg.thread[0].elf   = (sys_addr_t)_binary_spu_sample_sheap_key_mutex_spu_elf_start;
	groupArg.thread[0].arg[0]= (uint64_t)(uintptr_t)sheap_buffer;
	groupArg.thread[0].arg[1]= (uint64_t)n_count;
	groupArg.thread[0].arg[2]= (uint64_t)(uintptr_t)&count;
	for(i=1; i<NUM_SPU_THREADS; i++){
		groupArg.thread[i] = groupArg.thread[0];
	}
	

	ret = utilInitializeSpuThreadGroupAll(&groupInfo, &groupArg);
	if (ret != CELL_OK) {
		printf("utilInitializeSpuThreadGroupAll failed: %d\n", ret);
		printf("## libsheap : sample_sheap_key_mutex_ppu FAILED ##\n");
		exit(ret);
	}

	ret = spu_printf_attach_group(groupInfo.group_id);
	if (ret != CELL_OK) {
		printf("## libsheap : spu_printf_attach_group FAILED ##\n");
		exit(ret);
	}

	ret = utilStartSpuThreadGroup(&groupInfo);
	if (ret != CELL_OK) {
		printf("exec_spu_thread_group failed: %d\n", ret);
		printf("## libsheap : sample_sheap_key_mutex_ppu FAILED ##\n");
		exit(ret);
	}

	/* 5. Wait for termination */
	ret = utilWaitSpuThreadGroup(&groupInfo, &groupStatus);
	if (ret != CELL_OK) {
		printf("utilWaitSpuThreadGroup failed. %d\n", ret);
		printf("## libsheap : sample_sheap_key_mutex_ppu FAILED ##\n");
		exit(ret);
	}
	


	ret = check_status(&groupStatus);
	if (ret != CELL_OK) {
		printf("SPU Thread Group returns invalid status: %d\n", ret);
		printf("## libsheap : sample_sheap_key_mutex_ppu FAILED ##\n");
		exit(ret);
	}

	ret = spu_printf_finalize();
	if (ret != CELL_OK) {
		printf("## libsheap : spu_printf_finalize FAILED ##\n");
		exit(ret);
	}

	ret = utilFinalizeSpuThreadGroupAll(&groupInfo);
	if (ret != CELL_OK) {
		printf("utilFinalizeSpuThreadGroupAll failed. %d\n", ret);
		printf("## libsheap : sample_sheap_key_mutex_ppu FAILED ##\n");
		exit(ret);
	}

	/* 6. Print the contents of the buffer.
	 *    It should has an array of prime numbers 
	 */ 
	printf("ans => %d\n",count);
	if(count ==  n_count * NUM_SPU_THREADS){
		printf("## libsheap : sample_sheap_key_mutex_ppu SUCCEEDED ##\n");
	}else{
		printf("## libsheap : sample_sheap_key_mutex_ppu FAILED ##\n");
	}


	return 0;
}
