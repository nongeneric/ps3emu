/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#ifndef PPU_SHADER_MANAGER_H
#define PPU_SHADER_MANAGER_H

#include <Cg/cg.h>

namespace Render{
class ShaderManager{
public:

	enum FragmentShaderId{
#define FRAMGNET_SHADER_ID
#include "shader_list.inl"
#undef FRAMGNET_SHADER_ID
		FRAGMENT_SAHDER_ID_LAST
	};

	enum VertexShaderId{
#define VERTEX_SHADER_ID
#include "shader_list.inl"
#undef VERTEX_SHADER_ID
		VERTEX_SHADER_ID_LAST
	};

	enum UcodeLocation{
		UCODE_LOCATION_NONE, //J 後から配置するので、ここでは配置しない
		UCODE_LOCATION_LOCAL, //J ローカルメモリに配置する
		UCODE_LOCATION_MAIN, //J メインメモリに配置する
	};

	enum{
		SHADER_ALIGN = 16,
	};

	inline static CGprogram getFragmentShader(int id){ return m_fragment_shader_binary[id]; };
	inline static uint32_t getFragmentShaderUcodeOffset(int id){ return m_fragment_ucode_offset[id]; };
	inline static void* getFragmentShaderUcodeAddress(int id){ return m_fragment_ucode_address[id]; };
	inline static uint8_t getFragmentShaderUcodeLocation(int id){ return m_fragment_ucode_location[id]; };

	inline static CGprogram getVertexShader(int id){ return m_vertex_shader_binary[id];};
	static void init();

private:
	static CGprogram m_vertex_shader_binary[VERTEX_SHADER_ID_LAST];
	static uint32_t m_fragment_ucode_offset[FRAGMENT_SAHDER_ID_LAST];
	static void* m_fragment_ucode_address[FRAGMENT_SAHDER_ID_LAST];
	static CGprogram m_fragment_shader_binary[FRAGMENT_SAHDER_ID_LAST];
	static uint8_t m_fragment_ucode_location[FRAGMENT_SAHDER_ID_LAST];
	static void loadFragmentShader(int id, UcodeLocation place);
	static void loadVertexShader(int id);
}; // ShaderManager

}; // namespace Render

#endif //PPU_SHADER_MANAGER_H