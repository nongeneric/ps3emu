/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#ifndef PPU_GTF_H
#define PPU_GTF_H
#include <cell/gcm.h>
#include "debug.h"

namespace Render{
namespace Gtf{

typedef struct
{
	uint32_t  version;
	uint32_t  size;         // Total size of Texture (excluding header & attribute)
	uint32_t  num_texture; // number of textures in this file
} GtfFileHeader;

/* Attribute for Texture data */
typedef struct 
{        
	uint32_t id;
	uint32_t offset_to_tex;
	uint32_t texture_size;
	CellGcmTexture tex;
} GtfTextureAttribute;

inline GtfFileHeader* getTexture(void* addr){
	return reinterpret_cast<GtfFileHeader*>(addr);
}

inline GtfTextureAttribute* getAttribute(GtfFileHeader* gtf,uint32_t num){
	MY_ASSERT(num < gtf->num_texture);
	GtfTextureAttribute* begin = reinterpret_cast<GtfTextureAttribute*>(reinterpret_cast<uintptr_t>(gtf) + sizeof(GtfFileHeader));
	return begin + num;
}

inline void* getTextureAddress(GtfFileHeader* gtf,GtfTextureAttribute* attr){
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(gtf) + attr->offset_to_tex);
}
}; // namespace Gtf
}; // namespace Render
#endif // PPU_GTF_H
