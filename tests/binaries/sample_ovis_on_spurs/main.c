/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
*/

#include <sys/spu_initialize.h>
#include <sys/ppu_thread.h>
#include <sys/process.h>
SYS_PROCESS_PARAM(1001, 0x10000)
#include <stdlib.h> /* exit */
#include <string.h>
#include <stdio.h>
#include <cell/ovis.h>
#include <cell/spurs/control.h>
#include <cell/spurs/task.h>


#define SPU_THREAD_GROUP_PRIORITY 250


#define N_ONEWAY 8
#define N_WAY    16
int  ramdom_orderd_array[N_WAY][N_ONEWAY] __attribute__((aligned(128)));

#define MESSAGE_SIZE 128
char message[N_WAY + 1][2][MESSAGE_SIZE] __attribute__((aligned(128)));


extern char _binary_task_task_ovis_on_spurs_elf_start[] ;
//elf_spuoverlay;

#define NUM_USE_SPU 1
#define SPU_THREAD_GROUP_PRIORITY     250

#define NUM_SPU_THREADS 1

static inline int get_secondary_ppu_thread_priority(int *prio)
{
	int ret;
	sys_ppu_thread_t my_ppu_thread_id;
	ret = sys_ppu_thread_get_id(&my_ppu_thread_id);
	if(ret){ return ret; }
	ret = sys_ppu_thread_get_priority(my_ppu_thread_id, prio);
	if(ret){ return ret; }
	
	*prio = *prio -1;
	return 0;
}


int
main(void)
{
  void *buf_top, *buf;
  char *elf;
  int bufsize;
  int i,j;
  int ret;

  printf("spu_initialize\n");

  ret = sys_spu_initialize(6,0);
  if(ret != CELL_OK){
	  printf("sys_spu_initialize failed : return code = %d\n", ret);
	  printf("... but ignored.\n");
  }

  
  int ppu_thr_prio;
  
  ret = get_secondary_ppu_thread_priority(&ppu_thr_prio);
  if (ret) {
	  printf("get PPU thread priority failed : %d\n", ret);
	  printf("## libovis : sample_ovis_on_spurs FAILED ##\n");
	  return ret;
  }
  

  elf = (char*)_binary_task_task_ovis_on_spurs_elf_start;
  printf("elf=%p\n",elf);
  

  /* 1. Construct overlay table from SPU ELF */

  /* 128-byte aligned area */
  bufsize = cellOvisGetOverlayTableSize(elf);
  buf_top = malloc(bufsize + 127);
  if (buf_top == NULL){
	  printf("## libovis : sample_ovis_on_spurs FAILED ##\n");
	  return -1;
  }
  memset(buf_top, 0, bufsize + 127 );
  buf  = (void*)(((long)buf_top + 127) & ~127);
  
  ret = cellOvisInitializeOverlayTable(buf, elf);
  if (ret) {
	  printf("cellOvisInitializeOverlayTable failed : %d\n", ret);
	  printf("## libovis : sample_ovis_on_spurs FAILED ##\n");
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
  CellSpurs	*spurs = memalign(CELL_SPURS_ALIGN, sizeof(CellSpurs));
  ret = cellSpursInitialize(spurs, NUM_USE_SPU, SPU_THREAD_GROUP_PRIORITY, 1, true);
  if (ret) {
	  printf("cellSpursInitialize failed: %d\n", ret);
	  printf("## libovis : sample_ovis_on_spurs FAILED ##\n");
	  return ret;
  }
  
  uint64_t *args = (uint64_t*)memalign(128, sizeof(uint64_t) * 4);
  args[0] = (uint64_t)(uintptr_t)buf;
  args[1] = (uint64_t)(uintptr_t)message;
  args[2] = (uint64_t)(uintptr_t)ramdom_orderd_array;
  args[3] = 0;


  /* 2. register taskset */
  CellSpursTaskset *taskset = memalign(CELL_SPURS_TASKSET_ALIGN, sizeof(CellSpursTaskset));
  uint8_t priority[] = {1,1,1,1, 1,1,1,1};
  ret = cellSpursCreateTaskset(spurs, taskset, (uint64_t)(uintptr_t)args,priority, 1);
  if (ret) {
	  printf("cellSpursCreateTaskset failed: %d\n", ret);
	  printf("## libovis : sample_ovis_on_spurs FAILED ##\n");
	  return ret;
  }
  
  
  CellSpursTaskId taskid;
  CellSpursTaskLsPattern lspattern;
  lspattern.u64[0] = 0;
  lspattern.u64[1] = 0;
  CellSpursTaskArgument task_arg;
  task_arg.u64[0] = 0;
  task_arg.u64[1] = 0;
  ret = cellSpursCreateTask(taskset,
							&taskid,
							_binary_task_task_ovis_on_spurs_elf_start,
							NULL,
							0,
							&lspattern,
							&task_arg);
  if (ret) {
	  printf("cellSpursCreateTask failed: %d\n", ret);
	  printf("## libovis : sample_ovis_on_spurs FAILED ##\n");
	  return ret;
  }

  
  
  /* 5. wait for completion */
  ret = cellSpursJoinTaskset(taskset);
  if (ret) {
	  printf("cellSpursJoinHandlerThread failed: %d\n", ret);
	  printf("## libovis : sample_ovis_on_spurs FAILED ##\n");
	  return ret;
  }

  /* 4. Prints a message from SPU. */
  printf("%s", message[N_WAY][0]);

  /* 5. Print result */
  printf("Ordered Arrays\n");
  for(i=0; i<N_WAY; i++){
	  printf("[%s x %s] ", message[i][0], message[i][1]);
	  for(j =0; j<N_ONEWAY; j++){
		  printf("%d,\t", ramdom_orderd_array[i][j]);
	  }
	  printf("\n");
  }

  free(buf_top);

  ret = cellSpursFinalize(spurs);
  if(ret){
	  printf("cellSpursFinalize failed: %d\n", ret);
	  printf("## libovis : sample_ovis_on_spurs FAILED ##\n");
	  return ret;
  }
  free(spurs);


  printf("## libovis : sample_ovis_on_spurs SUCCEEDED ##\n");

  
  return 0;
}
