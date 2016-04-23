/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#include "gtf.h"

CellGcmTexture* getGtfTexture(uint32_t gtf_addr)
{
	CellGtfTextureAttribute* gtf_tex = (CellGtfTextureAttribute*)(gtf_addr + sizeof(CellGtfFileHeader));
	return &(gtf_tex->tex);
}

uint32_t getGtfTexAddr(uint32_t gtf_addr)
{
	return (gtf_addr + sizeof(CellGtfFileHeader) + sizeof(CellGtfTextureAttribute));
}


