/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#include "memory.h"
#include <cell/gcm.h>
#include <stdlib.h>

#include "gcmutil_error.h"

#define HOST_SIZE (32*1024*1024)
#define CB_SIZE	(0x100000)
/* local memory allocation */
static uint32_t local_mem_heap = 0;
void *localMemoryAlloc(const uint32_t size) 
{
	uint32_t allocated_size = (size + 1023) & (~1023);
	uint32_t base = local_mem_heap;
	local_mem_heap += allocated_size;
	return (void*)base;
}

void *localMemoryAlign(const uint32_t alignment, const uint32_t size)
{
	local_mem_heap = (local_mem_heap + alignment-1) & (~(alignment-1));
	return (void*)localMemoryAlloc(size);
}

static uint32_t local_mem_heap2 = 0;
void *localMemoryAlloc2(const uint32_t size) 
{
	uint32_t allocated_size = (size + 1023) & (~1023);
	uint32_t base = local_mem_heap2;
	local_mem_heap2 += allocated_size;
	return (void*)base;
}

void *localMemoryAlign2(const uint32_t alignment, const uint32_t size)
{
	local_mem_heap2 = (local_mem_heap2 + alignment-1) & (~(alignment-1));
	return (void*)localMemoryAlloc2(size);
}

static uint32_t main_mem_heap = 0;
void *mainMemoryAlloc(const uint32_t size) 
{
	uint32_t allocated_size = (size + 1023) & (~1023);
	uint32_t base = main_mem_heap;
	main_mem_heap += allocated_size;
	return (void*)base;
}

void *mainMemoryAlign(const uint32_t alignment, 
		const uint32_t size)
{
	main_mem_heap = (main_mem_heap + alignment-1) & (~(alignment-1));
	return (void*)mainMemoryAlloc(size);
}

void setupGlobalMemoryPtr()
{
	// get config
	CellGcmConfig config;
	cellGcmGetConfiguration(&config);
	// buffer memory allocation
	local_mem_heap = (uint32_t)config.localAddress;

	cellGcmGetConfiguration(&config);
	// buffer memory allocation
	local_mem_heap2 = (uint32_t)config.localAddress;
}

int initMemory(void)
{
	uint32_t host_addr = (uint32_t)memalign(1024*1024, HOST_SIZE);
	CELL_GCMUTIL_ASSERTS(host_addr != NULL,"memalign()");
	printf("host %x\n", host_addr);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmInit(CB_SIZE, HOST_SIZE, (void*)host_addr));
	main_mem_heap = host_addr + CB_SIZE;
	setupGlobalMemoryPtr();

	return CELL_OK;
}
