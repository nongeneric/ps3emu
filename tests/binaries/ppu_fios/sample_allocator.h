/*
	SCE CONFIDENTIAL
	PlayStation(R)3 Programmer Tool Runtime Library 400.001
	Copyright (C) 2008 Sony Computer Entertainment Inc.
	All Rights Reserved.
*/
/*
	sample_allocator.h
	Simple implementation of cell::fios::allocator based on memalign/free.
*/

#ifndef __FIOS_SAMPLE_ALLOCATOR_H__
#define __FIOS_SAMPLE_ALLOCATOR_H__

#include <cell/fios/fios_memory.h>

#include <stdlib.h>
#include <string.h>

class SampleAllocator : public cell::fios::allocator
{
public:
	SampleAllocator() {}
	virtual ~SampleAllocator() {}
	
	void* Allocate(uint32_t size, uint32_t flags, const char* pFile = 0, int line = 0) {
		(void) pFile;
		(void) line;
		return memalign(FIOS_ALIGNMENT_FROM_MEMFLAGS(flags), size);
	}

	void Deallocate(void* pMemory, uint32_t flags, const char* pFile = 0, int line = 0) {
		(void) flags;
		(void) pFile;
		(void) line;
		free(pMemory);
	}

	void* Reallocate(void* pMemory, uint32_t newSize, uint32_t flags, const char* pFile = 0, int line = 0) {
		(void) pMemory;
		(void) newSize;
		(void) flags;
		(void) pFile;
		(void) line;
		return NULL; /* fios does not use Reallocate */
	}
};

#endif //  __FIOS_SAMPLE_ALLOCATOR_H__

