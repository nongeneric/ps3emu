/*   SCE CONFIDENTIAL                                       
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001 
 *   Copyright (C) 2004 Sony Computer Entertainment Inc.    
 *   All Rights Reserved. 
 */      
/*E
 * File: spu_thr_dma_sync.ppu.c
 * Description:
 *  This sample shows how to practically make synchronization between PPU and 
 *  an SPU thread and between two SPU threads to notify completion of DMA.
 *
 *  The sample is played by a PPU thread and two SPU threads that belong to the 
 *  same SPU thread group.  They transfer data in cycle like, 
 *    
 *    PPU -> SPU Thread 1 -> SPU Thread 2 -> PPU -> SPU Thread 1 -> ..... 
 *       
 *  and data is incremented every time it passes to the next thread.
 *  Completion of the data passing is notified as follows.
 *  1.) PPU -> SPU Thread 1
 *      Signal by the sys_spu_thread_write_signal() system call 
 *  2.) SPU Thread 1 -> SPU Thread 2
 *      Signal via MMIO (See sync_singal.spu.c SPU program source.)
 *  3.) SPU Thread 2 -> PPU
 *      Event
 * 
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
#include "spu_thr_dma_sync.h"

SYS_PROCESS_PARAM(1001, 0x10000)

#define MAX_PHYSICAL_SPU       4
#define MAX_RAW_SPU            0
#define NUM_SPU_THREADS        2	/* The number of SPU threads in the group */
#define PRIORITY             100
#define SPU_PROG       (SYS_APP_HOME "/dma_sync.spu.self")

#define TERMINATE_PORT_NAME   102047749UL
#define QUEUE_SIZE             32
#define SPU_THREAD_PORT        58

unsigned int volatile
dma_buffer[TRANSFER_SIZE / sizeof(unsigned int)]__attribute__ ((aligned(128)));

void spu_thr_event_handler(uint64_t queue_id);
void increment_mark(unsigned int marker);

int main(void)
{
	sys_spu_thread_group_t group;	/* SPU thread group ID */
	const char *group_name = "Group";
	sys_spu_thread_group_attribute_t group_attr;	/* SPU thread group attribute */
	sys_spu_thread_t threads[NUM_SPU_THREADS];	/* SPU thread IDs */
	sys_spu_thread_attribute_t thread_attr;	/* SPU thread attribute */
	const char *thread_names[NUM_SPU_THREADS] = { "SPU Thread 0",
		"SPU Thread 1"
	};							/* The names of SPU threads */
	sys_spu_image_t spu_img;

	/*
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
	switch (ret) {
	case CELL_OK:
		break;
	case EBUSY:
		printf("SPUs have already been initialized.\n");
		break;
	default:
		fprintf(stderr, "sys_spu_initialize failed: %#.8x\n", ret);
		exit(ret);
	}

	/*E
	 * Initialize the buffer
	 */
	memset((void*)dma_buffer, 0x0, TRANSFER_SIZE);

	/*E
	 * Initialize the event queue
	 * This event queue will be connected to the SPU threads.
	 */
	printf("Creating an event queue.\n");
	queue_attr.attr_protocol = SYS_SYNC_PRIORITY;
	queue_attr.type = SYS_PPU_QUEUE;
	ret =
		sys_event_queue_create(&queue, &queue_attr, SYS_EVENT_QUEUE_LOCAL,
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
									  NUM_SPU_THREADS, PRIORITY, &group_attr);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_create failed: %#.8x\n", ret);
		exit(ret);
	}

	/*E
	 * Open the SPU program file
	 */
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
		static int spu_num[NUM_SPU_THREADS] = { 0, 1 };

		printf("Initializing SPU thread %d\n", i);

		sys_spu_thread_attribute_name(thread_attr,thread_names[i]);

		thread_args.arg1 = SYS_SPU_THREAD_ARGUMENT_LET_32(spu_num[i]);
		thread_args.arg2 =
			SYS_SPU_THREAD_ARGUMENT_LET_32(spu_num[(i + 1) % NUM_SPU_THREADS]);
		thread_args.arg3 = SYS_SPU_THREAD_ARGUMENT_LET_64((uint64_t)dma_buffer);


		ret = sys_spu_thread_initialize(&threads[i],
										group,
										spu_num[i], 
										&spu_img, &thread_attr, &thread_args);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_initialize failed: %#.8x\n", ret);
			exit(ret);
		}

		/*
		 * Connect the SPU thread user event type to the event queue.
		 * The SPU port from which the SPU thread sends events is represented
		 * by the fourth argument.
		 */
		printf("Conencting SPU thread (%#x) to the SPU thread user event.\n",
			   i);
		ret = sys_spu_thread_connect_event(threads[i], queue,
										   SYS_SPU_THREAD_EVENT_USER,
										   SPU_THREAD_PORT);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_connect_event failed: %#.8x\n", ret);
		}
   	}

	printf("All SPU threads have been successfully initialized.\n");

	/*E
	 *  Clear the DMA buffer
	 */
	for (unsigned int i = 0; i < TRANSFER_SIZE / sizeof(unsigned int); i++) {
		dma_buffer[i] = 0x0;
	}

	/*E
	 * Start the SPU thread group
	 */
	printf("Starting the SPU thread group.\n");
	ret = sys_spu_thread_group_start(group);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_start failed: %#.8x\n", ret);
		exit(ret);
	}

	for (int i = 0; i < ITERATION; i++) {
		/*E
		 * Increment values in the buffer
		 */
		increment_mark(PPU_MARKER);

		/*E
		 * Send a singal to SPU thread 1 that I (PU) have
		 * finished incrementing, and go ahead to read it.
		 */
		ret = sys_spu_thread_write_snr(threads[0], 0, PPU_MARKER);
		if (ret) {
			printf("sys_spu_thread_write_signal is failed %#.8x\n", ret);
			return -1;
		}

		/*E
		 * Wait for an event which notifies that the SPU thread 2 has completed
		 * writing in dma_buffer and it is now ready for PPU.
		 */
		sys_event_t event;
		ret = sys_event_queue_receive(queue, &event, SYS_NO_TIMEOUT);
		if (ret) {
			printf("sys_event_queue_receive is failed %#.8x\n", ret);
			return -1;
		}
		printf("PPU: Received an event. (i = %d)\n", i);	
	}

	/*E
	 * Output the result.
	 */
	printf("Dump the DMA buffer\n");
	for (unsigned int i = 0; i < TRANSFER_SIZE / sizeof(unsigned int) / 4; i++) {
		printf("\t0x%x, 0x%x, 0x%x, 0x%x\n",
			   dma_buffer[i * 4], dma_buffer[i * 4 + 1],
			   dma_buffer[i * 4 + 2], dma_buffer[i * 4 + 3]);
	}


	/*E
	 * Wait for the termination of the SPU thread group
	 */
	printf("Waiting for the SPU thread group to be terminated.\n");
	int cause, status;
	ret = sys_spu_thread_group_join(group, &cause, &status);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_join failed: %#.8x\n", ret);
		exit(ret);
	}

	/*E
	 * Show the exit cause and status.
	 */
	switch (cause) {
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
				fprintf(stderr, "sys_spu_thread_get_exit_status failed:%d\n",
						ret);
			}
			printf("SPU thread %d's exit status = %d\n", i, thr_exit_status);
		}
		break;
	case SYS_SPU_THREAD_GROUP_JOIN_TERMINATED:
		printf
			("The SPU thread group is terminated by sys_spu_thread_terminate().\n");
		printf("The group's exit status = %d\n", status);
		break;
	default:
		fprintf(stderr, "Unknown exit cause: %d\n", cause);
		break;
	}

	/*E
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
		fprintf(stderr, "sys_event_connect failed: %#.8x\n", ret);
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

	/*E
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
 * Increment values in dma_buffer by "marker".
 */
void increment_mark(unsigned int marker)
{
	int buf_size = TRANSFER_SIZE / sizeof(unsigned int);
	for (int i = 0; i < buf_size; i++) {
		dma_buffer[i] += marker;
	}
}

