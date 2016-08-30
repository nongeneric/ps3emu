#include "synchronized_incrementer.h"
/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/


void SynchronizedIncrementer::increment(uint64_t count_buffer)
{
	synchronizeBegin();
	// critical section 
	
	uint32_t *count = &count_array[ (count_buffer%16)/4];
	
	cellDmaSmallGet(count, count_buffer, sizeof(uint32_t), tag, 0 , 0);
	cellDmaWaitTagStatusAll(1<<tag);
	
	*count = *count + 1;
	
	cellDmaSmallPut(count, count_buffer, sizeof(uint32_t), tag, 0 , 0);
	cellDmaWaitTagStatusAll(1<<tag);
	
	// critical section
	synchronizeEnd();
}

