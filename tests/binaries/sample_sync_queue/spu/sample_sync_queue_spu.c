/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <sys/spu_thread.h>
#include <stdint.h>
#include <cell/sync.h>
#include <cell/dma.h>
#include <cellstatus.h>
#include <string.h>

char qbuffer[128] __attribute__ ((aligned (128)));

int main(uint64_t spu_num, uint64_t queue1, uint64_t queue2) 
{
	(void)spu_num;
	uint32_t tag = 3;
	int ret;
	int buffer_size = 128;
	__builtin_memset((void*)qbuffer, 0, 128);


	while(1){
		/* 1. Receive a command from queue1 */
		ret = cellSyncQueuePop(queue1, qbuffer, tag);
		if(ret != CELL_OK){
			sys_spu_thread_group_exit(1);
		}
		cellDmaWaitTagStatusAll(1<<tag);

		/* 1. Decode it and push to queue2 */
		if(strcmp((const char*)qbuffer,"S") == 0){
			strncpy((char*)qbuffer, "[SPU] Sony\n", buffer_size);
			do {
				ret = cellSyncQueuePush(queue2, qbuffer, tag);
			}while(ret == CELL_SYNC_ERROR_BUSY);
			cellDmaWaitTagStatusAll(1<<tag);

		}else if(strcmp((const char*)qbuffer,"C") == 0){
			strncpy((char*)qbuffer, "[SPU] Computer\n", buffer_size);
			ret = cellSyncQueuePush(queue2, qbuffer, tag);
			cellDmaWaitTagStatusAll(1<<tag);

		}else if(strcmp((const char*)qbuffer,"E") == 0){
			strncpy((char*)qbuffer, "[SPU] Entertainment\n", buffer_size);
			ret = cellSyncQueuePush(queue2, qbuffer, tag);
			cellDmaWaitTagStatusAll(1<<tag);

		}else if(strcmp((const char*)qbuffer,"I") == 0){
			strncpy((char*)qbuffer, "[SPU] Inc\n", buffer_size);
			ret = cellSyncQueuePush(queue2, qbuffer, tag);
			cellDmaWaitTagStatusAll(1<<tag);

		}else if(strcmp((const char*)qbuffer,"#") == 0){
			/* 3. If the command is termination command,
			 *    this SPU Thread also push it to queue2 and exits.
			 */ 
			strncpy((char*)qbuffer, "#", buffer_size);
			ret = cellSyncQueuePush(queue2, qbuffer, tag);
			cellDmaWaitTagStatusAll(1<<tag);
			
			sys_spu_thread_group_exit(0);
		}else{
			sys_spu_thread_group_exit(1);
		}
	}
	sys_spu_thread_group_exit(1);

}
