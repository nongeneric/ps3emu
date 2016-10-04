/*   SCE CONFIDENTIAL                                        */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2009 Sony Computer Entertainment Inc.     */
/*   All Rights Reserved.                                    */
#ifndef INCLUDED_FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cell/sysmodule.h>
#include <cell/font.h>
#include <cell/fontFT.h>
#include <cell/fontGcm.h>

#define FONT_FILE_CACHE_SIZE (1*1024*1024) //1MB

#define SYSTEM_FONT_MAX (16)
#define USER_FONT_MAX   (32-SYSTEM_FONT_MAX)

enum {
	FONT_SYSTEM_FONT0 = 0,
	 FONT_SYSTEM_GOTHIC_LATIN = FONT_SYSTEM_FONT0,
	 FONT_SYSTEM_GOTHIC_JP,
	 FONT_SYSTEM_SANS_SERIF,
	 FONT_SYSTEM_SERIF,
	FONT_USER_FONT0    = SYSTEM_FONT_MAX
}FontEnum;

#define FONT_ENABLE_BIT(n)  (1<<(n))

typedef struct 
{
	int sysFontMax;
	CellFont SystemFont[ SYSTEM_FONT_MAX ];

	int userFontMax;
	CellFont UserFont[ USER_FONT_MAX ];

	uint32_t openState;
} Fonts_t;

int Fonts_LoadModules( void );

Fonts_t* Fonts_Init( void );

int Fonts_InitLibraryFreeType( const CellFontLibrary** );

int Fonts_CreateRenderer( const CellFontLibrary*, uint32_t initSize, CellFontRenderer* );

int Fonts_OpenFonts( const CellFontLibrary*, Fonts_t* );

int Fonts_GetFontsHorizontalLayout( Fonts_t*, uint32_t fontmask, float scale, float* lineHeight, float*baseLineY );

int Fonts_AttachFont( Fonts_t* fonts, int fontEnum, CellFont*cf );

int Fonts_SetFontScale( CellFont* cf, float scale );
int Fonts_SetFontEffectWeight( CellFont* cf, float effWeight );
int Fonts_SetFontEffectSlant( CellFont* cf, float effSlant );

int Fonts_GetFontHorizontalLayout( CellFont* cf, float* lineHeight, float*baseLineY );
int Fonts_GetFontVerticalLayout( CellFont* cf, float* lineWidth, float*baseLineX );

float Fonts_GetPropTextWidth( CellFont*, 
                              uint8_t* utf8, float xScale, float yScale, float slant, float between,
                              float* strWidth, uint32_t* count );

float Fonts_GetVerticalTextHeight( CellFont*,
                                   uint8_t* utf8, float w, float h, float between,
                                   float* strHeight, uint32_t* count );

float Fonts_GetTextRescale( float scale, float w, float newW, float*ratio );
float Fonts_GetPropTextWidthRescale( float scale, float w, float newW, float*ratio );
float Fonts_GetVerticalTextHeightRescale( float scale, float h, float newH, float*ratio );


int Fonts_BindRenderer( CellFont*, CellFontRenderer* rend );

float Fonts_RenderPropText( CellFont*,
                            CellFontRenderSurface* surf, float x, float y,
                            uint8_t* utf8, float xScale, float yScale, float slant, float between );

float Fonts_RenderVerticalText( CellFont* cf,
                            CellFontRenderSurface* surf, float x, float y,
                            uint8_t* utf8, float xScale, float yScale, float slant, float between );

int Fonts_UnbindRenderer( CellFont* );

int Fonts_GenerateCharGlyph( CellFont* cf, uint32_t code, CellFontGlyph** glyph );
int Fonts_DeleteGlyph( CellFont* cf, CellFontGlyph* );

int Fonts_DetachFont( CellFont*cf );

int Fonts_CloseFonts( Fonts_t* );

int Fonts_DestroyRenderer( CellFontRenderer* rend );

int Fonts_EndLibrary( const CellFontLibrary* lib );
int Fonts_End( void );

void Fonts_UnloadModules( void );

void Fonts_PrintError( const char*mess, int d );

uint32_t Fonts_getUcs4FromUtf8( uint8_t*utf8, uint32_t*ucs4, uint32_t alterCode );

#ifdef __cplusplus
}
#endif
#define INCLUDED_FONTS_H
#endif

