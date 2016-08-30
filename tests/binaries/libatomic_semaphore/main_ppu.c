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

#define N_SPU_THREAD 3
#define SPU_THREAD_GROUP_PRIORITY     250
#define CHALLENGE    1000

/* shared variables */
uint32_t semaphore=0;
uint32_t result=0;

/* embedded SPU ELF symbols */
extern char _binary_semaphore_spu_elf_start[];




int main(void)
{
	int ret;
	UtilSpuThreadGroupInfo groupInfo;
	UtilSpuThreadGroupStatus groupStatus;

	UtilSpuThreadInfo thread[ N_SPU_THREAD];
	
	ret = sys_spu_initialize(6, 0);
	if (ret != CELL_OK) {
		printf("sys_spu_initialize failed: %d\n", ret);
		printf("...but continue!\n");
	}

	result = 0;
	semaphore = 1;						/* initial value */

	uint64_t arg[4] = { (uintptr_t)&semaphore, (uintptr_t)&result, CHALLENGE, 0};

	for (int i = 0; i < N_SPU_THREAD; i++) {
		char	name[128];
		sprintf(name, "semaphore challenger %d", i+1);
		ret = utilInitializeSpuThread(&thread[i], 	
								name,
								(sys_addr_t)_binary_semaphore_spu_elf_start,
								arg,
								SYS_SPU_THREAD_OPTION_NONE);
		if (ret != CELL_OK) {
			printf("utilInitializeSpuThread failed: %d\n", ret);
			printf("## libatomic : sample_semaphore FAILED ##\n");
			exit(ret);
		}
	}

	ret = utilInitializeSpuThreadGroup(&groupInfo, 
									   "A sample SPU thread group",
									   SPU_THREAD_GROUP_PRIORITY,
									   N_SPU_THREAD,
									   thread);
	if (ret != CELL_OK) {
		printf("initialize_spu_thread_group failed: %d\n", ret);
		printf("## libatomic : sample_semaphore FAILED ##\n");
		exit(ret);
	}
	ret = utilStartSpuThreadGroup(&groupInfo);
	if (ret != CELL_OK) {
		printf("start_spu_thread_group failed: %d\n", ret);
		printf("## libatomic : sample_semaphore FAILED ##\n");
		exit(ret);
	}
	ret = utilWaitSpuThreadGroup(&groupInfo, &groupStatus);
	if (ret != CELL_OK) {
		printf("wait_spu_thread_group failed: %d\n", ret);
		printf("## libatomic : sample_semaphore FAILED ##\n");
		exit(ret);
	}
	
	for (int i = 0; i < N_SPU_THREAD; i++) {
		ret = utilFinalizeSpuThread(&thread[i]);
		if (ret != CELL_OK) {
			printf("close_spu_thread_group_images failed: %d\n", ret);
			printf("## libatomic : close_spu_thread_group_images FAILED ##\n");
			exit(ret);
		}
	}

	ret = utilFinalizeSpuThreadGroup(&groupInfo);
	if (ret != CELL_OK) {
		printf("destroy_spu_thread_group failed: %d\n", ret);
		printf("## libatomic : sample_semaphore FAILED ##\n");
		exit(ret);
	}

	/*
	 * check the result
	 */
	printf("result=%08x\n", result);
	printf("## libatomic : sample_atomic_semaphore SUCCEEDED ##\n");
	return 0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
