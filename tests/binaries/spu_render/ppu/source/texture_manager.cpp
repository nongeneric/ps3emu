/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#include <cstring>
#include <sys/paths.h>
#include <cell/gcm.h>
#include "memory.h"
#include "file.h"
#include "gtf.h"
#include "texture_manager.h"

namespace Render{

CellGcmTexture TextureManager::m_texture_header[TEXTURE_ID_LAST];

static const char* TextureName[] = {
		SYS_APP_HOME"/data/duck256mod.gtf",
};

void TextureManager::init(){
	for(int id=0; id < TEXTURE_ID_LAST; id++){
		loadTexture(id,CELL_GCM_LOCATION_LOCAL);
	}
}

void TextureManager::loadTexture(int id, int location){
	void* buffer = Sys::File::loadFile(TextureName[id], Sys::Memory::getTemporaryHeap(),16);
	Gtf::GtfFileHeader* gtf = Gtf::getTexture(buffer);
	Gtf::GtfTextureAttribute* attr = Gtf::getAttribute(gtf,0);
	void* base_tex_addr = Gtf::getTextureAddress(gtf,attr);
	Sys::Memory::VramHeap& vramHeap = (location == CELL_GCM_LOCATION_LOCAL)
		? Sys::Memory::getLocalMemoryHeap()
		: Sys::Memory::getMappedMainMemoryHeap();
	void* copy_buffer = vramHeap.alloc(attr->texture_size,CELL_GCM_TEXTURE_SWIZZLE_ALIGN_OFFSET);
	std::memcpy(copy_buffer, base_tex_addr, attr->texture_size);
	CellGcmTexture* gcm_texture = &m_texture_header[id];
	*gcm_texture	 = attr->tex;
	gcm_texture->location = location;
	gcm_texture->offset = vramHeap.AddressToOffset(copy_buffer);
	Sys::Memory::getTemporaryHeap().reset();
}

}; // namespace Render
