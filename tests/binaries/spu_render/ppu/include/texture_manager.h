/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#ifndef PPU_TEXTURE_MANAGER_H
#define PPU_TEXTURE_MANAGER_H

struct CellGcmTexture;

namespace Render{

	class TextureManager{
	public:
		enum{
			TEXTURE_ID_DUCK,
			TEXTURE_ID_LAST,
		};
		static void init();
		static inline const CellGcmTexture* getTexture(int id){return &m_texture_header[id];}

	private:
		static void loadTexture(int id, int location);
		static CellGcmTexture m_texture_header[TEXTURE_ID_LAST];
	};
} // namespace Render;


#endif // PPU_TEXTURE_MANAGER_H
