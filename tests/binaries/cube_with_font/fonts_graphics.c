/*   SCE CONFIDENTIAL                                        */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2009 Sony Computer Entertainment Inc.     */
/*   All Rights Reserved.                                    */

//J libfontGcm を使用した処理のサンプル
#include "fonts.h"
#include "fonts_graphics.h"

//J フォントグラフィクスの初期化( libgcmを使用します。)
int FontGraphics_InitGcm( void* graAddr , int graSize ,
                           void* mainAddr, int mainSize,
                           uint32_t slotNumber,
                           const CellFontGraphics** graphics )
{
	CellFontInitGraphicsConfigGcm config;
	int ret;
	
	//J フォントインターフェースライブラリ初期化パラメータの初期化
	CellFontInitGraphicsConfigGcm_initialize( &config );
	config.GraphicsMemory.address  = graAddr;
	config.GraphicsMemory.size     = graSize;
	config.MappedMainMemory.address = mainAddr;
	config.MappedMainMemory.size    = mainSize;
	config.VertexShader.slotNumber = slotNumber;
	//config.VertexShader.slotCount = //J CellFontInitGraphicsConfigGcm_initialize()による設定を保持
	
	//J フォントインターフェースライブラリ初期化
	ret = cellFontInitGraphicsGcm( &config, graphics );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "   Fonts:cellFontInitGraphicsGcm=", ret );
	}
	return ret;
}

//J CellFontVertexesGlyph作成に必要な頂点展開制御適正値の取得
int FontGlyph_GetOutlineControlDistance( CellFontGlyph* glyph, float maxScele, float baseDistance, float* distance )
{
	int ret;
	
	ret = cellFontGlyphGetOutlineControlDistance( glyph, maxScele, baseDistance, distance );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    FontsGlyph:cellFontGlyphGetOutlineControlDistance=", ret );
	}
	return ret;
}

//J CellFontVertexesGlyph作成に必要なバッファサイズの取得
int FontGlyph_GetVertexesGlyphSize( CellFontGlyph* glyph, float distance, uint32_t* size )
{
	int ret;
	
	ret = cellFontGlyphGetVertexesGlyphSize( glyph, distance, size );
	
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    FontGlyph:cellFontGetVertexesGlyphSize=", ret );
	}
	return ret;
}

//J CellFontVertexesGlyphの作成
int FontGlyph_SetupVertexesGlyph( CellFontGlyph* glyph, float distance, uint32_t*mappedBuf, uint32_t bufSize, CellFontVertexesGlyph* vGlyph, uint32_t* dataSize )
{
	int ret;

	ret = cellFontGlyphSetupVertexesGlyph( glyph, distance, mappedBuf, bufSize, vGlyph, dataSize );
	
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    FontGlyph:cellFontSetupVertexesGlyph=", ret );
	}
	return ret;
}

//J CellFontVertexesGlyphの作成。
int FontGraphics_SetupFontVertexesGlyph( CellFont* cf, uint32_t code, float maxScale, uint32_t*mappedBuf, uint32_t bufMaxSize,
                                         CellFontVertexesGlyph* vGlyph, uint32_t*dataSize, CellFontGlyphMetrics* metrics )
{
	CellFontGlyph* glyph;
	int ret;
	
	ret = Fonts_GenerateCharGlyph( cf, code, &glyph );
	
	if ( ret == CELL_OK ) {
		float distance;
		
		//J グリフの頂点展開制御適正値取得
		ret = FontGlyph_GetOutlineControlDistance( glyph, maxScale, CELL_FONT_GLYPH_OUTLINE_CONTROL_DISTANCE_DEFAULT, &distance );
		if ( ret == CELL_OK ) {
			//J 頂点グリフのセットアップ
			ret =  FontGlyph_SetupVertexesGlyph( glyph, distance, mappedBuf, bufMaxSize, vGlyph, dataSize );
			if ( ret == CELL_OK ) {
				if ( metrics ) {
					metrics->width  = glyph->Metrics.width ;
					metrics->height = glyph->Metrics.height;
					metrics->Horizontal.bearingX = glyph->Metrics.Horizontal.bearingX;
					metrics->Horizontal.bearingY = glyph->Metrics.Horizontal.bearingY;
					metrics->Horizontal.advance  = glyph->Metrics.Horizontal.advance ;
					metrics->Vertical.bearingX   = glyph->Metrics.Vertical.bearingX  ;
					metrics->Vertical.bearingY   = glyph->Metrics.Vertical.bearingY  ;
					metrics->Vertical.advance    = glyph->Metrics.Vertical.advance   ;
				}
			}
		}
		Fonts_DeleteGlyph( cf, glyph );
	}
	if ( ret != CELL_OK ) {
		if ( metrics ) {
			metrics->width =
			metrics->height=
			metrics->Horizontal.bearingX =
			metrics->Horizontal.bearingY =
			metrics->Horizontal.advance  =
			metrics->Vertical.bearingX   =
			metrics->Vertical.bearingY   =
			metrics->Vertical.advance    = 0.0f;
		}
	}
	return ret;
}

//J フォント描画コンテキストの初期化
int FontGraphics_SetupDrawContext( const CellFontGraphics* fontGra, CellFontGraphicsDrawContext*fontDC )
{
	int ret;
	
	ret = cellFontGraphicsSetupDrawContext( fontGra, fontDC );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "   Fonts:cellFontGraphicsSetupDrawContext=", ret );
	}
	return ret;
}

//J 頂点データグリフの描画
float FontGraphics_GcmSetDrawGlyphArray( CellGcmContextData* gcm, CellFontRenderSurfaceGcm* surf, float x, float y,
                                                                          FontVertexGlyph_t* vglyph, int count, CellFontGraphicsDrawContext* fontDC )
{
	int n;
	
	for ( n=0; n<count; n++ ) {
		int ret = cellFontGraphicsGcmSetDrawGlyph( gcm, surf, x, y, &vglyph[n].VertexesGlyph, fontDC );
		
		if ( ret == CELL_OK ) {
			float w, h;
			float rescaleX;
			
			cellFontGraphicsGetScalePixel( fontDC, &w, &h );
			
			rescaleX = (w / vglyph[n].BaseScale.x);
			x += rescaleX * vglyph[n].Metrics.Horizontal.advance;
		}
		else {
			Fonts_PrintError( "   Fonts:cellFontGraphicsGcmSetDrawGlyph=", ret );
		}
	}
	return x;
}

//J フォントインターフェースライブラリ初期化 ( FreeType2を使用します。)
int FontGraphics_End( const CellFontGraphics* graphics )
{
	int ret;

	//J フォントインターフェースライブラリ初期化
	ret = cellFontEndGraphics( graphics );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "   Fonts:cellFontEndGraphics=", ret );
	}
	return ret;
}

