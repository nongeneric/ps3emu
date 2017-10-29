/* 
 *  SCE CONFIDENTIAL                                      
 *   PlayStation(R)3 Programmer Tool Runtime Library 475.001
 *   Copyright (C) 2004 Sony Computer Entertainment Inc.    
 *   All Rights Reserved.                                   
 */

/*E
 * File: spu_thr_dma_sync.c
 * Description:
 *
 *  This PPU program creates 4 SPU threads that belong to the same SPU thread
 *  group. Each SPU thread receives a signal from the previous SPU thread,
 *  and sends a signal that is incremented by one to the next SPU thread. 
 *  The last SPU thread sends a singal to the first SPU thread.  The first 
 *  SPU thread, then, send the resultant signal value to PPU (this program) as
 *  an event.  The resultant value should equals the total number of SPU 
 *  threads.
 *
 *  See "SPU_mmio.c" for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/spu_initialize.h>
#include <sys/spu_image.h>
#include <sys/spu_thread.h>
#include <sys/spu_thread_group.h>
#include <sys/spu_utility.h>
#include <sys/ppu_thread.h>
#include <sys/event.h>
#include <sys/paths.h>
#include <sys/process.h>

SYS_PROCESS_PARAM(1001, 0x10000)

#define MAX_PHYSICAL_SPU       4 
#define MAX_RAW_SPU            0
#define NUM_SPU_THREADS        1 /* The number of SPU threads in the group */ 
#define PRIORITY             100
#define SPU_PROG       (SYS_APP_HOME "/mmio.spu.self")

#define EVENT_QUEUE_KEY  85268864UL
#define TERMINATE_PORT_KEY   102047749UL
#define QUEUE_SIZE             32
#define SPU_THREAD_PORT        58

void increment_mark(unsigned int marker);

int main(void)
{
	sys_spu_thread_group_t group; /* SPU thread group ID */
	const char *group_name = "Group";
	sys_spu_thread_group_attribute_t group_attr;/* SPU thread group attribute*/
	sys_spu_thread_t threads[NUM_SPU_THREADS];  /* SPU thread IDs */
	sys_spu_thread_attribute_t thread_attr;     /* SPU thread attribute */
	const char *thread_names[NUM_SPU_THREADS] = 
		{"SPU Thread 0",
		 "SPU Thread 1",
		 "SPU_Thread 2", 
		 "SPU Thread 3"}; /* The names of SPU threads */
	sys_spu_image_t spu_img;

	/*E
	 * Variables for the event queue 
	 */
	sys_event_queue_t queue;
	sys_event_queue_attribute_t queue_attr;
	int ret;
	
	/*E
	 * Initialize SPUs
	 */
	printf("Initializing SPUs\n");
	ret = sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_initialize failed: %#.8x\n", ret);
		exit(ret);
	}

	/*E
	 * Initialize the event queue
	 * This event queue will be connected to the SPU threads.
	 */
	printf("Creating an event queue.\n");
	queue_attr.attr_protocol = SYS_SYNC_PRIORITY;
	queue_attr.type = SYS_PPU_QUEUE;
	ret = sys_event_queue_create(&queue, &queue_attr, SYS_EVENT_QUEUE_LOCAL, 
								 QUEUE_SIZE);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_queue_create failed: %#.8x\n", ret);
		exit(ret);
	}
	
	/*E
	 * Create an SPU thread group
	 * 
	 * The SPU thread group is initially in the NOT INITIALIZED state.
	 */
	printf("Creating an SPU thread group.\n");

	sys_spu_thread_group_attribute_initialize(group_attr);
	sys_spu_thread_group_attribute_name(group_attr,group_name);

	ret = sys_spu_thread_group_create(&group, 
									  NUM_SPU_THREADS,
									  PRIORITY, 
									  &group_attr);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_create failed: %#.8x\n", ret);
		exit(ret);
	}
	
	ret = sys_spu_image_open(&spu_img, SPU_PROG);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_image_open failed: %#.8x\n", ret);
		exit(ret);
	}

	/*E
	 * Initialize SPU threads in the SPU thread group.
	 * This sample loads the same image to all SPU threads.
	 */
	sys_spu_thread_attribute_initialize(thread_attr);
	for (int i = 0; i < NUM_SPU_THREADS; i++) {
		sys_spu_thread_argument_t thread_args;
		int spu_num = i;

		printf("Initializing SPU thread %d\n", i);
		
		sys_spu_thread_attribute_name(thread_attr,thread_names[i]);

		/*E
         *      arg1: The total number of SPU threads.
         *      arg2: SPU thread number of the SPU threa
		 */
		thread_args.arg1 = SYS_SPU_THREAD_ARGUMENT_LET_32(NUM_SPU_THREADS);
		thread_args.arg2 = SYS_SPU_THREAD_ARGUMENT_LET_32(spu_num);

		ret = sys_spu_thread_initialize(&threads[i],
										group,
										spu_num,
										&spu_img,
										&thread_attr,
										&thread_args);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_initialize failed: %#.8x\n", ret);
			exit(ret);
		}

		/*E
		 * Connect the SPU thread user event type to the event queue.
		 * The SPU port from which the SPU thread sends events is represented
		 * by the fourth argument.
		 */
		printf("Conencting SPU thread (%d) to the SPU thread user event.\n", i);
		ret = sys_spu_thread_connect_event(threads[i], queue, 
										   SYS_SPU_THREAD_EVENT_USER,
										   SPU_THREAD_PORT);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_connect_event failed: %#.8x\n", ret);
		}
	}

	printf("All SPU threads have been successfully initialized.\n");

	for (int i = 0; i < 20; ++i) {

		/*E
		 * Start the SPU thread group
		 */
		printf("Starting the SPU thread group.\n");
		ret = sys_spu_thread_group_start(group);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_group_start failed: %#.8x\n", ret);
			exit(ret);
		}
	
		/*E
		 * Receive the resultant value with an event
		 * If the signal passing have done successfully, the resultant value is
		 * equal to the number of SPU threads,
		 * 
		 */
		sys_event_t event;
		unsigned int sig_result;
		printf("Waiting for an SPU thread event\n");
		ret = sys_event_queue_receive(queue, &event, SYS_NO_TIMEOUT);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_event_queue_receive failed: %#.8x\n", ret);
			exit(ret);
		}

		sig_result = (unsigned int)event.data2;
		printf("The resultant signal = %d.\n", sig_result);

		/*E
		 * Wait for the termination of the SPU thread group
		 */
		//printf("Waiting for the SPU thread group to be terminated.\n");
		int cause, status;
		ret = sys_spu_thread_group_join(group, &cause, &status);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_group_join failed: %#.8x\n", ret);
			exit(ret);
		}

		/*E
		 * Show the exit cause and status.
		 */
		switch(cause) {
		case SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT:
			printf("The SPU thread group exited by sys_spu_thread_group_exit().\n");
			printf("The group's exit status = %d\n", status);
			break;
		case SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT:
			printf("All SPU thread exited by sys_spu_thread_exit().\n");
			for (int i = 0; i < NUM_SPU_THREADS; i++) {
				int thr_exit_status;
				ret = sys_spu_thread_get_exit_status(threads[i], &thr_exit_status);
				if (ret != CELL_OK) {
					fprintf(stderr, "sys_spu_thread_get_exit_status failed:%#.8x\n", ret);
				}
				printf("SPU thread %d's exit status = %d\n", i, thr_exit_status);
			}
			break;
		case SYS_SPU_THREAD_GROUP_JOIN_TERMINATED:
			printf("The SPU thread group is terminated by sys_spu_thread_terminate().\n");
			printf("The group's exit status = %d\n", status);
			break;
		default:
			fprintf(stderr, "Unknown exit cause: %d\n", cause);
			break;
		}

	}

	ret = sys_event_queue_destroy(queue, 0);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_queue_destroy failed: %#.8x\n", ret);
	}

	/*E
	 * Destroy the SPU thread group and clean up resources.
	 */
	ret = sys_spu_thread_group_destroy(group);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_destroy failed: %#.8x\n", ret);
	}

	ret = sys_spu_image_close(&spu_img);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_image_close failed: %.8x\n", ret);
	}
	
	printf("Exiting.\n");

	return 0;
}	


