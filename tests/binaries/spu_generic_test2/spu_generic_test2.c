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


#define MAX_PHYSICAL_SPU      4 
#define MAX_RAW_SPU           0

#define SPU_THREAD_GROUP_PRIORITY 100

int count;
uint64_t u64s[8] __attribute__ ((aligned (128))) = {
	0, 1, 2, 3, 4, 5, 6, 7
};
vector unsigned int outputs[16] __attribute__ ((aligned (128))) = { 0 };
char message[128] __attribute__ ((aligned (128)));
extern char _binary_spu_spu_generic_test_spu2_elf_start[];

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
	
	for (int i = 0; i < sizeof(outputs); ++i) {
		((uint8_t*)outputs)[i] = i;
	}

	/* 2. Execute and wait sample_sync_mutex_spu */
	groupArg.group_name = "sample_sync_barrier";
	groupArg.priority   = SPU_THREAD_GROUP_PRIORITY;
	groupArg.nThreads   = 1;
	groupArg.thread[0].name  = "sample_sync_mutex_spu";
	groupArg.thread[0].option = SYS_SPU_THREAD_OPTION_NONE;
	groupArg.thread[0].elf   = (sys_addr_t)_binary_spu_spu_generic_test_spu2_elf_start;
	groupArg.thread[0].arg[0]= (uint64_t)i;
	groupArg.thread[0].arg[1]= (uint64_t)u64s;
	groupArg.thread[0].arg[2]= (uint64_t)outputs;
	groupArg.thread[0].arg[3]= (uint64_t)&message;

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

	printf("message: %s\n", message);
	uint32_t* o = (uint32_t*)outputs;
	float* f = (float*)outputs;
	for (int i = 0; i < sizeof(outputs) / 4; i += 4) {
		switch (i / 4) {
		case 154:
		case 162:
		case 166:
		case 168:
		case 187:
		case 188:
		case 203:
			printf("o[%i] = %g %g %g %g\n", i / 4, f[i], f[i + 1], f[i + 2], f[i + 3]);
			break;
		case 189:
		case 190:
		case 191:
		case 192:
			printf("o[%i] = %1.f %1.f %1.f %1.f\n", i / 4, f[i], f[i + 1], f[i + 2], f[i + 3]);
			break;
		default: 
			printf("o[%i] = %08x %08x %08x %08x\n", i / 4, o[i], o[i + 1], o[i + 2], o[i + 3]);
		}
	}
	printf("complete");

	return 0;
}
