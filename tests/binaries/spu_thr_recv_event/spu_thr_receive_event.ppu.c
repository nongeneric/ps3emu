/*
 *   SCE CONFIDENTIAL                                      
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2005 Sony Computer Entertainment Inc.   
 *   All Rights Reserved.                                  
 *
 * File: spu_thr_receive_event.ppu.c
 * Description: 
 *  This sample shows how to send events to SPU threads.
 *  Note the following important steps.
 *   - The event queue must be created as an event queue for SPU thread
 *     (SYS_SPU_QUEUE).
 *   - Each SPU thread which receives events must be bound with the event 
 *     queue by sys_spu_thread_bind_queue().  
 *   - The SPU queue number is given at the time of binding. The SPU thread
 *     which receives events also needs to know this number.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/spu_initialize.h>
#include <sys/spu_image.h>
#include <sys/spu_thread.h>
#include <sys/spu_thread_group.h>
#include <sys/spu_utility.h>
#include <sys/event.h>
#include <sys/paths.h>
#include <sys/process.h>

SYS_PROCESS_PARAM(1001, 0x10000)

#define MAX_PHYSICAL_SPU       4 
#define MAX_RAW_SPU            0
#define NUM_SPU_THREADS        4 /* The number of SPU threads in the group */ 
#define PRIORITY             100
#define SPU_PROG       (SYS_APP_HOME "/event_receiver.spu.self")

#define QUEUE_SIZE             127
#define SPU_QUEUE_NUMBER  16850944
#define EVENT_TERMINATE 0xfee1deadUL

void spu_thr_event_handler(uint64_t queue_id);

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
		 "SPU Thread 2",
		 "SPU Thread 3"}; /* The names of SPU threads */
    sys_spu_image_t spu_img;

	/*
	 * Variables for the event queue and event port
	 */
	sys_event_queue_t queue;
	sys_event_queue_attribute_t queue_attr;
	sys_event_port_t port;

	int ret;
	
	/*
	 * Initialize SPUs
	 */
	printf("Initializing SPUs\n");
	ret = sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_initialize failed: %#.8x\n", ret);
		exit(ret);
	}

	/*
	 * Create the event queue
	 * The event queue type must be created as an event queue for SPU thread
	 * because SPU threads receive events from this event queue.
	 */
	printf("Creating an event queue.\n");
	queue_attr.attr_protocol = SYS_SYNC_PRIORITY;
	queue_attr.type = SYS_SPU_QUEUE;
	ret = sys_event_queue_create(&queue, &queue_attr, SYS_EVENT_QUEUE_LOCAL, 
								 QUEUE_SIZE);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_queue_create failed: %#.8x\n", ret);
		exit(ret);
	}
	
	/*
	 * Create an event port and connect it to the event queue.
	 *
	 * PPU-SPU connection is process-local.  The type is SYS_EVENT_PORT_LOCAL.
	 * The connection is made by sys_event_port_connect_local().
	 */
	printf("Creating an event port.\n");
	ret = sys_event_port_create(&port, SYS_EVENT_PORT_LOCAL, SYS_EVENT_PORT_NO_NAME);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_port_create failed: %#.8x\n", ret);
		exit(ret);
	}
	
	printf("Connectiong the event port and queue.\n");
	ret = sys_event_port_connect_local(port, queue);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_port_connect_local failed: %#.8x\n", ret);
		exit(ret);
	}

	/*
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

	/*
	 * Initialize SPU threads in the SPU thread group.
	 * This sample loads the same image to all SPU threads.
	 */
	sys_spu_thread_attribute_initialize(thread_attr);
	for (int i = 0; i < NUM_SPU_THREADS; i++) {
		sys_spu_thread_argument_t thread_args;

		printf("Initializing SPU thread %d\n", i);
		
		sys_spu_thread_attribute_name(thread_attr,thread_names[i]);
		
		/*
		 * Pass the SPU queue number to the SPU thread as an argument.
		 * The SPU queue number is used by the SPU thread to identify the 
		 * event queue.
		 */
		thread_args.arg1 = SYS_SPU_THREAD_ARGUMENT_LET_32(SPU_QUEUE_NUMBER);
		
		ret = sys_spu_thread_initialize(&threads[i],
										group,
										i,
										&spu_img,
										&thread_attr,
										&thread_args);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_initialize failed: %#.8x\n", ret);
			exit(ret);
		}

		/*
		 * Bind the event queue to each SPU thread.
		 * This makes the SPU thread able to receive events from the bound
		 * event queue.  The bound event queue must be created as an event
		 * queue for SPU thread. 
		 *
		 * The SPU queue number, the third parameter, is used by the SPU thread
		 * to identify the event queue.  The SPU thread must know this value
		 * in order to receive events from the event queue.  In this sample,
		 * the SPU queue number is passed to the SPU thread as an argument.
		 * 
		 * (It is also possible to hardcode this number in both PPU and SPU
		 * programs.)
		 */
		printf("Binding SPU thread (%#x) to the event queue.\n", i);
		ret = sys_spu_thread_bind_queue(threads[i], queue, SPU_QUEUE_NUMBER);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_bind_queue failed: %#.8x\n", ret);
		}
	}

	printf("All SPU threads have been successfully initialized.\n");

	/*
	 * Start the SPU thread group
	 */
	printf("Starting the SPU thread group.\n");
	ret = sys_spu_thread_group_start(group);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_start failed: %#.8x\n", ret);
		exit(ret);
	}

	/*
	 * Send events to the SPU threads
	 * Send events with data (1, 1, 1), 100 times.
	 */
	int event_count = 100000;
	for (int i = 0; i < event_count; i++) {
		while (sys_event_port_send(port, 1,  1, 1) != CELL_OK) ;
		/*ret = sys_event_port_send(port, 1,  1, 1);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_event_port_send failed: %#.8x\n", ret);
		}*/
	}
	/* Send the terminating value to all SPU threads*/
	for (int i = 0; i < NUM_SPU_THREADS; i++) {
		while (sys_event_port_send(port, EVENT_TERMINATE, EVENT_TERMINATE,
								  EVENT_TERMINATE) != CELL_OK) ;
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_event_port_send failed: %#.8x\n", ret);
		}
	}
	
	/*
	 * Wait for the termination of the SPU thread group
	 */
	printf("Waiting for the SPU thread group to be terminated.\n");
	int cause, status;
	ret = sys_spu_thread_group_join(group, &cause, &status);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_join failed: %#.8x\n", ret);
		exit(ret);
	}

	/*
	 * Show the exit cause and status.
	 */
	int sum = 0;
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
			sum += thr_exit_status;
			if (ret != CELL_OK) {
				fprintf(stderr, "sys_spu_thread_get_exit_status failed: %#.8x\n", ret);
			}
			//printf("SPU thread %d's exit status = %d\n", i, thr_exit_status);
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

	printf("The expected sum of all exit status is %d.\n", event_count * 3);
	printf("The actual sum of all exit status is %d.\n", sum);

	/*
	 * Destroy the SPU thread group, event port and queue.
	 */
	for (int i = 0; i < NUM_SPU_THREADS; i++) {
		ret = sys_spu_thread_unbind_queue(threads[i], SPU_QUEUE_NUMBER);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_unbind_queue(%u, %u) failed: %#.8x\n",
					threads[i], SPU_QUEUE_NUMBER, ret);
		} else {
			printf("Unbound SPU thread %u from SPU queue number %u.\n", 
				   i, SPU_QUEUE_NUMBER);
		}
	}
	
	ret = sys_spu_thread_group_destroy(group);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_destroy() failed: %#.8x\n", ret);
	} else {
		printf("Destroyed the SPU thread group.\n");
	}

	ret = sys_event_port_disconnect(port);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_port_disconnect() failed: %#.8x\n", ret);
	} else {
		printf("Disconnected the event port.\n");
	}

	ret = sys_event_port_destroy(port);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_port_destroy() failed: %#.8x\n", ret);
	} else {
		printf("Destroyed the event port.\n");
	}

	ret = sys_event_queue_destroy(queue, 0);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_queue_destroy failed: %#.8x\n", ret);
	} else {
		printf("Destroyed the event queue.\n");
	}

	ret = sys_spu_image_close(&spu_img);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_image_close failed: %.8x\n", ret);
	}
	
	printf("Exiting.\n");

	return 0;
}	





