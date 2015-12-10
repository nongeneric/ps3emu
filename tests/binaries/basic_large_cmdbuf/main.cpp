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

#define HOST_SIZE (10*1024*1024)

/* prototypes */
extern "C" int32_t userMain(void);

static void setRenderState(void);
static void setDrawEnv(void);


uint32_t display_width;
uint32_t display_height; 

float    display_aspect_ratio;
uint32_t color_pitch;
uint32_t depth_pitch;
uint32_t color_offset[COLOR_BUFFER_NUM];
uint32_t depth_offset;

extern uint32_t _binary_vpshader_vpo_start;
extern uint32_t _binary_vpshader_vpo_end;
extern uint32_t _binary_fpshader_fpo_start;
extern uint32_t _binary_fpshader_fpo_end;

static unsigned char *vertex_program_ptr = 
(unsigned char *)&_binary_vpshader_vpo_start;
static unsigned char *fragment_program_ptr = 
(unsigned char *)&_binary_fpshader_fpo_start;

static CGprogram vertex_program;
static CGprogram fragment_program;
static CGparameter model_view_projection;

static void *vertex_program_ucode;
static void *fragment_program_ucode;
static uint32_t fragment_offset;
static uint32_t vertex_offset[2];
static uint32_t color_index ;
static uint32_t position_index ;
static float MVP[16];

static uint32_t frame_index = 0;

#define CB_SIZE	(0x10000)

int userMain(void)
{
	void* host_addr = memalign(1024*1024, HOST_SIZE);
	CELL_GCMUTIL_ASSERTS(host_addr != NULL,"memalign()");
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmInit(CB_SIZE, HOST_SIZE, host_addr));

	uint32_t* buf = (uint32_t*)memalign(1024*1024, 1024*1024*2);
	uint32_t bufOffset;
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmMapMainMemory(buf, 1024*1024*2, &bufOffset));
	uint32_t vals[512] = { 0 };

	uint32_t begin = (uint32_t)gCellGcmCurrentContext->begin;
	uint32_t end = (uint32_t)gCellGcmCurrentContext->end;
	printf("end - begin = %x\n", end - begin);
	
	for (int i = 7; i < 200; ++i) {
		vals[13] = i;
		cellGcmInlineTransfer(bufOffset, vals, 512, CELL_GCM_LOCATION_MAIN);
		cellGcmFinish(i);
		if (buf[13] != i) {
			printf("failure %d instead %d\n", buf[13], i);
			return 1;
		}
	}

	printf("success\n");

	return 0;
}