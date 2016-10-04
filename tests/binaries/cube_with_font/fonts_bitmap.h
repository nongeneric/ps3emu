/*   SCE CONFIDENTIAL                                        */
/*PlayStation(R)3 Programmer Tool Runtime Library 400.001*/
/*   Copyright (C) 2008 Sony Computer Entertainment Inc.     */
/*   All Rights Reserved.                                    */
#ifndef INCLUDED_FONTS_BITMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset

#include <sys/types.h>   
#include <cell/font.h>
#include <cell/fontFT.h>

typedef struct {
	uint32_t code;
	CellFontGlyphMetrics Metrics;
	uint16_t w, h;
	int16_t x0, y0;
	uint8_t *Image;
} FontBitmapCharGlyph_t;

int  FontBitmapCharGlyph_Generate( FontBitmapCharGlyph_t *glyph, int count, 
                                 CellFont* cf, 
                                 float wf, float hf, float weight, float slant );

int  FontBitmapCharGlyph_Trans_blendCast_ARGB8( FontBitmapCharGlyph_t *glyph,
                                                CellFontRenderSurface* surf,
                                                float xf, float yf );
int FontBitmapCharGlyph_Trans_blendColorCast_ARGB8( FontBitmapCharGlyph_t *glyph,
                                                    CellFontRenderSurface* surf,
                                                    float xf, float yf,
                                                    uint8_t R, uint8_t G, uint8_t B );

void FontBitmapGlyph_Delete( FontBitmapCharGlyph_t *glyph, int count );


typedef struct {
	int type;
	uint32_t count;
	FontBitmapCharGlyph_t* BitmapGlyph;
	CellFont* font;
	float wf, hf;
	float weight, slant;
	uint32_t cacheN;
	
} FontBitmapCharGlyphCache_t;

int FontBitmapGlyphCache_Init( FontBitmapCharGlyphCache_t* cache, int count,
                               CellFont* cf,
                               float wf, float hf, float weight, float slant );

FontBitmapCharGlyph_t* FontBitmapGlyphCache_GetGlyph( FontBitmapCharGlyphCache_t* cache, uint32_t code );

void FontBitmapGlyphCache_End( FontBitmapCharGlyphCache_t* cache );



int FontBitmapGlyphAscii_Init( FontBitmapCharGlyphCache_t* Ascii, 
                               CellFont* cf, float wf, float hf, float weight, float slant );

FontBitmapCharGlyph_t* FontBitmapGlyphAscii_GetGlyph( FontBitmapCharGlyphCache_t* Ascii, uint32_t code );

int FontBitmapGlyphAscii_RenderPropText( FontBitmapCharGlyphCache_t* Ascii,
                                         CellFontRenderSurface* surf, float x, float y,
                                         uint8_t* utf8, float between );

void FontBitmapGlyphAscii_End( FontBitmapCharGlyphCache_t* Ascii );


typedef struct {
	CellFont Font;
	CellFontHorizontalLayout   HorizontalLayout;
	CellFontVerticalLayout     VerticalLayout;
	FontBitmapCharGlyphCache_t Ascii;
	FontBitmapCharGlyphCache_t Cache;
} FontBitmaps_t;

FontBitmaps_t* FontBitmaps_Init( FontBitmaps_t* fontBitmaps,
                                 CellFont* cf, float wf, float hf, float weight, float slant,
                                 int cacheMax );

void FontBitmaps_GetHorizontalLayout( FontBitmaps_t*fontBitmaps, CellFontHorizontalLayout*layout );
void FontBitmaps_GetVerticalLayout( FontBitmaps_t*fontBitmaps, CellFontVerticalLayout*layout );

FontBitmapCharGlyph_t* FontBitmap_GetGlyph( FontBitmaps_t*, uint32_t code );

int FontBitmaps_RenderPropText( FontBitmaps_t*,
                                CellFontRenderSurface*, float x, float y,
                                uint8_t* utf8, float between );

void FontBitmaps_End( FontBitmaps_t* );


#ifdef __cplusplus
}
#endif
#define INCLUDED_FONTS_BITMAP_H
#endif

