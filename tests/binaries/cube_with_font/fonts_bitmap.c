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
#include "fonts_bitmap.h"

//J UTF-8 文字列から、UCS4形式で文字列取り出し
#define getUcs4 Fonts_getUcs4FromUtf8

//J 文字イメージオブジェクト生成
int FontBitmapCharGlyph_Generate( FontBitmapCharGlyph_t *glyph, int count, 
                                  CellFont* cf, 
                                  float wf, float hf, float weight, float slant )
{
	CellFontRenderSurface    Surface;
	CellFontGlyphMetrics     metrics;
	CellFontImageTransInfo   TransInfo;
	int x,y;
	int w,h;
	int ret;

	if ( CELL_OK != (ret = cellFontSetupRenderScalePixel( cf, wf, hf ) )
	  || CELL_OK != (ret = cellFontSetupRenderEffectWeight( cf, weight ) )
	  || CELL_OK != (ret = cellFontSetupRenderEffectSlant( cf, slant )   )
	) {
		return ret;
	}
	
	x = ((int)wf) * 2;
	y = ((int)hf) * 2;
	w = x*2;
	h = y*2;
	
	cellFontRenderSurfaceInit( &Surface, (void*)0, w, 1, w, h );
	cellFontRenderSurfaceSetScissor( &Surface, 0, 0, w, h );
	
	for ( ; count; count-- ) {
	
		ret = cellFontRenderCharGlyphImage( cf, glyph->code, &Surface, (float)x, (float)y, &metrics, &TransInfo );
		if ( ret == CELL_OK ) {
			int ibw, size;
			
			ibw = TransInfo.imageWidthByte;
			glyph->w = TransInfo.imageWidth;
			glyph->h = TransInfo.imageHeight;
			
			size = glyph->w * glyph->h;
			if ( size ) {
				glyph->Image = malloc( size );
				
				if ( glyph->Image ) {
					int ix, iy;
					for ( iy = 0; iy < glyph->h; iy++ ) {
						for ( ix = 0; ix < glyph->w; ix++ ) {
							glyph->Image[iy*glyph->w+ix] = TransInfo.Image[ iy*ibw+ix ];
						}
					}
				}
			}
			else {
				glyph->Image = (uint8_t*)0;
			}
			ibw = TransInfo.surfWidthByte;
			glyph->x0 = (int)TransInfo.Surface % ibw - x;
			glyph->y0 = (int)TransInfo.Surface / ibw - y;
			
			glyph->Metrics = metrics;
		}
		else {
			glyph->Image = (uint8_t*)0;
		}
		glyph++;
	}
	return CELL_OK;
}


//J 文字イメージオブジェクトからテクスチャへ転送
//J イメージのクオリティは、サブピクセル分のシフトがあるため元イメージより劣化します。
int FontBitmapCharGlyph_Trans_blendCast_ARGB8( FontBitmapCharGlyph_t *glyph,
                                               CellFontRenderSurface* surf, float xf, float yf )
{
	int xi, yi;
	int x, y;
	int sx, sy;
	float f0,f1,f2,f3;
	
	{
		float xm0, ym0, xm1, ym1;
		xi = (int)xf;
		if ( xf >= 0.0f ) {
			xm1 = xf - (float)xi;
			xm0 = 1.0f - xm1;
		}
		else {
			xm0 = -( xf - (float)xi );
			xm1 = 1.0f - xm0;
			xi--;
		}
		yi = (int)yf;
		if ( yf >= 0.0f ) {
			ym1 = yf - (float)yi;
			ym0 = 1.0f - ym1;
		}
		else {
			ym0 = -( yf - (float)yi );
			ym1 = 1.0f - ym0;
			yi--;
		}
		xi += (int)glyph->x0;
		yi += (int)glyph->y0;
		
		f0 = xm0 * ym0;
		f1 = xm1 * ym0;
		f2 = xm0 * ym1;
		f3 = xm1 * ym1;
	}
	#if 0
	int n = 0;
	
	for ( y = 0; y < glyph->h; y++ ) {
		for ( x = 0; x < glyph->w; x++ ) {
			unsigned char *p;
			int level, a;
			level = glyph->Image[ n ];
			n++;
			
			sx = xi + x;
			sy = yi + y;
			p = (uint8_t*)surf->buffer + (sy*surf->widthByte + sx*4);
			
			if ( sy >= (int)surf->Scissor.y0 && sy < (int)surf->Scissor.y1 ) {
				if ( sx >= (int)surf->Scissor.x0 && sx < (int)surf->Scissor.x1 ) {
					a = level * f0;
					if ( x || y ) {
						a += p[0];
					}
					else if(a==0) p[0] = a;
					if ( a ) {
						uint32_t rgba = a;
						rgba = (rgba<<8)|( p[1] * (255-a) + 255 * a)/ 255;
						rgba = (rgba<<8)|( p[2] * (255-a) + 255 * a)/ 255;
						rgba = (rgba<<8)|( p[3] * (255-a) + 255 * a)/ 255;
						*(uint32_t*)&p[0] = rgba;
					}
				}
				sx++;
				if ( sx >= (int)surf->Scissor.x0 && sx <  (int)surf->Scissor.x1 ) {
					a = level * f1;
					if ( y ) a += p[4];
					else if(a==0) p[4] = a;
					if ( a ) {
						if ( x == glyph->w-1 ) {
							uint32_t rgba = a;
							rgba = (rgba<<8)|( p[5] * (255-a) + 255 * a)/ 255;
							rgba = (rgba<<8)|( p[6] * (255-a) + 255 * a)/ 255;
							rgba = (rgba<<8)|( p[7] * (255-a) + 255 * a)/ 255;
							*(uint32_t*)&p[4] = rgba;
						}
						else p[4] = a;
					}
				}
				sx--;
			}
			sy++;
			if ( sy >= (int)surf->Scissor.y0 && sy < (int)surf->Scissor.y1 ) {
				p += surf->widthByte;
				if ( sx >= (int)surf->Scissor.x0 && sx < (int)surf->Scissor.x1 ) {
					a = level * f2;
					if ( x ) a += p[0];
					else if(a==0) p[0] = a;
					if ( a ) {
						if ( y == glyph->h-1 ) {
							uint32_t rgba = a;
							rgba = (rgba<<8)|( p[1] * (255-a) + 255 * a)/ 255;
							rgba = (rgba<<8)|( p[2] * (255-a) + 255 * a)/ 255;
							rgba = (rgba<<8)|( p[3] * (255-a) + 255 * a)/ 255;
							*(uint32_t*)&p[0] = rgba;
						}
						else p[0] = a;
					}
				}
				sx++;
				if ( sx >= (int)surf->Scissor.x0 && sx < (int)surf->Scissor.x1 ) {
					a = level * f3;
					p[4] = a;
				}
			}
		}
	}
	#else
	{
		int w, h;
		w = glyph->w;
		h = glyph->h;
		if ( xf != (float)(int)xf ) w++;
		if ( yf != (float)(int)yf ) h++;
		
		for ( sy = (yi < (int)surf->Scissor.y0)?(int)surf->Scissor.y0:yi; sy < (int)surf->Scissor.y1; sy++ ) {
			y = ( sy - yi );
			if ( y >= h ) break;
			
			for ( sx = (xi < (int)surf->Scissor.x0)?(int)surf->Scissor.x0:xi; sx < (int)surf->Scissor.x1; sx++ ) {
				unsigned char *p;
				int a;
				
				x = ( sx - xi );
				if ( x >= w ) break;
				
				a = 0;
				if ( y ) {
					if ( x ) {
						a += (int)(f3 * glyph->Image[ (y-1)*glyph->w + x - 1 ]);
					}
					if ( x < glyph->w ) {
						a += (int)(f2 * glyph->Image[ (y-1)*glyph->w + x ]);
					}
				}
				if ( y < glyph->h ) {
					if ( x ) {
						a += (int)(f1 * glyph->Image[ (y)*glyph->w + x -1 ]);
					}
					if ( x < glyph->w ) {
						a += (int)(f0 * glyph->Image[ (y)*glyph->w + x ]);
					}
				}
				
				p = (uint8_t*)surf->buffer + (sy*surf->widthByte + sx*4);
				if ( a ) {
					#if 1
					//J サーフェスのアルファを操作しない
					uint32_t rgba = p[0];
					rgba = (rgba<<8) | ( p[1] * (255-a) + 255 * a ) / 255;
					rgba = (rgba<<8) | ( p[2] * (255-a) + 255 * a ) / 255;
					rgba = (rgba<<8) | ( p[3] * (255-a) + 255 * a ) / 255;
					*(uint32_t*)&p[0] = rgba;
					#else 
					//J アルファを残して、隣接文字の接触部が潰れないよう考慮
					//J（サーフェス初期状態のalpah値が、0クリアされている必要あり）
					int32_t a0 = p[0];
					if ( a0 ) {
						if ( a > a0 ) {
							uint32_t rate = 255*(255-a)/(255-a0);
							uint32_t rgba = a;
							rgba = (rgba<<8) | ( ( p[1] * 255 - 255 * a0 ) * rate / 255 + 255 * a ) / 255;
							rgba = (rgba<<8) | ( ( p[2] * 255 - 255 * a0 ) * rate / 255 + 255 * a ) / 255;
							rgba = (rgba<<8) | ( ( p[3] * 255 - 255 * a0 ) * rate / 255 + 255 * a ) / 255;
							*(uint32_t*)&p[0] = rgba;
						}
					}
					else {
						uint32_t rgba = a;
						rgba = (rgba<<8) | ( p[1] * (255-a) + 255 * a) / 255;
						rgba = (rgba<<8) | ( p[2] * (255-a) + 255 * a) / 255;
						rgba = (rgba<<8) | ( p[3] * (255-a) + 255 * a) / 255;
						*(uint32_t*)&p[0] = rgba;
					}
					#endif
				}
			}
		}
	}
	#endif
	return CELL_OK;
}

//J 文字イメージオブジェクトからテクスチャへ転送
//J イメージのクオリティは、サブピクセル分のシフトがあるため元イメージより劣化します。
int FontBitmapCharGlyph_Trans_blendColorCast_ARGB8( FontBitmapCharGlyph_t *glyph,
                                                    CellFontRenderSurface* surf, float xf, float yf, uint8_t R, uint8_t G, uint8_t B )
{
	int xi, yi;
	int x, y;
	int sx, sy;
	float f0,f1,f2,f3;
	
	{
		float xm0, ym0, xm1, ym1;
		xi = (int)xf;
		if ( xf >= 0.0f ) {
			xm1 = xf - (float)xi;
			xm0 = 1.0f - xm1;
		}
		else {
			xm0 = -( xf - (float)xi );
			xm1 = 1.0f - xm0;
			xi--;
		}
		yi = (int)yf;
		if ( yf >= 0.0f ) {
			ym1 = yf - (float)yi;
			ym0 = 1.0f - ym1;
		}
		else {
			ym0 = -( yf - (float)yi );
			ym1 = 1.0f - ym0;
			yi--;
		}
		xi += (int)glyph->x0;
		yi += (int)glyph->y0;
		
		f0 = xm0 * ym0;
		f1 = xm1 * ym0;
		f2 = xm0 * ym1;
		f3 = xm1 * ym1;
	}
	
	{
		int w, h;
		w = glyph->w;
		h = glyph->h;
		if ( xf != (float)(int)xf ) w++;
		if ( yf != (float)(int)yf ) h++;
		
		for ( sy = (yi < (int)surf->Scissor.y0)?(int)surf->Scissor.y0:yi; sy < (int)surf->Scissor.y1; sy++ ) {
			y = ( sy - yi );
			if ( y >= h ) break;
			
			for ( sx = (xi < (int)surf->Scissor.x0)?(int)surf->Scissor.x0:xi; sx < (int)surf->Scissor.x1; sx++ ) {
				unsigned char *p;
				int a;
				
				x = ( sx - xi );
				if ( x >= w ) break;
				
				a = 0;
				if ( y ) {
					if ( x ) {
						a += (int)(f3 * glyph->Image[ (y-1)*glyph->w + x - 1 ]);
					}
					if ( x < glyph->w ) {
						a += (int)(f2 * glyph->Image[ (y-1)*glyph->w + x ]);
					}
				}
				if ( y < glyph->h ) {
					if ( x ) {
						a += (int)(f1 * glyph->Image[ (y)*glyph->w + x -1 ]);
					}
					if ( x < glyph->w ) {
						a += (int)(f0 * glyph->Image[ (y)*glyph->w + x ]);
					}
				}
				
				p = (uint8_t*)surf->buffer + (sy*surf->widthByte + sx*4);
				if ( a ) {
					#if 1
					//J サーフェスのアルファを操作しない
					uint32_t rgba = p[0];
					rgba = (rgba<<8) | ( p[1] * (255-a) + R * a ) / 255;
					rgba = (rgba<<8) | ( p[2] * (255-a) + G * a ) / 255;
					rgba = (rgba<<8) | ( p[3] * (255-a) + B * a ) / 255;
					*(uint32_t*)&p[0] = rgba;
					#else 
					//J アルファを残して、隣接文字の接触部が潰れないよう考慮
					//J（サーフェス初期状態のalpah値が0である必要あり）
					int32_t a0 = p[0];
					if ( a0 ) {
						if ( a > a0 ) {
							uint32_t rate = 255*(255-a)/(255-a0);
							uint32_t rgba = a;
							rgba = (rgba<<8) | ( ( p[1] * 255 - R * a0 ) * rate / 255 + R * a ) / 255;
							rgba = (rgba<<8) | ( ( p[2] * 255 - G * a0 ) * rate / 255 + G * a ) / 255;
							rgba = (rgba<<8) | ( ( p[3] * 255 - B * a0 ) * rate / 255 + B * a ) / 255;
							*(uint32_t*)&p[0] = rgba;
						}
					}
					else {
						uint32_t rgba = a;
						rgba = (rgba<<8) | ( p[1] * (255-a) + R * a) / 255;
						rgba = (rgba<<8) | ( p[2] * (255-a) + G * a) / 255;
						rgba = (rgba<<8) | ( p[3] * (255-a) + B * a) / 255;
						*(uint32_t*)&p[0] = rgba;
					}
					#endif
				}
			}
		}
	}
	return CELL_OK;
}


//J 文字イメージオブジェクトの破棄
void FontBitmapGlyph_Delete( FontBitmapCharGlyph_t *glyph, int count )
{
	int n;

	if ( glyph ) {
		for ( n = 0; n < count; n++ ) {
			void* p = glyph[ n ].Image;
			if ( p ) free(p);
		}
	}
}


//J 文字イメージオブジェクトの半角文字作り置き
int FontBitmapGlyphAscii_Init( FontBitmapCharGlyphCache_t* Ascii, 
                               CellFont* cf, float wf, float hf, float weight, float slant )
{
	uint32_t n;
	int ret = !CELL_OK;

	if (! Ascii ) return ret;

	Ascii->count = 0x7E - 0x20 + 1;
	Ascii->wf = wf;
	Ascii->hf = hf;
	Ascii->weight = weight;
	Ascii->slant  = slant;
	
	Ascii->BitmapGlyph = malloc( Ascii->count * sizeof( FontBitmapCharGlyph_t ) );

	if ( Ascii->BitmapGlyph ) {
		for ( n = 0; n < Ascii->count; n++ ) {
			Ascii->BitmapGlyph[ n ].code = 0x20 + n;
		}
		FontBitmapCharGlyph_Generate( Ascii->BitmapGlyph, Ascii->count, 
		                              cf, wf, hf, weight, slant );
		return CELL_OK;
	}
	return ret;
}

//J 文字イメージオブジェクトの取得（半角文字専用)
FontBitmapCharGlyph_t* FontBitmapGlyphAscii_GetGlyph( FontBitmapCharGlyphCache_t* Ascii, uint32_t code )
{
	int i;
	
	if ( code < 0x80 ) {
		i = ( code < 0x20 || code > 0x7E )? 0 : code - 0x20;
		return &Ascii->BitmapGlyph[ i ];
	}
	return (FontBitmapCharGlyph_t*)0;
}

//J 文字イメージを使用したテキストテキスト（半角文字専用)
int FontBitmapGlyphAscii_RenderPropText( FontBitmapCharGlyphCache_t* ascii,
                                         CellFontRenderSurface* surf, float x, float y,
                                         uint8_t* utf8, float between )
{
	FontBitmapCharGlyph_t* codeGlyph;
	uint32_t code;
	
	if ( (!utf8) || *utf8 == 0x00 ) return x;
	
	//J 最初の文字取り出し
	utf8 += getUcs4( utf8, &code, 0x3000 );
	
	codeGlyph = FontBitmapGlyphAscii_GetGlyph( ascii, code );
	
	//J 最初の文字の左合わせ
	x += -(codeGlyph->Metrics.Horizontal.bearingX);
	
	for (;;) {
		//J レンダリング
		if ( codeGlyph->Image ) {
			FontBitmapCharGlyph_Trans_blendCast_ARGB8( codeGlyph, surf, x, y );
		}
		else {
			codeGlyph = FontBitmapGlyphAscii_GetGlyph( ascii, 0x0020 );
		}
		x += codeGlyph->Metrics.Horizontal.advance + between;

		//J 次の文字を取得
		utf8 += getUcs4( utf8, &code, 0x3000 );
		
		if ( code == 0x00000000 ) break;
		codeGlyph = FontBitmapGlyphAscii_GetGlyph( ascii, code );
	}
	return x;
}

//J 文字イメージを破棄（半角文字）
void FontBitmapGlyphAscii_End( FontBitmapCharGlyphCache_t* Ascii )
{
	if ( Ascii ) {
		FontBitmapGlyph_Delete( Ascii->BitmapGlyph, Ascii->count );
	}
}

//J 文字イメージキャッシュの作成
int FontBitmapGlyphCache_Init( FontBitmapCharGlyphCache_t* cache, int count,
                               CellFont* cf,
                               float wf, float hf, float weight, float slant )
{
	int n, ret = !CELL_OK;

	if (! cache ) return CELL_OK;

	cache->font  = cf;
	cache->count = count;
	cache->wf = wf;
	cache->hf = hf;
	cache->weight = weight;
	cache->slant  = slant;
	
	cache->BitmapGlyph = malloc( count * sizeof( FontBitmapCharGlyph_t ) );

	if ( cache->BitmapGlyph ) {
		for ( n = 0; n < count; n++ ) {
			cache->BitmapGlyph[ n ].code  = 0x00000000;
			cache->BitmapGlyph[ n ].Image = (uint8_t*)0;
		}
		return CELL_OK;
	}
	return ret;
}

//J 文字イメージキャッシュのから、文字イメージ取得
FontBitmapCharGlyph_t* FontBitmapGlyphCache_GetGlyph( FontBitmapCharGlyphCache_t* cache, uint32_t code )
{
	FontBitmapCharGlyph_t* codeGlyph;
	uint32_t n;
	int ret;
	
	if ( code == 0x0000 ) return (FontBitmapCharGlyph_t*)0;
	
	for ( n = 0; n < cache->count; n++ ) {
		codeGlyph = &cache->BitmapGlyph[ n ];
		
		if ( codeGlyph->code == code ) {
			return codeGlyph;
		}
	}
	for ( n = 0; n < cache->count; n++ ) {
		if (! cache->BitmapGlyph[ n ].Image ) {
			codeGlyph = &cache->BitmapGlyph[ n ];
			
			codeGlyph->code = code;
			ret = FontBitmapCharGlyph_Generate( codeGlyph, 1,
			                                    cache->font, cache->wf, cache->hf,
			                                    cache->weight, cache->slant );
			if ( ret != CELL_OK ) {
				codeGlyph->code = 0;
				return (FontBitmapCharGlyph_t*)0;
			}
			return codeGlyph;
		}
	}
	cache->cacheN++;
	if ( cache->cacheN >= cache->count ) {
		cache->cacheN = 0;
	}
	codeGlyph = &cache->BitmapGlyph[ cache->cacheN ];
	
	//J キャッシュをつぶして使う
	FontBitmapGlyph_Delete( codeGlyph, 1 );

	codeGlyph->code = code;
	ret = FontBitmapCharGlyph_Generate( codeGlyph, 1, 
	                                    cache->font, cache->wf, cache->hf,
	                                    cache->weight, cache->slant );
	if ( ret != CELL_OK ) {
		codeGlyph->code = 0;
		return (FontBitmapCharGlyph_t*)0;
	}
	return codeGlyph;
}

//J 文字イメージキャッシュ破棄
void FontBitmapGlyphCache_End( FontBitmapCharGlyphCache_t* cache )
{
	if ( cache ) {
		FontBitmapGlyph_Delete( cache->BitmapGlyph, cache->count );
	}
}



//J 文字イメージフォントの初期化
FontBitmaps_t* FontBitmaps_Init( FontBitmaps_t* fontBitmaps,
                                 CellFont* cf, float wf, float hf, float weight, float slant,
                                 int cacheMax )
{
	int ret;
	if (! cf ) cf = &fontBitmaps->Font;

	if ( CELL_OK != (ret = cellFontSetScalePixel( cf, wf, hf )    )
	  || CELL_OK != (ret = cellFontSetEffectWeight( cf, weight )  )
	  || CELL_OK != (ret = cellFontSetEffectSlant( cf, slant )    )
	  || CELL_OK != (ret = cellFontGetHorizontalLayout( cf, &fontBitmaps->HorizontalLayout ) )
	  || CELL_OK != (ret = cellFontGetVerticalLayout( cf, &fontBitmaps->VerticalLayout )   )
	) {
		return (FontBitmaps_t*)0;
	}
	
	ret = FontBitmapGlyphAscii_Init( &fontBitmaps->Ascii, cf, wf, hf, weight, slant );
	if ( ret == CELL_OK ) {
		ret = FontBitmapGlyphCache_Init( &fontBitmaps->Cache, cacheMax, cf, wf, hf, weight, slant );
		if ( ret == CELL_OK ) {
			return fontBitmaps;
		}
		FontBitmapGlyphAscii_End( &fontBitmaps->Ascii );
	}
	return (FontBitmaps_t*)0;
}

//J 文字イメージフォントの横書きレイアウト取得
void FontBitmaps_GetHorizontalLayout( FontBitmaps_t*fontBitmaps, CellFontHorizontalLayout*layout )
{
	if (! fontBitmaps ) return;
	
	if ( layout ) {
		*layout = fontBitmaps->HorizontalLayout;
	}
}

//J 文字イメージフォントの縦書きレイアウト取得
void FontBitmaps_GetVerticalLayout( FontBitmaps_t*fontBitmaps, CellFontVerticalLayout*layout )
{
	if (! fontBitmaps ) return;
	
	if ( layout ) {
		*layout = fontBitmaps->VerticalLayout;
	}
}

//J 文字イメージフォントの文字イメージ取得
FontBitmapCharGlyph_t* FontBitmap_GetGlyph( FontBitmaps_t* fontBitmaps, uint32_t code )
{
	FontBitmapCharGlyph_t* codeGlyph;
	
	codeGlyph = FontBitmapGlyphAscii_GetGlyph( &fontBitmaps->Ascii, code );
	if (! codeGlyph ) {
		codeGlyph = FontBitmapGlyphCache_GetGlyph( &fontBitmaps->Cache, code );
		if (! codeGlyph ) {
			codeGlyph = FontBitmapGlyphAscii_GetGlyph( &fontBitmaps->Ascii, 0 );
		}
	}
	return codeGlyph;
}


//J 文字イメージフォントによる文字列レンダリング
int FontBitmaps_RenderPropText( FontBitmaps_t* fontBitmaps,
                               CellFontRenderSurface* surf, float x, float y,
                               uint8_t* utf8, float between )
{
	FontBitmapCharGlyph_t* codeGlyph;
	uint32_t code;
	
	if ( (!utf8) || *utf8 == 0x00 ) return x;
	
	//J 最初の文字取り出し
	utf8 += getUcs4( utf8, &code, 0x3000 );
	
	codeGlyph = FontBitmap_GetGlyph( fontBitmaps, code );
	
	//J 最初の文字の左合わせ
	x += -(codeGlyph->Metrics.Horizontal.bearingX);
	
	for (;;) {
		//J レンダリング
		if ( codeGlyph->Image ) {
			#if 1
			FontBitmapCharGlyph_Trans_blendCast_ARGB8( codeGlyph, surf, x, y );
			#else
			FontBitmapCharGlyph_Trans_blendColorCast_ARGB8( codeGlyph, surf, x, y, 255,255,255 );
			#endif
		}
		x += codeGlyph->Metrics.Horizontal.advance + between;

		//J 次の文字を取得
		utf8 += getUcs4( utf8, &code, 0x3000 );
		
		if ( code == 0x00000000 ) break;
		
		codeGlyph = FontBitmap_GetGlyph( fontBitmaps, code );
	}
	return x;
}

//J 文字イメージフォントの終了
void FontBitmaps_End( FontBitmaps_t* fontBitmaps )
{
	FontBitmapGlyphAscii_End( &fontBitmaps->Ascii );
	FontBitmapGlyphCache_End( &fontBitmaps->Cache );
}

