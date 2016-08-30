/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
*/

#include <sys/spu_initialize.h>
#include <sys/spu_thread.h>
#include <sys/spu_thread_group.h>
#include <sys/spu_utility.h>
#include <sys/process.h>
SYS_PROCESS_PARAM(1001, 0x10000)
#include <stdlib.h> /* exit */
#include <cell/ovis/util.h>
#include <string.h>
#include <stdio.h>
#include <cell/ovis.h>
#include <spu_thread_group_utils.h>


#define SPU_THREAD_GROUP_PRIORITY 250

#define MAX_PHYSICAL_SPU      6 
#define MAX_RAW_SPU           0


#define N_ONEWAY 8
#define N_WAY    16
int  ramdom_orderd_array[N_WAY][N_ONEWAY] __attribute__((aligned(128)));

#define MESSAGE_SIZE 128
char message[N_WAY + 1][2][MESSAGE_SIZE] __attribute__((aligned(128)));

int system_ovis_spu(char elf_img[],  
					uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, 
					const char* path);


extern char _binary_spu_sample_ovis_manual_spu_elf_start[] ;
//elf_spuoverlay;

#define NUM_USE_SPU 2



#define NUM_SPU_THREADS 1

int check_status(UtilSpuThreadGroupStatus *status);

int check_status(UtilSpuThreadGroupStatus *status)
{
	switch(status->cause) {
	case SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT:
		return status->status;
	case SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT:
		{
			int i;
			for (i = 0; i < status->nThreads; i++) {
				if (status->threadStatus[i] != CELL_OK) {
					return status->threadStatus[i];
				}
			}
			return CELL_OK;
		}
	case SYS_SPU_THREAD_GROUP_JOIN_TERMINATED:
		return status->status;
	}
	return -1;
}




int system_ovis_spu(char elf_img[],  
					uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, 
					const char* path)
{
	UtilSpuThreadGroupInfo groupInfo;
	UtilSpuThreadGroupStatus groupStatus;
	
	UtilSpuThreadInfo threadInfo;
	int ret;
	
	uint64_t args[] = {arg1, arg2, arg3, arg4};
	ret = utilInitializeSpuThread(&threadInfo, path, (sys_addr_t)elf_img, args, SYS_SPU_THREAD_OPTION_NONE);
	if (ret != CELL_OK) {
		fprintf(stderr, "sys_spu_thread_elf_loader failed: %d\n", ret);
		return ret;
	}


	cellOvisFixSpuSegments(&threadInfo.thread_image);


	ret = utilInitializeSpuThreadGroup(&groupInfo, 
									   __FILE__,
									   SPU_THREAD_GROUP_PRIORITY,
									   1,
									   &threadInfo);
	if (ret != CELL_OK) {
		fprintf(stderr, "utilInitializeSpuThreadGroupAll_from_attr failed: %d\n", ret);
		return ret;
	}
	ret = utilStartSpuThreadGroup(&groupInfo);
	if (ret != CELL_OK) {
		fprintf(stderr, "utilStartSpuThreadGroup failed: %d\n", ret);
		return ret;
	}
	ret = utilWaitSpuThreadGroup(&groupInfo, &groupStatus);
	if (ret != CELL_OK) {
		fprintf(stderr, "utilWaitSpuThreadGroup failed : %d\n", ret);
		return ret;
	}

	ret = utilFinalizeSpuThread(&threadInfo);
	if (ret != CELL_OK) {
		fprintf(stderr, "utilFinalizeSpuThread failed : %d\n", ret);
		return ret;
	}


	ret = utilFinalizeSpuThreadGroup(&groupInfo);
	if (ret != CELL_OK) {
		fprintf(stderr, "utilFinalizeSpuThreadGroup failed : %d\n", ret);
		return ret;
	}



	return 0;
	
	
}


int
main(void)
{
  void *buf_top, *buf;
  char *elf;
  int bufsize;
  unsigned int r;
  int i,j;
  int ret;
  printf("spu_initialize\n");

  ret = sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);
  if(ret != CELL_OK){
	  printf("sys_spu_initialize failed : return code = %d\n", ret);
	  printf("... but ignored.\n");
  }

  elf = (char*)_binary_spu_sample_ovis_manual_spu_elf_start;
  printf("elf=%p\n",elf);
  

  /* 1. Construct overlay table from SPU ELF */

  /* 128-byte aligned area */
  bufsize = cellOvisGetOverlayTableSize(elf);
  buf_top = malloc(bufsize + 127);
  if (buf_top == NULL){
	  printf("## libovis : sample_ovis_manual FAILED ##\n");
	  return -1;
  }
  memset(buf_top, 0, bufsize + 127 );
  buf  = (void*)(((long)buf_top + 127) & ~127);
  
  ret = cellOvisInitializeOverlayTable(buf, elf);
  if (ret != CELL_OK) {
	  fprintf(stderr, "cellOvisInitializeOverlayTable failed: %d\n", ret);
	  printf("## libovis : sample_ovis_manual FAILED ##\n");
	  return ret;
  }
  
  //printf("overlay table=%p\n",buf);


  /* 2. Specify the algorithms executed by SPU. 
   *    This value is decoded by SPU,
   *    and used to determing the sort and random number generation algothms.
   */ 
  for(i=0; i<N_WAY; i++){
	  ramdom_orderd_array[i][0] = i%2;
	  ramdom_orderd_array[i][1] = (i/2)%2;
  }

  /* 3. Execute sample_ovis_manual_spu and wait. */
  r = system_ovis_spu(elf,
					  (uint64_t)(uintptr_t)buf, 
					  (uint64_t)(uintptr_t)message,
					  (uint64_t)(uintptr_t)ramdom_orderd_array,0,
 					  "spu/sample_ovis_manual_spu");
  if (r != 0) {
	  fprintf(stderr, "system_ovis_spu failed: %d\n", r);
	  printf("## libovis : sample_ovis_manual FAILED ##\n");
	  return r;
  }

  /* 4. Prints a message from SPU. */
  printf("%s", message[N_WAY][0]);

  /* 5. Print result */
  printf("Ordered Arrays\n");
  for(i=0; i<N_WAY; i++){
	  printf("[%s x %s] ", message[i][0], message[i][1]);
	  for(j =0; j<N_ONEWAY; j++){
		  if(j<N_ONEWAY-1){
			  if(ramdom_orderd_array[i][j] > ramdom_orderd_array[i][j+1]){
				  printf("## libovis : sample_ovis_manual FAILED ##\n");
				  return 1;
			  }
		  }
		  printf("%d,\t", ramdom_orderd_array[i][j]);
	  }
	  printf("\n");
  }

  free(buf_top);

  printf("## libovis : sample_ovis_manual SUCCEEDED ##\n");
  
  return 0;
}
