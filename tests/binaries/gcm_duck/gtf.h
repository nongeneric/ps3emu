/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#ifndef __GTF_H_
#define __GTF_H_
#include <cell/gcm.h>

typedef struct
{
	uint32_t  Version;
	uint32_t  Size;         // Total size of Texture (excluding header & attribute)
	uint32_t  NumTexture; // number of textures in this file
} CellGtfFileHeader;

/* Attribute for Texture data */
typedef struct 
{        
	uint32_t Id;
	uint32_t OffsetToTex;
	uint32_t TextureSize;

#ifdef __CELL_GCM_H__ // {
	CellGcmTexture tex;
#else  // } {
	struct tex {
		uint8_t		format;
		uint8_t		mipmap;
		uint8_t		dimension;
		uint8_t		cubemap;

		uint32_t	remap;

		uint16_t	width;
		uint16_t	height;
		uint16_t	depth;
		uint8_t		location;
		uint8_t		_padding;

		uint32_t	pitch;
		uint32_t	offset;
	};
#endif // __CELL_GCM_H__ // }
	uint32_t reserved[4*5];

} CellGtfTextureAttribute;

CellGcmTexture* getGtfTexture(uint32_t gtf_addr);
uint32_t getGtfTexAddr(uint32_t gtf_addr);
#endif // __GTF_H_
