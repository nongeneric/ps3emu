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
#include <sys/spu_event.h>
#include <libsn_spu.h>

#define LOOP_COUNT 10

uint32_t count_array[4] __attribute__ ((aligned (128)));
char message_buffer[128] __attribute__ ((aligned (128)));
uint32_t tag = 3;


int main(uint64_t spu_num, uint64_t queue, uint64_t count_buffer, uint64_t message) 
{
	sys_spu_thread_send_event(44, 0x11223344, 0xaabbccdd);

	uint32_t d1, d2, d3;
	
	sys_spu_thread_receive_event(45, &d1, &d2, &d3);
	uint32_t count = d1 + d2 + d3;
	cellDmaSmallPut(&count, count_buffer, sizeof(uint32_t), tag, 0 , 0);
	cellDmaWaitTagStatusAll(1<<tag);

	return 0;
}
