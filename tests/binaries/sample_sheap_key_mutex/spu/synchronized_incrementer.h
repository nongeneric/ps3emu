#ifndef SYNCHRONIZED_INCREMENTER_H
#define SYNCHRONIZED_INCREMENTER_H

/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/


#include "key_usage.h"


#include <cell/sheap/key_sheap_mutex.h>
#include <stdlib.h>
#include <cell/dma.h>
class SynchronizedIncrementer 
{
private:
	uint32_t count_array[4] __attribute__ ((aligned (128)));
	uint64_t m_ea_sheap;
	static const int tag=3;
	CellKeySheapMutex mutex;

	void synchronizeBegin()
		{
			cellKeySheapMutexLock(&mutex);
		}
	void synchronizeEnd()
		{
			cellKeySheapMutexUnlock(&mutex);
		}
public:
	explicit SynchronizedIncrementer(uint64_t ea_sheap) : 
		m_ea_sheap(ea_sheap)
		{
			int ret = cellKeySheapMutexNew(&mutex, ea_sheap, KEY_SYNCHPOINT);
			if(ret != CELL_OK){
				abort();
			}
		}
	
	~SynchronizedIncrementer() 
		{ 
			cellKeySheapMutexDelete(&mutex);
		}
	void increment(uint64_t count_buffer);

} __attribute__ ((aligned (128)));


#endif /* SYNCHRONIZED_INCREMENTER_H */
