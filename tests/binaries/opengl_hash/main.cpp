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

#define HOST_SIZE (4*1024*1024)

/* prototypes */
extern "C" int32_t userMain(void);

#define CB_SIZE	(0x100000)

#define MB(x) (x * 1024 * 1024)

struct SourceImagePixel {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

typedef unsigned char GLubyte;
typedef unsigned int GLuint;
#define _JS_MAX_TEXTURE_UNITS 4
#define HASH_SIZE 509

struct jsGcmFPShaderState
{
    GLubyte loadType[_JS_MAX_TEXTURE_UNITS];
    GLubyte texEnv[_JS_MAX_TEXTURE_UNITS];
    GLubyte texFormat[_JS_MAX_TEXTURE_UNITS];
    GLubyte fog;
};

static GLuint jsGcmHashState( const jsGcmFPShaderState *state )
{
    GLuint hash = 0;
    int i;
    for ( i = 0;i < _JS_MAX_TEXTURE_UNITS;++i )
    {
        GLuint texState = (( state->texFormat[i] << 4 ) + ( state->texEnv[i] ) ) * state->loadType[i];
        hash = (( hash * 11 ) + texState ) % HASH_SIZE;
    }
    hash = (( hash * 11 ) + state->fog ) % HASH_SIZE;
    return hash;
}

int userMain(void)
{
	jsGcmFPShaderState state;
	char mem[] = { 
		0x01, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x04, 0x04, 0x04, 0x04, 
		0x00
	};
	memcpy(&state, mem, sizeof(state));
	GLuint hash = jsGcmHashState(&state);
	printf("hash: %x\n", hash);
}
