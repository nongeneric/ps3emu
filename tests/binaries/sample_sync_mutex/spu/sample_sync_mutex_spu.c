/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 360.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <sys/spu_thread.h>
#include <stdint.h>
#include <cell/sync.h>
#include <cell/dma.h>
#include <cellstatus.h>
#include <string.h>
#include <stdio.h>

#define LOOP_COUNT 3000

uint32_t count_array[4] __attribute__ ((aligned (128)));
char message_buffer[128] __attribute__ ((aligned (128)));



int main(uint64_t spu_num, uint64_t mutex, uint64_t count_buffer, uint64_t message) 
{
	uint32_t tag = 3;
	int ret;
	int i;

	//snprintf((char*)message_buffer, 128, "Last Writer is SPU %d", spu_num);
	
	uint32_t *count = &count_array[ (count_buffer%16)/4];


	/* 1. Execute following sequence LOOP_COUNT times */
	for(i = 0; i<LOOP_COUNT; i++){

		/* 2. Acquire mutex lock */ 
		ret = cellSyncMutexLock(mutex);
		if(ret != CELL_OK){
			return 1;
		}
		
		/* 3. Read current number, increment it and write it. 
		 *    And write the last writer to message_buffer at the same time.
		 */
		cellDmaSmallGet(count, count_buffer, sizeof(uint32_t), tag, 0 , 0);
		cellDmaWaitTagStatusAll(1<<tag);

		*count = *count + 1;

		cellDmaSmallPut(count, count_buffer, sizeof(uint32_t), tag, 0 , 0);
		cellDmaPut(message_buffer, message, 128, tag, 0 , 0);
		cellDmaWaitTagStatusAll(1<<tag);

		/* 4. Release mutex lock */ 
		ret = cellSyncMutexUnlock(mutex);

		if(ret != CELL_OK){
			return 1;
		}
	}

	return 0;
}
