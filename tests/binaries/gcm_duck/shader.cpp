/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#include "shader.h"
#include "memory.h"
#include <cell/gcm.h>
#include <string.h>

#include "gcmutil_error.h"

using namespace cell::Gcm;

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

static void *vertex_program_ucode;
static void *fragment_program_ucode;
static uint32_t fragment_offset;

int initShader(void)
{
	vertex_program   = (CGprogram)vertex_program_ptr;
	fragment_program = (CGprogram)fragment_program_ptr;

	// init
	cellGcmCgInitProgram(vertex_program);
	cellGcmCgInitProgram(fragment_program);

	uint32_t ucode_size;
	void *ucode;

	// main shader
	cellGcmCgGetUCode(fragment_program, &ucode, &ucode_size);
	// 128B alignment required 
	void *ret = localMemoryAlign(128, ucode_size);
	fragment_program_ucode = ret;
	memcpy(fragment_program_ucode, ucode, ucode_size); 

	// fragment program offset
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(fragment_program_ucode, &fragment_offset));

	cellGcmCgGetUCode(vertex_program, &ucode, &ucode_size);
	vertex_program_ucode = ucode;

	return CELL_OK;
}

void setShaderProgram(void)
{
	cellGcmSetFragmentProgram(fragment_program, fragment_offset);
	cellGcmSetVertexProgram(vertex_program, vertex_program_ucode);
}

uint32_t getFpUcode(void)
{
	return (uint32_t)fragment_program_ucode;
}
