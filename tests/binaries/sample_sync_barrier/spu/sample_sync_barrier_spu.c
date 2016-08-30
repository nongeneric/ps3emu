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
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 128
char ls_buffer[BUFFER_SIZE] __attribute__ ((aligned (128)));
char ls_buffer2[BUFFER_SIZE] __attribute__ ((aligned (128)));


int main(uint64_t spu_num, uint64_t barrier, uint64_t n_spu, uint64_t buffer) 
{
	int ret;
	int tag = 3;

	/* 1. Places own message at each position on main storage. */
	snprintf((char*)ls_buffer, BUFFER_SIZE, "SPU %d", (uint32_t)spu_num);
	cellDmaPut(ls_buffer, buffer + BUFFER_SIZE * spu_num, BUFFER_SIZE, tag, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag);

	/* 2. Wait for other SPU Threads */
	ret = cellSyncBarrierNotify(barrier);
	if(ret != CELL_OK){
		exit(1);
	}
	ret = cellSyncBarrierWait(barrier);
	if(ret != CELL_OK){
		exit(1);
	}
	int next_spu_num = (spu_num + 1) % n_spu;

	/* 3. Reads next SPU Thread's message, append a new own message to it and place it again. */
	cellDmaGet(ls_buffer2, buffer + BUFFER_SIZE * next_spu_num, BUFFER_SIZE, tag, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag);

	snprintf((char*)ls_buffer, BUFFER_SIZE, "SPU %d reads \"%s\"", (uint32_t)spu_num, ls_buffer2);
	
	cellDmaPut(ls_buffer, buffer + BUFFER_SIZE * (n_spu + spu_num), BUFFER_SIZE, tag, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag);

	return 0;
	
}
