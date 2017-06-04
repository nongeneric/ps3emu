#ifndef COMMON_DATA_H
#define COMMON_DATA_H

#ifdef __SPU__
#include <cell/gcm_spu.h>
#else // __PPU__
#include <cell/gcm.h>
#endif
#include <vectormath/cpp/vectormath_aos.h>
#include <cell/gcm/gcm_method_data.h>

#include "debug.h"
#include "base.h"
#include "offset_ptr.h"
#include "linked_list.h"

struct TextureInfo{
	uint32_t header;
	uint32_t offset;
	uint32_t format;
	uint32_t address;

	uint32_t control0;
	uint32_t control1;
	uint32_t filter;
	uint32_t image_rect;

	uint32_t border_color;
	uint32_t header_control2;
	uint32_t control2;
	uint32_t header_control3;

	uint32_t control3;
	uint32_t padding[3];
	
	inline void SetIndex(uint8_t index){
		header = CELL_GCM_METHOD_HEADER_TEXTURE_OFFSET(index, 8);
		header_control2 = CELL_GCM_METHOD_HEADER_TEXTURE_CONTROL2(index, 1);
		header_control3 = CELL_GCM_METHOD_HEADER_TEXTURE_CONTROL3(index, 1);
		padding[0] = padding[1] = padding[2] = CELL_GCM_METHOD_NOP;
	}; 
	inline void SetOffset(uint32_t offset_address){
		offset = CELL_GCM_METHOD_DATA_TEXTURE_OFFSET(offset_address);
	}
	inline void SetFormat(uint8_t location, uint8_t texformat, uint8_t mipmap,	uint8_t dimension,
	uint8_t cubemap,uint8_t border){
		format = CELL_GCM_METHOD_DATA_TEXTURE_BORDER_FORMAT(location, cubemap, dimension, texformat, mipmap, border);
	}
	inline void SetWrapAddress( uint8_t wraps,  uint8_t wrapt,  uint8_t wrapr,  uint8_t unsignedRemap,
	 uint8_t zfunc,  uint8_t gamma,  uint8_t anisoBias,  uint8_t signedRemap){
		address = CELL_GCM_METHOD_DATA_TEXTURE_ADDRESS(wraps, wrapt, wrapr, unsignedRemap, zfunc, gamma, anisoBias);
	}
	inline void SetControl(	 uint16_t minLod,  uint16_t maxLod,  uint8_t maxAniso, uint32_t alphaKill){
		control0 = CELL_GCM_METHOD_DATA_TEXTURE_CONTROL0_ALPHA_KILL((uint32_t)CELL_GCM_TRUE, minLod, maxLod, maxAniso, alphaKill);
	}
	inline void SetRemap(uint8_t RemapOrder, uint8_t OutB, uint8_t OutG, uint8_t OutR, uint8_t OutA,
		uint8_t InB, uint8_t InG, uint8_t InR, uint8_t InA){
		control1 = CELL_GCM_METHOD_DATA_TEXTURE_CONTROL1(RemapOrder, OutB, OutG, OutR, OutA, InB, InG, InR, InA);
	}
	inline void SetFilter( uint16_t bias, uint8_t min, uint8_t mag, uint8_t conv,
		uint8_t as, uint8_t rs, uint8_t gs,  uint8_t bs){
		filter = CELL_GCM_METHOD_DATA_TEXTURE_FILTER_SIGNED(bias, min, mag, conv, as, rs, gs, bs);
	}
	inline void SetBorderColor(uint32_t bcolor){
		this->border_color = CELL_GCM_METHOD_DATA_TEXTURE_BORDER_COLOR(bcolor);
	}
	inline void SetOptimization(uint8_t slope, uint8_t iso, uint8_t aniso){
		control2 = CELL_GCM_METHOD_DATA_TEXTURE_CONTROL2(slope, iso, aniso);
	}
	inline void SetSize(uint16_t height, uint16_t width, uint16_t depth, uint32_t pitch){
		image_rect = CELL_GCM_METHOD_DATA_TEXTURE_IMAGE_RECT(height, width);
		control3 = CELL_GCM_METHOD_DATA_TEXTURE_CONTROL3(pitch,depth);
	}
	static inline uint32_t getCommandSize(){return sizeof(TextureInfo)/sizeof(uint32_t);}
	inline void SetTexture(CellGcmContextData* context, uint32_t update){
		MY_ASSERT(((uintptr_t) context->current & 15) == 0);
		vec_uint4* v_cmdptr = (vec_uint4*) context->current;
		vec_uint4* v_texInfo = (vec_uint4*) this;
		v_cmdptr[0] = v_texInfo[0];
		v_cmdptr[1] = v_texInfo[1];
		v_cmdptr[2] = v_texInfo[2];
		v_cmdptr[3] = v_texInfo[3];
		context->current += update ? getCommandSize() : 0;
	}

} __attribute__ ((aligned(16)));

struct VertexInfo{
	enum{
		ATTRIBUTE_POSISION		= 0,
		ATTRIBUTE_BLENDWEIGHT	= 1,
		ATTRIBUTE_NORMAL		= 2,
		ATTRIBUTE_COLOR0		= 3,
		ATTRIBUTE_COLOR1		= 4,
		ATTRIBUTE_FOGCOORD		= 5,
		ATTRIBUTE_PSIZE			= 6,
		ATTRIBUTE_BLENDINDICES	= 7,
		ATTRIBUTE_TEXTURE_0		= 8,
		ATTRIBUTE_TEXTURE_1		= 9,
		ATTRIBUTE_TEXTURE_2		= 10,
		ATTRIBUTE_TEXTURE_3		= 11,
		ATTRIBUTE_TEXTURE_4		= 12,
		ATTRIBUTE_TEXTURE_5		= 13,
		ATTRIBUTE_TEXTURE_6		= 14,
		ATTRIBUTE_TEXTURE_7		= 15,
		ATTRIBUTE_MAX,
	};

	uint32_t format[ATTRIBUTE_MAX] __attribute__ ((aligned(16)));
	uint32_t offset[ATTRIBUTE_MAX] __attribute__ ((aligned(16)));
	uint32_t  index_count;
	uint32_t index_offset;
	uint32_t padding;
	uint8_t index_type;
	uint8_t index_location;
	uint8_t index_mode;
	uint8_t	padding0;
	
	inline void SetFormat(uint8_t index, uint16_t frequency, uint8_t stride, uint8_t size, uint8_t type){
		format[index] = CELL_GCM_METHOD_DATA_VERTEX_DATA_ARRAY_FORMAT(frequency,stride,size,type);
	}
	inline void SetInvaid(uint8_t index){
		format[index] = CELL_GCM_METHOD_DATA_VERTEX_DATA_ARRAY_FORMAT(0,0,0,CELL_GCM_VERTEX_F);
	}
	inline void SetOffset(uint8_t index, uint8_t location, uint32_t _offset){
		offset[index] = CELL_GCM_METHOD_DATA_VERTEX_DATA_ARRAY_OFFSET(location, _offset);
	}
	inline void SetIndex(uint32_t count, uint32_t offset, uint8_t type, uint8_t location, uint8_t mode){
		index_count = count;
		index_offset = offset;
		index_type = type;
		index_location = location;
		index_mode = mode;
	}
	inline void init(){
		vec_uint4* v_format = (vec_uint4*) format;
		vec_uint4* v_offset = (vec_uint4*) offset;
		vec_uint4 invalid_format = (vec_uint4){
			CELL_GCM_METHOD_DATA_VERTEX_DATA_ARRAY_FORMAT(0,0,0,CELL_GCM_VERTEX_F),
			CELL_GCM_METHOD_DATA_VERTEX_DATA_ARRAY_FORMAT(0,0,0,CELL_GCM_VERTEX_F),
			CELL_GCM_METHOD_DATA_VERTEX_DATA_ARRAY_FORMAT(0,0,0,CELL_GCM_VERTEX_F),
			CELL_GCM_METHOD_DATA_VERTEX_DATA_ARRAY_FORMAT(0,0,0,CELL_GCM_VERTEX_F),
		};
		v_format[0] = invalid_format;
		v_format[1] = invalid_format;
		v_format[2] = invalid_format;
		v_format[3] = invalid_format;
		v_offset[0] = (vec_uint4){0,0,0,0};
		v_offset[1] = (vec_uint4){0,0,0,0};
		v_offset[2] = (vec_uint4){0,0,0,0};
		v_offset[3] = (vec_uint4){0,0,0,0};
	}

	static inline uint32_t getCommandSizeVertexDataArray(){return  4 * 10;}
	inline void SetVertexDataArray(CellGcmContextData* context, uint32_t update){
		MY_ASSERT(((uintptr_t) context->current & 15) == 0);
		vec_uint4* v_cmdptr = (vec_uint4*) context->current;
		vec_uint4* v_format = (vec_uint4*) format;
		vec_uint4* v_offset = (vec_uint4*) offset;
		vec_uint4 header_format = (vec_uint4){
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_HEADER_VERTEX_DATA_ARRAY_FORMAT(ATTRIBUTE_POSISION, ATTRIBUTE_MAX),
		};
		vec_uint4 header_offset = (vec_uint4){
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_HEADER_VERTEX_DATA_ARRAY_OFFSET(ATTRIBUTE_POSISION, ATTRIBUTE_MAX),
		};
		v_cmdptr[0] = header_format;
		v_cmdptr[1] = v_format[0];
		v_cmdptr[2] = v_format[1];
		v_cmdptr[3] = v_format[2];
		v_cmdptr[4] = v_format[3];
		v_cmdptr[5] = header_offset;
		v_cmdptr[6] = v_offset[0];
		v_cmdptr[7] = v_offset[1];
		v_cmdptr[8] = v_offset[2];
		v_cmdptr[9] = v_offset[3];
		context->current += update ? getCommandSizeVertexDataArray() : 0;
	}

} __attribute__ ((aligned(16)));

struct ShaderInfo{
	uint32_t binary_size;
	spu_addr ea_ShaderBinary;
};

/*
Material Data
	---------------------
	| MaterialHeader    |
	---------------------
	| Vertex Shader     |
	---------------------
	| Fragment Shader   |
	---------------------
	| Texture[0]	    |
	---------------------
	| Texture[1..texNum]|
	---------------------
*/

struct Material{
	uint32_t size;
	uint8_t texture_number;
	uint8_t padding1[3];
	OffsetPtr<TextureInfo> texture_info;
	vec_float4	color;
	ShaderInfo vertex_shader_info;
	ShaderInfo fragment_shader_info;
	VertexInfo vertex_info;
} __attribute__ ((aligned(16)));

struct Transform{
	Vectormath::Aos::Vector3 translate;
	Vectormath::Aos::Quat rotate;
	Vectormath::Aos::Vector3 scale;
	inline const Vectormath::Aos::Matrix4 getTransformMatrix(){
		return Vectormath::Aos::Matrix4::translation(translate)
		* Vectormath::Aos::Matrix4::rotation(rotate) 
		* Vectormath::Aos::Matrix4::scale(scale);
	}
} __attribute__ ((aligned(16)));;

struct Work{
	Transform	transform;
	vec_float4	overwrite_light_color;
	uint32_t	degree;
	uint32_t	isOverwrite;
	uint32_t	padding[2];
} __attribute__ ((aligned(16)));;

struct RenderEnv{
	Vectormath::Aos::Matrix4 ProjView;
	Vectormath::Aos::Matrix4 View;
	vec_float4 directional_light_dir;
	vec_float4 directional_light_color;
	uint32_t	directional_light_degree;
	uint32_t	padding[3];
} __attribute__ ((aligned(16)));;

struct JumpPacket;
struct CallPacket;
struct DrawPacket;

struct DrawPacketBase{

	enum{
		TYPE_DRAW_COMMAND = 0,
		TYPE_CALL_PREBUILD_COMMAND = 1,
		TYPE_JUMP_PREBUILD_COMMAND = 2,
		TYPE_DEBUG_BIT = ( 1 << 2 ),
		
		TYPE_MASK = 0x3,
		TYPE_DEBUG_MASK = 0x7
	};
	
	union{
		uint32_t type;
		uint32_t material_size;
	};
	
	union{
		uint32_t	ea_jts_pre_build_command;
		uint32_t	ea_material;
	};
	
	union{
		uint32_t	offset_head_pre_build_command;
		uint32_t	ea_head_pre_build_command;
		uint32_t	ea_work;
	};
	
	inline void createCallPacket(uint32_t offset){
		type = TYPE_CALL_PREBUILD_COMMAND;
		offset_head_pre_build_command = offset;
	}
	inline void createJumpPacket(spu_addr _ea_head_pre_build_command, spu_addr _ea_jts_pre_build_command){
		type = TYPE_JUMP_PREBUILD_COMMAND;
		ea_head_pre_build_command = _ea_head_pre_build_command;
		ea_jts_pre_build_command = _ea_jts_pre_build_command;
	}
	inline void createDrawPacket(Material* material, Work* work){
		type = TYPE_DRAW_COMMAND;
		MY_ASSERT((material->size & 0x7) == 0x0);
		material_size |= material->size;
		ea_material = reinterpret_cast<uintptr_t>(material);
		ea_work = reinterpret_cast<uintptr_t>(work);
	}

	inline void setDebugBit(){type |= TYPE_DEBUG_BIT;}
	inline void unsetDebugBit(){type &= ~TYPE_DEBUG_BIT;}
	inline uint32_t checkDebugBit(){return (type & TYPE_DEBUG_BIT);}
	inline uint32_t getType(){return type & TYPE_MASK;}
	inline JumpPacket* getJumpPacket(){return reinterpret_cast<JumpPacket*>(this);}
	inline CallPacket* getCallPacket(){return reinterpret_cast<CallPacket*>(this);}
	inline DrawPacket* getDrawPacket(){return reinterpret_cast<DrawPacket*>(this);}
};

struct JumpPacket : public DrawPacketBase {
	inline uint32_t getEaPrebuildCommandHead(){return (ea_head_pre_build_command);}
	inline uint32_t getEaPrebuildCommandJts(){return (ea_jts_pre_build_command);}
};

struct CallPacket : public DrawPacketBase {
	inline uint32_t getOffsetPrebuildCommandHead(){return (offset_head_pre_build_command);}
};

struct DrawPacket : public DrawPacketBase{
	inline uint32_t getMaterialSize(){return material_size & ~TYPE_DEBUG_MASK;}
	inline uint32_t getEaMaterial(){return ea_material;}
	inline uint32_t getEaWork(){return ea_work;}
};

typedef LinkedListNode<DrawPacketBase> DrawPacketNode;
typedef LinkedList<DrawPacketNode> DrawPacketList;

#endif // COMMON_DATA_H