/*   SCE CONFIDENTIAL                                        */
/*PlayStation(R)3 Programmer Tool Runtime Library 400.001*/
/*   Copyright (C) 2009 Sony Computer Entertainment Inc.     */
/*   All Rights Reserved.                                    */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset

#include <sys/types.h>   
#include <cell/font.h>
#include <cell/fontFT.h>

#include "fonts.h"

//J UTF-8 文字列から、UCS4形式で文字列取り出し
#define getUcs4 Fonts_getUcs4FromUtf8

//J サーフェス(ARGB8フォーマット)へ、ブレンディングコピー
static void cellFontRenderTrans_blendCast_ARGB8( CellFontImageTransInfo* transInfo,
                                                 uint8_t r, uint8_t g, uint8_t b );

//J テキストの幅の取得。対となるレンダリング関数と、同様のロジックにする。
float Fonts_GetPropTextWidth( CellFont* cf,
                              uint8_t* utf8, float w, float h, float slant, float between,
                              float* strWidth, uint32_t* count )
{
	uint32_t code;
	float width=0.0f;
	float d = 0.0f;
	CellFontGlyphMetrics  metrics;
	uint32_t preFontId;
	uint32_t fontId;
	uint32_t cn = 0;
	int ret;
	
	if ( (! utf8) 
	  || CELL_OK != cellFontSetScalePixel( cf, w, h )
	  || CELL_OK != cellFontSetEffectSlant( cf, slant )
	) {
		if ( strWidth ) *strWidth = 0.0f;
		if ( count    ) *count = 0;
		return width;
	}

	//J 最初の文字取得
	utf8 += getUcs4( utf8, &code, 0x3000 );
	//J 文字コードに対する、内部フォントID情報取得。
	ret = cellFontGetFontIdCode( cf, code, &fontId, (uint32_t*)0 );
	if ( ret != CELL_OK || code == 0 ) {
		if ( strWidth ) *strWidth = 0.0f;
		if ( count    ) *count = 0;
		return 0.0f;
	}
	preFontId = fontId;

	//J 最初の文字の左端を揃える。
	if ( CELL_OK == cellFontGetCharGlyphMetrics( cf, code, &metrics ) ) {
		width = -(metrics.Horizontal.bearingX);
		//J １文字目処理。文字送りと文字間の足しこみ。
		width += metrics.Horizontal.advance + between;
	}
	else {
		width = 0.0f;
		//J １文字目処理。文字送りと文字間の足しこみ。
		width += w + between;
	}
	
	for (cn=1;;cn++) {
		//J 次の文字を取得
		utf8 += getUcs4( utf8, &code, 0x3000 );
	
		if ( code == 0x00000000 ) break;
		
		//J 使用フォントを識別
		ret = cellFontGetFontIdCode( cf, code, &fontId, (uint32_t*)0 );
		if ( ret != CELL_OK ) {
			width += w + between;
			continue;
		}
		
		//J 内部的にフォントファイルが変更されていたらベアリングによる引き込みを解除。
		if ( fontId != preFontId ) {
			preFontId = fontId;
			cellFontSetEffectSlant( cf, 0.0f );
			ret = cellFontGetCharGlyphMetrics( cf, code, &metrics );
			if ( ret == CELL_OK ) {
				if ( metrics.Horizontal.bearingX < 0.0f ) {
					width += -(metrics.Horizontal.bearingX);
				}
			}
			cellFontSetEffectSlant( cf, slant );
			ret = cellFontGetCharGlyphMetrics( cf, code, &metrics );
		}
		else {
			//J メトリクス情報取得
			ret = cellFontGetCharGlyphMetrics( cf, code, &metrics );
		}
		//J 文字送りの文字間の足しこみ
		if ( ret == CELL_OK ) {
			width += d = metrics.Horizontal.advance + between;
		}
		else {
			//J 空白
			metrics.Horizontal.advance  = w;
			metrics.Horizontal.bearingX = metrics.width = 0.0f;
			width += d = w + between;
		}
	}

	if ( strWidth ) *strWidth = width;
	//J 最終文字調整。１文字前に戻して右端に合わせ再計算。
	width += - d + metrics.Horizontal.bearingX + metrics.width;

	return width;
}

//J 縦書きテキスト行の高さ取得。対となるレンダリング関数と、同様のロジックにする。
float Fonts_GetVerticalTextHeight( CellFont* cf,
                                   uint8_t* utf8, float w, float h, float between,
                                   float* strHeight, uint32_t* count )
{
	uint32_t code;
	CellFontGlyphMetrics metrics;
	float height = 0.0f;
	int flag = 0;
	float d = 0.0f;
	int n;
	
	if ( (!utf8) || *utf8 == 0x00
	  || CELL_OK != cellFontSetScalePixel( cf, w, h ) //J スケール設定
	) {
		if ( strHeight ) *strHeight = 0.0f;
		if ( count ) *count = 0;
		return 0.0f;
	}

	for ( n=0;;n++ ) {
		//J 最初の文字取り出し
		utf8 += getUcs4( utf8, &code, 0x3000 );
		
		if ( code == 0x00000000 ) break;
		
		if ( flag <= 0 ) {
			//J 句読点後の閉じ括弧上詰め処理
			if ( flag && ( code == 0x300d || code == 0x300f || code == 0xfe42 || code == 0xfe44 ) ) {
				height += - d + metrics.Vertical.bearingY + metrics.height;
			}
		}

		d = 0.0f;
		//J メトリクス情報取得
		if ( CELL_OK == cellFontGetCharGlyphMetricsVertical( cf, code, &metrics ) ) {
			//J 上端をそろえる
			if ( flag < 0 || metrics.Vertical.bearingY < 0 ) {
				height += -metrics.Vertical.bearingY;
			}
			flag = 1;
			//J 文字送り+文字間送り
			height += d = metrics.Vertical.advance + between;
			
			if ( code==0x20 || ( code >= 0x3000 && code <= 0x3002 ) ) flag = -1;
		}
		else {
			metrics.Vertical.advance  = 
			metrics.Vertical.bearingY =
			metrics.height = 0.0f;
			//J 空白
			height += d = h + between; //J 文字送り幅+文字間+指定。
			flag = -1;
		}
	}
	if ( strHeight ) *strHeight = height;
	if ( count ) *count = n;
	
	height += - d + metrics.Vertical.bearingY + metrics.height;//J １文字前に戻して下端に合わせ再計算。
	
	return height;
}

//J 文字列幅を指定幅に収めるための、縮尺率を求めます。
//J 一文字で最大1/64ピクセル程度の誤差はあります。
float Fonts_GetTextRescale( float scale, float w, float newW, float*ratio )
{
	float rate, rescale;

	rate = (newW/w);
	rescale = scale * rate;
	#if 0
	//J 用意するフォントによっては、浮動小数点のスケール指定を受け付けません。
	//J その場合、文字幅が四捨五入されるため、レンダリング結果が指定幅を超えて
	//J しまう場合あります。そういう場合は、前もって整数に丸めておきます。
	{
		//J 小数部切捨て
		int i_scale = (int)(rescale);
		rescale = (float)i_scale;
		rate = rescale/scale;
	}
	#endif
	
	if ( ratio ) *ratio = rate;
	
	return rescale;
}
float Fonts_GetPropTextWidthRescale( float scale, float w, float newW, float*ratio )
{
	return Fonts_GetTextRescale( scale, w, newW, ratio );
}
float Fonts_GetVerticalTextHeightRescale( float scale, float h, float newH, float*ratio )
{
	return Fonts_GetTextRescale( scale, h, newH, ratio );
}

int Fonts_BindRenderer( CellFont* cf, CellFontRenderer* rend )
{
	//J レンダラー接続
	int ret = cellFontBindRenderer( cf, rend );
	
	return ret;
}

int Fonts_UnbindRenderer( CellFont* cf ) 
{
	//J レンダラー接続解除。
	int ret = cellFontUnbindRenderer( cf );
	
	return ret;
}


//J テキスト行のレンダリング。
float Fonts_RenderPropText( CellFont* cf,
                            CellFontRenderSurface* surf, float x, float y,
                            uint8_t* utf8, float w, float h, float slant, float between )
{
	uint32_t code;
	CellFontGlyphMetrics  metrics;
	CellFontImageTransInfo TransInfo;
	uint32_t preFontId;
	uint32_t fontId;
	int ret;
	
	if ( (!utf8) || *utf8 == 0x00 ) return x;

	//J レンダリングスケールでレンダリングバッファ初期化。
	ret = cellFontSetupRenderScalePixel( cf, w, h );
	if ( ret != CELL_OK ) {
		Fonts_PrintError("Fonts_RenderPropText:",ret);
		return x;
	}

	//J 最初の文字取り出し
	utf8 += getUcs4( utf8, &code, 0x3000 );

	//J 最初の文字の左合わせ
	{
		cellFontGetFontIdCode( cf, code, &preFontId, (uint32_t*)0 );
		
		//J 文字コードに対する、内部フォントID情報取得。
		if ( CELL_OK == cellFontGetRenderCharGlyphMetrics( cf, code, &metrics ) ) {
			//J 左端を揃える。
			x += -(metrics.Horizontal.bearingX);
		}
	}

	for ( ;; ) {
		//J レンダリング
		ret = cellFontRenderCharGlyphImage( cf, code, surf, x, y, &metrics, &TransInfo );
		if ( ret == CELL_OK ) {
			//J 文字送り+文字間送り
			x += metrics.Horizontal.advance + between;
			
			//J サーフェスへコピー（サンプルソース内の関数です。）
			cellFontRenderTrans_blendCast_ARGB8( &TransInfo, 255, 255, 255 );
		}
		else {
			//J 空白
			x += w + between; //J 文字送り幅+文字間+指定。
		}

		//J 次の文字を取得
		utf8 += getUcs4( utf8, &code, 0x3000 );
		
		if ( code == 0x00000000 ) break;
		
		//J 文字コードに対する、内部フォントID情報取得。
		cellFontGetFontIdCode( cf, code, &fontId, (uint32_t*)0 );
		
		//J 内部的にフォントファイルが変更されていたらベアリングによる引き込みを解除。
		if ( fontId != preFontId ) {
			preFontId = fontId;
			cellFontSetupRenderEffectSlant( cf, 0.0f );
			if ( CELL_OK == cellFontGetRenderCharGlyphMetrics( cf, code, &metrics ) ) {
				if ( metrics.Horizontal.bearingX < 0.0f ) {
					x += -(metrics.Horizontal.bearingX);
				}
			}
			cellFontSetupRenderEffectSlant( cf, slant );
		}
	}
	
	return x;
}

//J 縦書きテキスト行のレンダリング
float Fonts_RenderVerticalText( CellFont* cf,
                            CellFontRenderSurface* surf, float x, float y,
                            uint8_t* utf8, float w, float h, float slant, float between )
{
	uint32_t code;
	CellFontGlyphMetrics   metrics;
	CellFontImageTransInfo TransInfo;
	int flag = 0;
	float d = 0.0f;
	int ret;
	
	if ( (!utf8) || *utf8 == 0x00 ) return y;

	//J レンダリングスケールでレンダリングバッファ初期化。
	ret = cellFontSetupRenderScalePixel( cf, w, h );
	if ( ret != CELL_OK ) {
		Fonts_PrintError("Fonts_RenderVerticalText:",ret);
		return y;
	}
	
	for ( ;; ) {
		//J 最初の文字取り出し
		utf8 += getUcs4( utf8, &code, 0x3000 );
		
		if ( code == 0x00000000 ) break;
		
		if ( flag <= 0 ) {
			//J 句読点後の閉じ括弧上詰め処理
			if ( flag && ( code == 0x300d || code == 0x300f || code == 0xff42 || code == 0xff4f ) ) {
				y += d = -d + metrics.Vertical.bearingY + metrics.height;
				x -= d * slant;
			}
		}
		d = 0.0f;
		//J メトリクス情報取得
		if ( CELL_OK == cellFontGetRenderCharGlyphMetricsVertical( cf, code, &metrics ) ) {
			//J 上端をそろえる
			if ( flag < 0 || metrics.Vertical.bearingY < 0 ) {
				y += d = -metrics.Vertical.bearingY;
				x -= d * slant;
			}
		}
		flag = 1;
		//J レンダリング
		ret = cellFontRenderCharGlyphImageVertical( cf, code, surf, x, y, &metrics, &TransInfo );
		if ( ret == CELL_OK ) {
			//J 文字送り+文字間送り
			y += d = metrics.Vertical.advance + between;
			x -= d * slant;
			
			//J サーフェスへコピー（サンプルソース内の関数です。）
			cellFontRenderTrans_blendCast_ARGB8( &TransInfo, 255, 255, 255 );
			
			//空白、句読点、の後の余白を詰める。
			if ( code==0x20 || ( code >= 0x3000 && code <= 0x3002 ) ) flag = -1;
		}
		else {
			//J 上端そろえのキャンセル
			y -= d;
			x += d * slant;
			//J 空白
			metrics.Vertical.advance  =
			metrics.Vertical.bearingY =
			metrics.height = 0.0f;
			y += d = h + between; //J 文字送り幅+文字間+指定。
			x -= d * slant;
			flag = -1;
		}
	}
	
	return y;
}

//J サーフェス(ARGB8フォーマット)へ、ブレンディングコピー
static void cellFontRenderTrans_blendCast_ARGB8( CellFontImageTransInfo* transInfo,
                                                 uint8_t r, uint8_t g, uint8_t b )
{
	if ( transInfo ) {
		unsigned char* tex;
		unsigned char* img = transInfo->Image;
		int img_bw = transInfo->imageWidthByte;
		int tex_bw = transInfo->surfWidthByte;
		int w = transInfo->imageWidth;
		int h = transInfo->imageHeight;
		uint64_t a0,a1;
		uint64_t ARGBx2;
		uint64_t A0, R0, G0, B0;
		uint64_t A1, R1, G1, B1;
		int x, y;
		
		uint64_t _r, _b, _g;
		_r=r;
		_g=g;
		_b=b;
		
		for ( y=0; y < h; y++ ) {
			tex = ((uint8_t*)transInfo->Surface) + tex_bw*y;
			for ( x=0; x < w; x++ ) {
				if ( (((sys_addr_t)tex) & 7) || x == w-1 ) {
					a1 = img[x];
					if ( a1 ) {
						ARGBx2 = *(uint32_t*)tex;
						A1 = (255-a1);
						R1 = ((ARGBx2>>16)&0xff);
						G1 = ((ARGBx2>> 8)&0xff);
						B1 = ((ARGBx2>> 0)&0xff);
						*(uint32_t*)tex =
						             (a1<<24) |
						             (( A1 * R1 + a1 * _r )/255<<16)| //J 背景とブレンド
						             (( A1 * G1 + a1 * _g )/255<< 8)| //J 背景とブレンド
						             (( A1 * B1 + a1 * _b )/255    ); //J 背景とブレンド
					}
					tex += 4;
				}
				else{
					a0 = img[x]; x++;
					a1 = img[x];
					if ( a0 || a1 ) {
						ARGBx2 = *(uint64_t*)tex;
						A0 = (255-a0);
						A1 = (255-a1);
						R0 = ((ARGBx2>>48)&0xff);
						G0 = ((ARGBx2>>40)&0xff);
						B0 = ((ARGBx2>>32)&0xff);
						R1 = ((ARGBx2>>16)&0xff);
						G1 = ((ARGBx2>> 8)&0xff);	
						B1 = ((ARGBx2>> 0)&0xff);
						*(uint64_t*)tex =
						             (a0<<56) |
						             (( A0 * R0 + a0 * _r )/255<<48)| //J 背景とブレンド
						             (( A0 * G0 + a0 * _g )/255<<40)| //J 背景とブレンド
						             (( A0 * B0 + a0 * _b )/255<<32)| //J 背景とブレンド
						             (a1<<24) |
						             (( A1 * R1 + a1 * _r )/255<<16)| //J 背景とブレンド
						             (( A1 * G1 + a1 * _g )/255<< 8)| //J 背景とブレンド
						             (( A1 * B1 + a1 * _b )/255    ); //J 背景とブレンド
					}
					tex += 8;
				}
			}
			img += img_bw;
		}
	}
	return;
}

#if 0
//J サーフェス(ARGB8フォーマット)へ、αの高い方優先でコピー
static void cellFontRenderTrans_AlphaCast_ARGB8( CellFontImageTransInfo* transInfo,
                               unsigned char r, unsigned char g, unsigned char b )
{
	if ( transInfo ) {
		unsigned char* tex;
		unsigned char* img = transInfo->Image;
		int img_bw = transInfo->imageWidthByte;
		int tex_bw = transInfo->surfWidthByte;
		int w = transInfo->imageWidth;
		int h = transInfo->imageHeight;
		uint64_t a;
		uint64_t _r, _b, _g;
		int x, y;
		
		_r=r;
		_g=g;
		_b=b;
		
		for ( y=0; y < h; y++ ) {
			tex = ((uint8_t*)transInfo->Surface) + tex_bw*y;
			for ( x=0; x < w; x++ ) {
				a = (uint64_t)img[x];
				//J 無地のサーフェスに文字を並べる時は、前の文字とピクセルが
				//J 重なる場合を考慮し、濃い方を優先する。
				if ( a > (uint64_t)*tex ) {
					*(uint32_t*)tex = (a<<24)|(_r<<16)|(_g<< 8)|(_b);
				}
				tex += 4;
			}
			img += img_bw;
		}
	}
	return;
}
#endif

