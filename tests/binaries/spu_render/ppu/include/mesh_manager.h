/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#ifndef PPU_MESH_MANAGER_H
#define PPU_MESH_MANAGER_H

namespace Render{
	struct Mesh{
		struct MeshAttribute{
			uint8_t location;
			uint8_t stride;
			uint8_t size;
			uint8_t type;
			uint32_t offset;
		};
		uint32_t index_offset;
		uint32_t num_vertex;
		uint32_t padding;
		uint8_t  draw_mode;
		uint8_t  index_type;
		uint8_t  index_location;
		uint8_t  _padding;
		
		MeshAttribute position;
		MeshAttribute normal;
		MeshAttribute texture;
		MeshAttribute reserved;
	};

	class MeshManager{
	public:
		enum MeshId{
			MESH_ID_DUCK,
			MESH_ID_ALL_QUAD,
			MESH_ID_LAST,
		};

		static inline const Mesh* getMesh(int id){return &mesh_data[id];}
		static void init();
	private:
		static const char* MeshName[MESH_ID_LAST];
		static Mesh mesh_data[MESH_ID_LAST];
	};
} // namepsace Render

#endif // PPU_MESH_MANAGER_H