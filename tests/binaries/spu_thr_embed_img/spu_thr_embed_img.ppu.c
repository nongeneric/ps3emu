/*
 *   SCE CONFIDENTIAL                                      
 *   PlayStation(R)3 Programmer Tool Runtime Library 475.001
 *   Copyright (C) 2005 Sony Computer Entertainment Inc.   
 *   All Rights Reserved.                                  
 *
 * File: spu_thr_embed_img.c
 * Description:
 *  This samples show how to load an embedded SPU ELF image to SPU threads.
 *  The address of the embedded SPU ELF image is statically decided at linking,
 *  and it is embedded as a symbol which is derived from the orginal file 
 *  name.  The SPU ELF image is mapped to the process address space, when 
 *  the process is created from the PPU program which embeds the SPU ELF image
 *  in itself.  
 *
 *  The SPU ELF image which is located in a known address in the process 
 *  address space can be imported by sys_spu_image_import().
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/spu_initialize.h>
#include <sys/spu_image.h>
#include <sys/spu_thread.h>
#include <sys/spu_thread_group.h>
#include <sys/spu_utility.h>
#include <sys/process.h>
#include <sys/paths.h>
#include <spu_printf.h>

SYS_PROCESS_PARAM(1001, 0x10000)

#define MAX_PHYSICAL_SPU      4 
#define MAX_RAW_SPU           0
#define NUM_SPU_THREADS       1 /* The number of SPU threads in the group */ 
#define PRIORITY            100
#define SPU_PROG     "SPU_simple"
#define SPU_PROG2  (SYS_APP_HOME "/simple.spu.elf")

/*
 * These symbols represent the top and bottom addresses and the size of 
 * "SPU_simple" SPU ELF file which is embedded in this PPU program.
 * They are statically decided at the time of linking. 
 */
extern const char _binary_simple_spu_elf_start[];
extern const char _binary_simple_spu_elf_end[];
extern const char _binary_simple_spu_elf_size[];

int main(void)
{
	spu_printf_initialize(100, NULL);
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
	const void* elf_src;
	int ret;
		
	/*
	 * Output the addresses and size of the embed SPU ELF image, and store
	 * the top address of the SPU ELF image to elf_src.
	 */
	printf ("start=%p end=%p size=%p\n",
			_binary_simple_spu_elf_start,
			_binary_simple_spu_elf_end,
			_binary_simple_spu_elf_size);

	elf_src = (const void*)_binary_simple_spu_elf_start;

	/*
	 * Initialize SPUs
	 */
	printf("Initializing SPUs\n");
	ret = sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_initialize failed: %d\n", ret);
		exit(ret);
	}


	/*
	 * Create an SPU thread group
	 */
	printf("Creating an SPU thread group.\n");

	sys_spu_thread_group_attribute_initialize(group_attr);
	sys_spu_thread_group_attribute_name(group_attr,group_name);

	ret = sys_spu_thread_group_create(&group, 
									  NUM_SPU_THREADS,
									  PRIORITY, 
									  &group_attr);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_create failed: %d\n", ret);
		exit(ret);
	}

	/*
	 * The SPU program image is embedded in the PPU program, and its address is
	 * set to elf_src
	 */
	memset(&spu_img, 0x13, sizeof(spu_img));
	//sys_spu_image_open(&spu_img, SPU_PROG2);
	sys_spu_image_import(&spu_img, elf_src, SYS_SPU_IMAGE_PROTECT);

	/*
	 * Initialize SPU threads in the SPU thread group.
	 */
	sys_spu_thread_attribute_initialize(thread_attr);
	for (int i = 0; i < NUM_SPU_THREADS; i++) {
		sys_spu_thread_argument_t thread_args;

		printf("Initializing SPU thread %d\n", i);

		sys_spu_thread_attribute_name(thread_attr,thread_names[i]);
		sys_spu_thread_argument_initialize(thread_args);

		ret = sys_spu_thread_initialize(&threads[i],
										group,
										i,
										&spu_img,
										&thread_attr,
										&thread_args);
		if (ret != CELL_OK) {
			fprintf(stderr, "sys_spu_thread_initialize failed: %d\n", ret);
			exit(ret);
		}
		
	}

	printf("All SPU threads have been successfully initialized.\n");

	/*
	 * Start the SPU thread group
	 */
	printf("Starting the SPU thread group.\n");
	ret = sys_spu_thread_group_start(group);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_start failed: %d\n", ret);
		exit(ret);
	}

	/*
	 * Wait for the termination of the SPU thread group
	 */
	printf("Waiting for the SPU thread group to be terminated.\n");
	int cause, status;
	ret = sys_spu_thread_group_join(group, &cause, &status);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_join failed: %d\n", ret);
		exit(ret);
	}

	/*
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
				fprintf(stderr, "sys_spu_thread_get_exit_status failed:%d\n", ret);
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

	/*
	 * Destroy the SPU thread group and clean up resources.
	 */
	ret = sys_spu_thread_group_destroy(group);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_group_destroy failed: %d\n", ret);
	}

	/*
	 * The imported SPU program must be closed.
	 */
	ret = sys_spu_image_close(&spu_img);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_image_close failed: %.8x\n", ret);
	}


	printf("Exiting.\n");
	return 0;
}	




