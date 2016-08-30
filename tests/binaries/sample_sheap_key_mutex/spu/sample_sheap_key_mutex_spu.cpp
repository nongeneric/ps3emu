/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdint.h>

#include "synchronized_incrementer.h"

void * operator new(size_t bytes)
{
	return memalign(128, bytes);
}




int main(uint64_t ea_sheap, uint64_t N,  uint64_t count_buffer) 
{
	SynchronizedIncrementer* sync_inc = new SynchronizedIncrementer(ea_sheap);
	
	
	for(uint32_t i=0;i<N;i++){
		sync_inc->increment(count_buffer);
	}
	
	delete sync_inc;

	return 0;
}
