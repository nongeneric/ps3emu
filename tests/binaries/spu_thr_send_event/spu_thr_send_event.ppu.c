/*
 *   SCE CONFIDENTIAL                                      
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2005 Sony Computer Entertainment Inc.   
 *   All Rights Reserved.                                  
 *
 * File: spu_thr_send_event.ppu.c
 * Description: 
 *  This sample shows how to implement event passing from SPU threads to a 
 *  PPU thread.  The typical sequence of the PPU side is as follows.
 *
 *  1. Create an event queue.  The queue type is SYS_PPU_THREAD in this sample.
 *     The SYS_SPU_THREAD type is also possible, if events are passed from an 
 *     SPU thread to another SPU thread.
 *  2. Connect the event sources to the event queue. 
 *     - SYS_SPU_THREAD_EVENT_USER: SPU Thread Event Ports
 *  3. Receive events.  This sample creates a PPU thread to handle this task.
 *  4. Disconnect and destroy the event queue. 
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
#define NUM_SPU_THREADS        4 /* The number of SPU threads in the group */ 
#define PRIORITY             100
#define SPU_PROG       (SYS_APP_HOME "/event_sender.spu.self")

#define TERMINATE_PORT_NAME   102047749UL
#define QUEUE_SIZE             32
#define SPU_THREAD_PORT        58

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
	 * Variables for the event queue and event handler PPU thread
	 */
	sys_event_queue_t queue;
	sys_event_queue_attribute_t queue_attr;
	sys_ppu_thread_t event_handle_thread; 

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
	
	/*
	 * Create a PPU thread to handle SPU thread events.
	 *
	 * This thread receives and handles the SPU thread events.  
	 * If an event is received from the terminating port (TERMINATE_PORT_KEY),
	 * this thread exits.
	 */
	sys_ppu_thread_t my_thread_id;
	int event_handle_prio;
	const char *event_handle_name = "SPU Thread Event Handler";

	sys_ppu_thread_get_id(&my_thread_id);
	ret = sys_ppu_thread_get_priority(my_thread_id, &event_handle_prio);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_ppu_thread_get_priority failed: %#.8x\n", ret);
		exit(ret);	
	} else 	{
		event_handle_prio += 1;
	}

	ret = sys_ppu_thread_create(&event_handle_thread, spu_thr_event_handler,
								(uint64_t)queue, event_handle_prio,
								4096, SYS_PPU_THREAD_CREATE_JOINABLE,
								event_handle_name);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_ppu_thread_create failed: %#.8x\n", ret);
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
		
		thread_args.arg1 = SYS_SPU_THREAD_ARGUMENT_LET_32(100);
		thread_args.arg2 = SYS_SPU_THREAD_ARGUMENT_LET_32(200);


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
		 * Connect the SPU thread user event type to the event queue.
		 * The SPU port from which the SPU thread sends events is represented
		 * by the fourth argument.
		 */
		printf("Conencting SPU thread (%#x) to the SPU thread user event.\n", i);
		ret = sys_spu_thread_connect_event(threads[i], queue, 
										   SYS_SPU_THREAD_EVENT_USER,
										   SPU_THREAD_PORT);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_connect_event failed: %#.8x\n", ret);
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
	 * Wait for the termination of the SPU thread group
	 */
	//printf("Waiting for the SPU thread group to be terminated.\n");
	int cause, status;
	ret = sys_spu_thread_group_join(group, &cause, &status);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_join failed: %#.8x\n", ret);
		exit(ret);
	}

	/*
	 * Show the exit cause and status.
	 */
	//switch(cause) {
	//case SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT:
	//	printf("The SPU thread group exited by sys_spu_thread_group_exit().\n");
	//	printf("The group's exit status = %d\n", status);
	//	break;
	//case SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT:
	//	printf("All SPU thread exited by sys_spu_thread_exit().\n");
	//	for (int i = 0; i < NUM_SPU_THREADS; i++) {
	//		int thr_exit_status;
	//		ret = sys_spu_thread_get_exit_status(threads[i], &thr_exit_status);
	//		if (ret != CELL_OK) {
	//			fprintf(stderr, "sys_spu_thread_get_exit_status failed: %#.8x\n", ret);
	//		}
	//		printf("SPU thread %d's exit status = %d\n", i, thr_exit_status);
	//	}
	//	break;
	//case SYS_SPU_THREAD_GROUP_JOIN_TERMINATED:
	//	printf("The SPU thread group is terminated by sys_spu_thread_terminate().\n");
	//	printf("The group's exit status = %d\n", status);
	//	break;
	//default:
	//	fprintf(stderr, "Unknown exit cause: %d\n", cause);
	//	break;
	//}

	/*
	 * Send a terminating event, and wait for event_handle_thread to join.
	 * If it successfully joined, disconnect and destroy the event queue
 	 */
	
	sys_event_port_t terminate_port;
	ret = sys_event_port_create(&terminate_port, SYS_EVENT_PORT_LOCAL,
								TERMINATE_PORT_NAME);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_port_create failed: %#.8x\n", ret);
	}

	ret = sys_event_port_connect_local(terminate_port, queue);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_port_connect_local failed: %#.8x\n", ret);
	}

	ret = sys_event_port_send(terminate_port, 0, 0, 0);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_port_send failed: %#.8x\n", ret);
	}
	
	ret = sys_event_port_disconnect(terminate_port);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_port_disconnect failed; %#.8x\n", ret);
	}

	ret = sys_event_port_destroy(terminate_port);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_event_port_destroy() failed: %#.8x\n", ret);
	}	

	uint64_t event_handle_status;
	ret = sys_ppu_thread_join(event_handle_thread, &event_handle_status);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_ppu_thread_join failed: %#.8x\n", ret);
	}
	
	/*
	 * Destroy the SPU thread group and clean up resources.
	 */
	for (int i = 0; i < NUM_SPU_THREADS; i++) {	
		ret = sys_spu_thread_disconnect_event(threads[i],
											  SYS_SPU_THREAD_EVENT_USER,
											  SPU_THREAD_PORT);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_disconnect_event() failed: %#.8x\n", ret);
		} else {
			printf("Disconnected SPU thread %u from the user event port %u.\n",
				   i, SPU_THREAD_PORT);
		}
	}

	ret = sys_spu_thread_group_destroy(group);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_destroy failed: %#.8x\n", ret);
	} else {
		printf("Destroyed the SPU thread group.\n");
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

/*
 * SPU Thread Event handler
 *
 * \param queue_id Event queue ID (Casted to uint64_t)
 */
void spu_thr_event_handler(uint64_t uint64_t_queue_id) 
{
	sys_event_queue_t queue_id = uint64_t_queue_id;
	int ret;

	while (1) {
		sys_event_t event;
		ret = sys_event_queue_receive(queue_id, &event, SYS_NO_TIMEOUT);
		if (ret != CELL_OK) {
			if (ret == (int)ECANCELED) {
				break;
			} else {
				fprintf(stderr, "sys_event_queue_receive failed: %#.8x\n", ret);
				sys_ppu_thread_exit(ret);
			}
		}

		if (event.source == TERMINATE_PORT_NAME) {
			printf("Received the terminating event.\n");
			break;
		}

		/*
		 * Multiplex the event source 
		 */
		switch (event.source) {
		case SYS_SPU_THREAD_EVENT_USER_KEY:
			printf("Received a user SPU thread event.\n");
			//printf("SPU Thread ID = %#llx\n", event.data1);
			printf("SPU thread port number = %llu\n", 
				   (event.data2 >> 32) & 0x000000FF);
			printf("Data = %llu\n", event.data2 & 0x0000000000FFFFFF);
			printf("Data = %llu\n", event.data3);
			printf("\n");
			break;
		default:
			printf("Received an event of unknown type.\n");
			printf("\n");
			break;
		}
	}
	printf("SPU Thread Event Handler is exiting.\n");
	sys_ppu_thread_exit(0);
}


