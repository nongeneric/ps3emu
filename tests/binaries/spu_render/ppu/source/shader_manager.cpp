/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#include <string.h>
#include <cell/gcm.h>
#include <sys/paths.h>

#include "file.h"
#include "memory.h"
#include "shader_manager.h"

namespace Render{
CGprogram	ShaderManager::m_vertex_shader_binary[VERTEX_SHADER_ID_LAST];
CGprogram	ShaderManager::m_fragment_shader_binary[FRAGMENT_SAHDER_ID_LAST];
uint32_t	ShaderManager::m_fragment_ucode_offset[FRAGMENT_SAHDER_ID_LAST];
void*		ShaderManager::m_fragment_ucode_address[FRAGMENT_SAHDER_ID_LAST];
uint8_t	ShaderManager::m_fragment_ucode_location[FRAGMENT_SAHDER_ID_LAST];

static const char* FragmentShaderName[] ={
#define FRAMGNET_SHADER_NAME
#include "shader_list.inl"
#undef FRAMGNET_SHADER_NAME
};

static const char* VertexShaderName[] = {
#define VERTEX_SHADER_NAME
#include "shader_list.inl"
#undef VERTEX_SHADER_NAME
};

void ShaderManager::loadFragmentShader(int id, UcodeLocation place){
	void* address = Sys::File::loadFile(FragmentShaderName[id], Sys::Memory::getMainMemoryHeap(), SHADER_ALIGN);
	CGprogram program = reinterpret_cast<CGprogram>(address);
	m_fragment_shader_binary[id] = program;
	uint32_t ucode_size;
	void* ucode;
	cellGcmCgGetUCode(program, &ucode, &ucode_size);
	void* copyAddress = NULL;
	uint32_t offset = 0;
	uint8_t location = 0;
	switch(place){
		case UCODE_LOCATION_LOCAL:
		{
			Sys::Memory::VramHeap& vramheap = Sys::Memory::getLocalMemoryHeap();
			copyAddress = vramheap.alloc(ucode_size,CELL_GCM_FRAGMENT_UCODE_LOCAL_ALIGN_OFFSET);
			offset = vramheap.AddressToOffset(copyAddress);
			location = CELL_GCM_LOCATION_LOCAL;
			break;
		}
		case UCODE_LOCATION_MAIN:
		{
			Sys::Memory::VramHeap& vramheap = Sys::Memory::getMappedMainMemoryHeap();
			copyAddress = Sys::Memory::getMappedMainMemoryHeap().alloc(ucode_size,CELL_GCM_FRAGMENT_UCODE_MAIN_ALIGN_OFFSET);
			offset = vramheap.AddressToOffset(copyAddress);
			location = CELL_GCM_LOCATION_MAIN;
			break;
		}
		default:
			;
	}
	if(copyAddress){
		memcpy(copyAddress, ucode, ucode_size);
	}
	m_fragment_ucode_address[id] = copyAddress;
	m_fragment_ucode_offset[id] = offset;
	m_fragment_ucode_location[id] = location;
}

void ShaderManager::loadVertexShader(int id){
	void* address = Sys::File::loadFile(VertexShaderName[id], Sys::Memory::getMainMemoryHeap(), SHADER_ALIGN);
	m_vertex_shader_binary[id] = reinterpret_cast<CGprogram>(address);
}

void ShaderManager::init(){
	for(int id=0; id < FRAGMENT_SAHDER_ID_LAST; id++){
		loadFragmentShader(id, UCODE_LOCATION_LOCAL);
	}
	for(int id=0; id < VERTEX_SHADER_ID_LAST; id++){
		loadVertexShader(id);
	}
}

}; // namespace Render

