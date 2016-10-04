/*   SCE CONFIDENTIAL                                        */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2009 Sony Computer Entertainment Inc.     */
/*   All Rights Reserved.                                    */
#ifndef INCLUDED_FONTS_GRAPHICS_H
#define INCLUDED_FONTS_GRAPHICS_H

#include <cell/fontGcm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	struct {
		float x, y;
	} BaseScale;
	CellFontGlyphMetrics  Metrics;
	CellFontVertexesGlyph VertexesGlyph;
	
} FontVertexGlyph_t;

int FontGraphics_InitGcm( void* graAddr , int graSize ,
                          void* mainAddr, int mainSize,
                          uint32_t slotNumber,
                          const CellFontGraphics** graphics );

int FontGlyph_GetOutlineControlDistance( CellFontGlyph*, float maxScele, float baseDistance, float* distance );
int FontGlyph_GetVertexesGlyphSize( CellFontGlyph*, float distance, uint32_t* size );
int FontGlyph_SetupVertexesGlyph( CellFontGlyph*, float distance,
                                  uint32_t*mappedBuf, uint32_t bufSize,
                                  CellFontVertexesGlyph* vGlyph, uint32_t* dataSize );

int FontGraphics_SetupFontVertexesGlyph( CellFont* cf, uint32_t code, float maxScale,
                                         uint32_t*mappedBuf, uint32_t bufMaxSize,
                                         CellFontVertexesGlyph* vGlyph, uint32_t*dataSize,
                                         CellFontGlyphMetrics* metrics );

int FontGraphics_SetupDrawContext( const CellFontGraphics* fontGra,
                                   CellFontGraphicsDrawContext*fontDC );

float FontGraphics_GcmSetDrawGlyphArray( CellGcmContextData* gcm,
                                         CellFontRenderSurfaceGcm* surf, float x, float y,
                                         FontVertexGlyph_t* cfVertexGlyph, int count,
                                         CellFontGraphicsDrawContext* fontDC );

int FontGraphics_End( const CellFontGraphics* graphics );



#ifdef __cplusplus
}
#endif
#endif //INCLUDED_FONTS_GRAPHICS_H

