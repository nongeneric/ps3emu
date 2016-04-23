/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#include <stdio.h>
#include <string.h>
#include "texture.h"
#include "gtf.h"
#include "fs.h"
#include "memory.h"
#include <cell/gcm.h>
#include <sys/paths.h>

#include "gcmutil_error.h"

using namespace cell::Gcm;



static void reorderTex(uint32_t gtf_addr0)
{
	CellGcmTexture* tex = getGtfTexture(gtf_addr0);
	void* addr = (void*)getGtfTexAddr(gtf_addr0);
	void* tmp = mainMemoryAlign(128*1024, tex->height*tex->pitch);
	for (int i = 0; i < tex->height; i++) {
		memcpy((void*)((uint32_t)tmp+i*tex->pitch), (void*)((uint32_t)addr+(tex->height-1-i)*tex->pitch), tex->pitch);
	}
	memcpy(addr, tmp, tex->height*tex->pitch);
}

static const char* sFileName0 = GCM_SAMPLE_DATA_PATH "/duck/duck256.gtf";
static CellGcmTexture* tex;
int setupTex()
{
	uint32_t gtf_addr0;

	if (loadFile(&gtf_addr0, (char*)sFileName0, CELL_GCM_LOCATION_MAIN) != CELL_OK)
		return -1;
	reorderTex(gtf_addr0);

	tex = getGtfTexture(gtf_addr0);
	void*  tex_addr = (void*)getGtfTexAddr(gtf_addr0);
	uint32_t tex_offset;
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(tex_addr, &tex_offset));
	tex->offset = tex_offset;
	tex->location = CELL_GCM_LOCATION_MAIN;
	tex->remap =  CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
				 CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
				 CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
				 CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
				 CELL_GCM_TEXTURE_REMAP_FROM_A << 6 |
				 CELL_GCM_TEXTURE_REMAP_FROM_R << 4 |
				 CELL_GCM_TEXTURE_REMAP_FROM_G << 2 |
				 CELL_GCM_TEXTURE_REMAP_FROM_B;
	return CELL_OK;
}

void setGcmTexture(uint32_t tex_sampler0)
{
	cellGcmSetTexture(tex_sampler0, tex);
	cellGcmSetTextureBorderColor(tex_sampler0, 0x00000000);
	cellGcmSetTextureControl(tex_sampler0, CELL_GCM_TRUE, 0, 0, CELL_GCM_TEXTURE_MAX_ANISO_1);
	cellGcmSetTextureAddress(tex_sampler0, CELL_GCM_TEXTURE_BORDER, CELL_GCM_TEXTURE_BORDER, CELL_GCM_TEXTURE_BORDER, CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL, CELL_GCM_TEXTURE_ZFUNC_LESS, 0);
	cellGcmSetTextureFilter(tex_sampler0, 0, CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);

}
