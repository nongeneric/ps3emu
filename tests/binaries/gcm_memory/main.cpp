/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2009 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */
/*   File: main.cpp
 *   Description:
 *     simple graphics to show how to use libgcm
 *
 */

#define __CELL_ASSERT__
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timer.h>
#include <sys/return_code.h>
#include <cell/gcm.h>
#include <stddef.h>
#include <math.h>
#include <sysutil/sysutil_sysparam.h>

#include "snaviutil.h"
#include "gcmutil_error.h"

/* double buffering */
#define COLOR_BUFFER_NUM 2

// For exit routine
static void sysutil_exit_callback(uint64_t status, uint64_t param, void* userdata);
static bool sKeepRunning = true;

using namespace cell::Gcm;

typedef struct
{
	float x, y, z;
	uint32_t rgba; 
} Vertex_t;


/* local memory allocation */
static uint32_t local_mem_heap = 0;
static void *localMemoryAlloc(const uint32_t size) 
{
	uint32_t allocated_size = (size + 1023) & (~1023);
	uint32_t base = local_mem_heap;
	local_mem_heap += allocated_size;
	return (void*)base;
}

static void *localMemoryAlign(const uint32_t alignment, 
		const uint32_t size)
{
	local_mem_heap = (local_mem_heap + alignment-1) & (~(alignment-1));
	return (void*)localMemoryAlloc(size);
}

#define HOST_SIZE (1*1024*1024)

/* prototypes */
extern "C" int32_t userMain(void);

#define CB_SIZE	(0x10000)

void printTable(CellGcmOffsetTable table) {
	for (int i = 0; i < 0xbff; ++i) {
		if (table.ioAddress[i] != 0xffff) {
			printf("va to io: %x -> %x\n", i, table.ioAddress[i]);
		}
	}

	for (int i = 0; i < 511; ++i) {
		if (table.eaAddress[i] != 0xffff) {
			printf("io to va: %x -> %x\n", i, table.eaAddress[i]);
		}
	}
}

#define MB(x) (x * 1024 * 1024)

int userMain(void)
{
	void* host_addr = memalign(1024*1024, HOST_SIZE);
	CELL_GCMUTIL_ASSERTS(host_addr != NULL,"memalign()");
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmInit(CB_SIZE, HOST_SIZE, host_addr));
	
	CellGcmConfig gcmConfig;
	cellGcmGetConfiguration(&gcmConfig);
	printf("* vidmem base: 0x%p\n", gcmConfig.localAddress);
	printf("* IO base    : 0x%p\n", gcmConfig.ioAddress);
	printf("* vidmem size: 0x%x\n", gcmConfig.localSize);
	printf("* IO size    : 0x%x\n", gcmConfig.ioSize);

	uint32_t offset1 = 0xdd;
	cellGcmAddressToOffset((uint32_t*)gcmConfig.localAddress + 2, &offset1);
	printf("localAddress offset: %x\n", offset1);
	cellGcmAddressToOffset((uint32_t*)gcmConfig.ioAddress + 2, &offset1);
	printf("ioAddress offset: %x\n", offset1);

	uint32_t offset = 0xcc;
	uint32_t io = 0xcc;
	void* ea = 0;

	offset = cellGcmGetMaxIoMapSize();
	printf("offset: %x\n", offset);

	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(host_addr, &offset));
	printf("offset: %x\n", offset);

	CellGcmOffsetTable table;
	cellGcmGetOffsetTable(&table);
	printTable(table);

	CELL_GCMUTIL_CHECK_ASSERT(cellGcmIoOffsetToAddress(offset, &ea));
	printf("va: %x\n", ea);
	
	void* ea2 = memalign(MB(1), MB(3));
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmMapEaIoAddress(ea2, MB(5), MB(3)));
	printTable(table);

	cellGcmAddressToOffset(ea2, &offset);
	printf("ea2 offset: %x\n", offset);

	void* ea3 = memalign(MB(1), MB(2));
	int err = cellGcmMapEaIoAddressWithFlags(ea3, MB(15) + 500, MB(2), CELL_GCM_IOMAP_FLAG_STRICT_ORDERING);
	printf("err: %x\n", err);
	printTable(table);

	err = cellGcmMapEaIoAddressWithFlags(ea3, MB(15), MB(2), CELL_GCM_IOMAP_FLAG_STRICT_ORDERING);
	printf("err: %x\n", err);
	printTable(table);

	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(ea3, &offset));
	printf("ea3 offset: %x\n", offset);

	void* ea4 = memalign(MB(1), MB(4));
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmMapMainMemory(ea4, MB(4), &offset));
	printTable(table);
	printf("ea4 offset: %x\n", offset);

	CELL_GCMUTIL_CHECK_ASSERT(cellGcmUnmapEaIoAddress(ea3));
	printTable(table);

	CELL_GCMUTIL_CHECK_ASSERT(cellGcmUnmapIoAddress(offset));
	printTable(table);

	return 0;
}
