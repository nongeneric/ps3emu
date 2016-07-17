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
#include <sys/event.h>


#include <spu_thread_group_utils.h>

#include <cell/sync.h>


#define MAX_PHYSICAL_SPU      5 
#define MAX_RAW_SPU           0

#define NUM_SPU_THREADS 2
#define SPU_THREAD_GROUP_PRIORITY 100

uint32_t count;
char message[128] __attribute__ ((aligned (128)));
extern char _binary_spu_spu_queue_spu_elf_start[];

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

#define CHECK(a) { int ret; if ((ret = (a)) != CELL_OK) { printf("failure at line %d: %08x\n", __LINE__, ret); exit(0); } }

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

	sys_event_queue_t queue;
	sys_event_queue_attribute_t queue_attr = { SYS_SYNC_PRIORITY, SYS_PPU_QUEUE };
	CHECK(sys_event_queue_create(&queue, &queue_attr, SYS_EVENT_QUEUE_LOCAL, 64));

	sys_event_queue_t queueToSpu;
	sys_event_queue_attribute_t queueToSpu_attr = { SYS_SYNC_PRIORITY, SYS_SPU_QUEUE };
	CHECK(sys_event_queue_create(&queueToSpu, &queueToSpu_attr, SYS_EVENT_QUEUE_LOCAL, 64));
	sys_event_port_t port;
	CHECK(sys_event_port_create(&port, SYS_EVENT_PORT_LOCAL, SYS_EVENT_PORT_NO_NAME));
	CHECK(sys_event_port_connect_local(port, queueToSpu));
	
	/* 2. Execute and wait sample_sync_mutex_spu */
	groupArg.group_name = "sample_sync_barrier";
	groupArg.priority   = SPU_THREAD_GROUP_PRIORITY;
	groupArg.nThreads   = 1;
	for(i=0;i<NUM_SPU_THREADS; i++){
		groupArg.thread[i].name  = "sample_sync_mutex_spu";
		groupArg.thread[i].option = SYS_SPU_THREAD_OPTION_NONE;
		groupArg.thread[i].elf   = (sys_addr_t)_binary_spu_spu_queue_spu_elf_start;
		groupArg.thread[i].arg[0]= (uint64_t)i;
		groupArg.thread[i].arg[1]= (uint64_t)&queue;
		groupArg.thread[i].arg[2]= (uint64_t)&count;
		groupArg.thread[i].arg[3]= (uint64_t)&message;
	}

	CHECK(utilInitializeSpuThreadGroupAll(&groupInfo, &groupArg));

	CHECK(sys_spu_thread_connect_event(groupInfo.thread_id[0], queue, SYS_SPU_THREAD_EVENT_USER, 44));
	CHECK(sys_spu_thread_bind_queue(groupInfo.thread_id[0], queueToSpu, 45));

	CHECK(utilStartSpuThreadGroup(&groupInfo));

	sys_event_t event;
	CHECK(sys_event_queue_receive(queue, &event, SYS_NO_TIMEOUT));
	printf("source: %08x, data: data1 [%08x,%08x] [%08x,%08x]\n", 
		(uint32_t)event.source, 
		(uint32_t)event.data2, (uint32_t)(event.data2 >> 32), 
		(uint32_t)event.data3, (uint32_t)(event.data3 >> 32));

	printf("event.data01 == thread id: %d\n", event.data1 == groupInfo.thread_id[0]);

	CHECK(sys_event_port_send(port, 10, 17, 33));

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
