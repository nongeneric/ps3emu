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

#include <cell/sheap/key_sheap_buffer.h>

/* sample libraries */
#include <spu_thread_group_utils.h>

#define MAX_PHYSICAL_SPU      4 
#define MAX_RAW_SPU           0

#define NUM_SPU_THREADS 1
#define SPU_THREAD_GROUP_PRIORITY 100

#define KEY 11
#define N_PARRAY 512
#define S_BUFFER (1024 * 10)
char sheap_buffer[S_BUFFER] __attribute__ ((aligned (128)));

#define TAG 3



extern char _binary_spu_sample_sheap_key_buffer_spu_elf_start[];

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
	CellKeySheapBuffer b;
	unsigned int *ans;

	ret = sys_spu_initialize(6, 0);

	if(ret != CELL_OK){
		printf("sys_spu_initialize failed : return code = %d\n", ret);
		printf("... but ignored.\n");
	}
	
	/* 1. Initilize KeySheap */
	ret = cellKeySheapInitialize(sheap_buffer, S_BUFFER, TAG);
	if(ret != CELL_OK){
		printf("cellKeySheapInitialize failed : ret = %d", ret);
		printf("## libsheap : sample_sheap_key_buffer_ppu FAILED ##\n");
	}
	
	/* 2. Create new buffer object. */
	ret = cellKeySheapBufferNew(&b, sheap_buffer, KEY, sizeof(unsigned int) * N_PARRAY);	
	if(ret != CELL_OK){
		printf("cellKeySheapBufferNew failed : ret = %d", ret);
		printf("## libsheap : sample_sheap_key_buffer_ppu FAILED ##\n");
		exit(1);
	}

	/* 3. Initialize the buffer */
	ans = (unsigned int*)cellKeySheapBufferGetEa(&b);
	for(i=0;i<N_PARRAY;i++){
		ans[i] = 0;
	}


	/* 4. Execute sample_sheap_key_buffer_spu */
	groupArg.group_name = "sample_sheap_key_buffer";
	groupArg.priority   = SPU_THREAD_GROUP_PRIORITY;
	groupArg.nThreads   = NUM_SPU_THREADS;
	groupArg.thread[0].name  = "sample_sheap_key_buffer_spu";
	groupArg.thread[0].option = SYS_SPU_THREAD_OPTION_NONE;
	groupArg.thread[0].elf   = (sys_addr_t)_binary_spu_sample_sheap_key_buffer_spu_elf_start;
	groupArg.thread[0].arg[0]= (uint64_t)sheap_buffer;
	groupArg.thread[0].arg[1]= (uint64_t)KEY;
	groupArg.thread[0].arg[2]= (uint64_t)N_PARRAY;
	

	ret = utilInitializeSpuThreadGroupAll(&groupInfo, &groupArg);
	if (ret != CELL_OK) {
		printf("utilInitializeSpuThreadGroupAll failed: %d\n", ret);
		printf("## libsheap : sample_sheap_key_buffer_ppu FAILED ##\n");
		exit(ret);
	}
	ret = utilStartSpuThreadGroup(&groupInfo);
	if (ret != CELL_OK) {
		printf("exec_spu_thread_group failed: %d\n", ret);
		printf("## libsheap : sample_sheap_key_buffer_ppu FAILED ##\n");
		exit(ret);
	}

	/* 5. Wait for termination */
	ret = utilWaitSpuThreadGroup(&groupInfo, &groupStatus);
	if (ret != CELL_OK) {
		printf("utilWaitSpuThreadGroup failed. %d\n", ret);
		printf("## libsheap : sample_sheap_key_buffer_ppu FAILED ##\n");
		exit(ret);
	}
	
	ret = check_status(&groupStatus);
	if (ret != CELL_OK) {
		printf("SPU Thread Group returns invalid status: %d\n", ret);
		printf("## libsheap : sample_sheap_key_buffer_ppu FAILED ##\n");
		exit(ret);
	}


	ret = utilFinalizeSpuThreadGroupAll(&groupInfo);
	if (ret != CELL_OK) {
		printf("utilFinalizeSpuThreadGroupAll failed. %d\n", ret);
		printf("## libsheap : sample_sheap_key_buffer_ppu FAILED ##\n");
		exit(ret);
	}

	
	/* 6. Print the contents of the buffer.
	 *    It should has an array of prime numbers 
	 */ 
	printf("PRIME NUMBERS(%d)\n",N_PARRAY);
	for(i = 0; i<N_PARRAY;i++){
		if((i%8) ==0){
			printf("\n");
		}
		printf("%d, ", ans[i]);
	}
	printf("\n");

	printf("## libsheap : sample_sheap_key_buffer_ppu SUCCEEDED ##\n");
	return 0;
}
