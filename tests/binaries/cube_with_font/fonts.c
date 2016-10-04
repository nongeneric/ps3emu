/*   SCE CONFIDENTIAL                                        */
/*PlayStation(R)3 Programmer Tool Runtime Library 400.001*/
/*   Copyright (C) 2009 Sony Computer Entertainment Inc.     */
/*   All Rights Reserved.                                    */

#include <stdio.h>
#include <stdlib.h>
#include <sys/paths.h>

#include <cell/font.h>
#include <cell/fontFT.h>

#include "fonts.h"

static Fonts_t Fonts;

static void* fonts_malloc( void*, uint32_t size );
static void  fonts_free( void*, void*p );
static void* fonts_realloc( void*, void* p, uint32_t size );
static void* fonts_calloc( void*, uint32_t numb, uint32_t blockSize );
static void* loadFile( uint8_t* fname, size_t *size, int offset, int addSize );

static void* fonts_malloc( void*obj, uint32_t size )
{
	(void)obj;
	return malloc( size );
}
static void  fonts_free( void*obj, void*p )
{
	(void)obj;
	free( p );
}
static void* fonts_realloc( void*obj, void* p, uint32_t size )
{
	(void)obj;
	return realloc( p, size );
}
static void* fonts_calloc( void*obj, uint32_t numb, uint32_t blockSize )
{
	(void)obj;
	return calloc( numb, blockSize );
}

static void* loadFile( uint8_t* fname, size_t *size, int offset, int addSize )
{
	FILE* fp;
	size_t file_size;
	void* p;

	fp = fopen( (const char*)fname, "rb" );
	if (! fp ) {
		printf("cannot open %s\n", fname );
		if ( size ) *size = 0;
		return NULL;
	}

	fseek( fp, 0, SEEK_END );
	file_size = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	if ( size ) *size = file_size;
	
	p = malloc( file_size + offset + addSize );
	if ( p ) {
		fread( (unsigned char*)p+offset, file_size, 1, fp );
	}

	fclose( fp );

	return p;
}


//J libfontライブラリのロード
int Fonts_LoadModules()
{
	int ret;
	
	//J libfont モジュールのロード
	ret = cellSysmoduleLoadModule( CELL_SYSMODULE_FONT );
	if ( ret == CELL_OK ) {
		//J libfreetype モジュールのロード
		ret = cellSysmoduleLoadModule( CELL_SYSMODULE_FREETYPE );
		if ( ret == CELL_OK ) {
			//J libfontFT モジュールのロード
			ret = cellSysmoduleLoadModule( CELL_SYSMODULE_FONTFT );
			if ( ret == CELL_OK ) {
				return ret;
			}
			else printf("Fonts: 'CELL_SYSMODULE_FONTFT' NG! %08x\n",ret);
			//J 以下、ロードエラー時のアンロード処理
			cellSysmoduleUnloadModule( CELL_SYSMODULE_FREETYPE );
		}
		else printf("Fonts: 'CELL_SYSMODULE_FREETYPE' NG! %08x\n",ret);
		
		cellSysmoduleUnloadModule( CELL_SYSMODULE_FONT );
	}
	else printf("Fonts: 'CELL_SYSMODULE_FONT' NG! %08x\n",ret);
	
	return ret;
}

//J libfontライブラリのアンロード
void Fonts_UnloadModules()
{
	cellSysmoduleUnloadModule( CELL_SYSMODULE_FONTFT );
	cellSysmoduleUnloadModule( CELL_SYSMODULE_FREETYPE );
	cellSysmoduleUnloadModule( CELL_SYSMODULE_FONT );
}

//J libfont初期化
Fonts_t* Fonts_Init()
{
	//J libfontに必要なリソース
	static CellFontEntry UserFontEntrys[USER_FONT_MAX];
	static uint32_t FontFileCache[FONT_FILE_CACHE_SIZE/sizeof(uint32_t)];

	CellFontConfig config;
	int ret;

	//J libfont 初期化パラメータ設定。
	//----------------------------------------------------------------------
	CellFontConfig_initialize( &config );

	//J ファイルキャッシュバッファを設定
	config.FileCache.buffer = FontFileCache;
	config.FileCache.size   = FONT_FILE_CACHE_SIZE;

	//J ユーザーフォント用、エントリバッファ設定。
	config.userFontEntrys   = UserFontEntrys;
	config.userFontEntryMax = sizeof(UserFontEntrys)/sizeof(CellFontEntry);

	//J フラグは現在 0 固定
	config.flags = 0;

	//J libfontライブラリ 初期化
	//----------------------------------------------------------------------
	ret = cellFontInit( &config );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "   Fonts:cellFontInit=", ret );
		return (Fonts_t*)0;
	}
	
	return &Fonts;
}


//J フォントインターフェースライブラリ初期化 ( FreeType2を使用します。)
int Fonts_InitLibraryFreeType( const CellFontLibrary** lib )
{
	CellFontLibraryConfigFT config;
	const CellFontLibrary*  fontLib;
	int ret;
	
	//J フォントインターフェースライブラリ初期化パラメータの初期化
	CellFontLibraryConfigFT_initialize( &config );
	//J メモリーインターフェースの設定( 設定必須 )
	config.MemoryIF.Object  = NULL;
	config.MemoryIF.Malloc  = fonts_malloc;
	config.MemoryIF.Free    = fonts_free;
	config.MemoryIF.Realloc = fonts_realloc;
	config.MemoryIF.Calloc  = fonts_calloc;

	//J フォントインターフェースライブラリ初期化
	ret = cellFontInitLibraryFreeType( &config, &fontLib );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "   Fonts:cellFontInitLibrary_FreeType=", ret );
	}
	if ( lib ) *lib = fontLib;
	
	return ret;
}

//J レンダラー初期化。
int Fonts_CreateRenderer( const CellFontLibrary* lib, uint32_t initSize, CellFontRenderer* rend )
{
	CellFontRendererConfig config;
	int ret;
	
	CellFontRendererConfig_initialize( &config );
	CellFontRendererConfig_setAllocateBuffer( &config, initSize, 0 );
	
	//J レンダラの作成
	ret = cellFontCreateRenderer( lib, &config, rend );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "   Fonts:cellFontCreateRenderer=", ret );
		return ret;
	}

	return ret;
}

//J フォントのオープン
int Fonts_OpenFonts( const CellFontLibrary*lib, Fonts_t* fonts )
{
	int n;
	int ret=CELL_OK;
	
	//J システム搭載フォントセットのオープン例
	{
		static struct {
			uint32_t isMemory;
			int      fontsetType;
		} openSystemFont[ SYSTEM_FONT_MAX ] = {
			{ 0, CELL_FONT_TYPE_DEFAULT_GOTHIC_LATIN_SET },
			{ 0, CELL_FONT_TYPE_DEFAULT_GOTHIC_JP_SET    },
			{ 1, CELL_FONT_TYPE_DEFAULT_SANS_SERIF       },
			{ 1, CELL_FONT_TYPE_DEFAULT_SERIF            },
		};
		CellFontType type;
		
		fonts->sysFontMax = 4;
		//J オンメモリでオープンする
		for ( n=0; n < fonts->sysFontMax; n++ ) {
			if (! openSystemFont[n].isMemory ) continue;
			
			type.type = openSystemFont[n].fontsetType;
			type.map  = CELL_FONT_MAP_UNICODE;
			//J システム搭載フォントセットオープン
			ret = cellFontOpenFontsetOnMemory( lib, &type, &fonts->SystemFont[n] );
			if ( ret != CELL_OK ) {
				Fonts_PrintError( "   Fonts:cellFontOpenFontset=", ret );
				Fonts_CloseFonts( fonts ); //J オープンした分、クローズして戻る。
				return ret;
			}
			fonts->openState |= (1<<n);
			//J 初期スケール設定
			#if 1
			//J ポイントで指定
			cellFontSetResolutionDpi( &fonts->SystemFont[n], 72, 72 );
			cellFontSetScalePoint( &fonts->SystemFont[n], 26.f, 26.f );
			#else
			//J ピクセルで指定
			cellFontSetScalePixel( &fonts->SystemFont[n], 26.f, 26.f );
			#endif
		}
		//J ファイルアクセスオープンする
		for ( n=0; n < fonts->sysFontMax; n++ ) {
			if ( openSystemFont[n].isMemory ) continue;
			
			type.type = openSystemFont[n].fontsetType;
			type.map  = CELL_FONT_MAP_UNICODE;
			//J システム搭載フォントセットオープン
			ret = cellFontOpenFontset( lib, &type, &fonts->SystemFont[n] );
			if ( ret != CELL_OK ) {
				Fonts_PrintError( "   Fonts:cellFontOpenFontset=", ret );
				Fonts_CloseFonts( fonts ); //J オープンした分、クローズして戻る。
				return ret;
			}
			fonts->openState |= (1<<n);
			//J 初期スケール設定
			#if 1
			//J ポイントで指定
			cellFontSetResolutionDpi( &fonts->SystemFont[n], 72, 72 );
			cellFontSetScalePoint( &fonts->SystemFont[n], 26.f, 26.f );
			#else
			//J ピクセルで指定
			cellFontSetScalePixel( &fonts->SystemFont[n], 26.f, 26.f );
			#endif
		}
	}
	
	//J アプリケーションで用意したフォントのオープン例
	{
		static struct {
			uint32_t isMemory;
			const char* filePath;
		} userFont[ USER_FONT_MAX ] = {
			{ 0, SYS_APP_HOME"/app.ttf" }, //J app.ttf は、用意されていません。
		};
		uint32_t fontUniqueId = 0;
		
		fonts->userFontMax = 0;            //J app.ttf は、用意されていません。
		
		for ( n=0; n < fonts->userFontMax; n++ ) {
			uint8_t* path = (uint8_t*)userFont[n].filePath;
			
			if (! userFont[n].isMemory ) {
			  //J ファイルアクセスでオープン
				ret = cellFontOpenFontFile( lib, path, 0, fontUniqueId, &fonts->UserFont[n] );
				if ( ret != CELL_OK ) {
					Fonts_PrintError( "    Fonts:cellFontOpenFile=", ret );
					Fonts_CloseFonts( fonts );
					printf("    Fonts:   [%s]\n", userFont[n].filePath);
					return ret;
				}
			}
			else {
			  //J メモリーアクセスでオープン
				size_t size;
				void *p = loadFile( path, &size, 0, 0 );
				
				ret = cellFontOpenFontMemory( lib, p, size, 0, fontUniqueId, &fonts->UserFont[n] );
				if ( ret != CELL_OK ) {
					if (p) free( p );
					Fonts_PrintError( "    Fonts:cellFontMemory=", ret );
					Fonts_CloseFonts( fonts );
					printf("    Fonts:   [%s]\n", userFont[n].filePath);
					return ret;
				}
			}
			fonts->openState |= (1<<(FONT_USER_FONT0+n));
			fontUniqueId++;
			
			if ( ret == CELL_OK ) {
			  //J 初期スケール設定
				#if 1
				//J ポイントで指定
				cellFontSetResolutionDpi( &fonts->UserFont[n], 72, 72 );
				cellFontSetScalePoint( &fonts->UserFont[n], 26.f, 26.f );
				#else
				//J ピクセルで指定
				cellFontSetScalePixel( &fonts->UserFont[n], 26.f, 26.f );
				#endif
			}
		}
	}
	
	return ret;
}

//J 行の高さとベースライン位置の取得。
int Fonts_GetFontsHorizontalLayout( Fonts_t* fonts, uint32_t fontmask, float scale, float* lineHeight, float*baseLineY )
{
	float ascent  = 0.0f;
	float descent = 0.0f;
	int ret = CELL_OK;

	if ( fonts ) {
		CellFont* cf;
		CellFontHorizontalLayout Layout;
		int n;
		
		//J システムフォントのレイアウトを調べる
		for ( n=0; n < fonts->sysFontMax; n++ ) {
			if ( (fontmask & (1<<n))==0x00000000 ) continue;
			
			cf = &fonts->SystemFont[n];
			
			ret = cellFontSetScalePixel( cf, scale, scale );
			if ( ret != CELL_OK ) {
				Fonts_PrintError( "    Fonts.SystemFont:cellFontSetScalePixel=", ret );
			}
			
			ret = cellFontGetHorizontalLayout( cf, &Layout );
			if ( ret != CELL_OK ) {
				Fonts_PrintError( "    Fonts.SystemFont:cellFontGetHorizontalLayout=", ret );
			}
			
			if ( Layout.baseLineY > ascent ) {
				ascent = Layout.baseLineY;
			}
			if ( Layout.lineHeight - Layout.baseLineY > descent ) {
				descent = Layout.lineHeight - Layout.baseLineY;
			}
		}
		//J ユーザーフォントのレイアウトを調べる
		for ( n=0; n < fonts->userFontMax; n++ ) {
			if ( (fontmask & (1<<(FONT_USER_FONT0+n)))==0x00000000 ) continue;
			
			cf = &fonts->UserFont[n];
			
			ret = cellFontSetScalePixel( cf, scale, scale );
			if ( ret != CELL_OK ) {
				Fonts_PrintError( "    Fonts.SystemFont:cellFontSetScalePixel=", ret );
			}
			
			ret = cellFontGetHorizontalLayout( cf, &Layout );
			if ( ret != CELL_OK ) {
				Fonts_PrintError( "    Fonts.UserFont:cellFontGetHorizontalLayout=", ret );
			}
			
			if ( Layout.baseLineY > ascent ) {
				ascent = Layout.baseLineY;
			}
			if ( Layout.lineHeight - Layout.baseLineY > descent ) {
				descent = Layout.lineHeight - Layout.baseLineY;
			}
		}
	}
	if ( lineHeight ) *lineHeight = ascent + descent;
	if ( baseLineY  ) *baseLineY  = ascent;

	return CELL_OK;
}

//J 指定したフォント取得。
int Fonts_AttachFont( Fonts_t* fonts, int fontEnum, CellFont*cf )
{
	CellFont* openedFont = (CellFont*)0;
	int ret;
	
	if ( fonts ) {
		if ( fontEnum < FONT_USER_FONT0 ) {
			uint32_t n = fontEnum;
			
			if ( n < (uint32_t)fonts->sysFontMax ) {
				if ( fonts->openState & (1<<fontEnum) ) {
					openedFont = &fonts->SystemFont[ n ];
				}
			}
		}
		else {
			uint32_t n = fontEnum - FONT_USER_FONT0;
			
			if ( n < (uint32_t)fonts->userFontMax ) {
				if ( fonts->openState & (1<<fontEnum) ) {
					openedFont = &fonts->UserFont[ n ];
				}
			}
		}
	}
	//J すでにオープン済みのフォントのインスタンスを生成。
	ret = cellFontOpenFontInstance( openedFont, cf );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:AttachFont:cellFontOpenFontInstance=", ret );
	}
	return ret;
}

//J フォントのスケール設定(ピクセル指定。縦横比１：１)
int Fonts_SetFontScale( CellFont* cf, float scale )
{
	int ret;
	
	ret = cellFontSetScalePixel( cf, scale, scale );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:cellFontSetScalePixel=", ret );
	}
	return ret;
}

//J フォントの太さの調整値設定
int Fonts_SetFontEffectWeight( CellFont* cf, float effWeight )
{
	int ret;
	
	ret = cellFontSetEffectWeight( cf, effWeight );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:cellFontSetEffectWeight=", ret );
	}
	return ret;
}

//J フォントに与えるを傾きを設定
int Fonts_SetFontEffectSlant( CellFont* cf, float effSlant )
{
	int ret;
	
	ret = cellFontSetEffectSlant( cf, effSlant );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:cellFontSetEffectSlant=", ret );
	}
	return ret;
}

//J フォントの横書きレイアウトを取得
int Fonts_GetFontHorizontalLayout( CellFont* cf, float* lineHeight, float*baseLineY )
{
	CellFontHorizontalLayout Layout;
	int ret;
	
	ret = cellFontGetHorizontalLayout( cf, &Layout );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:cellFontGetHorizontalLayout=", ret );
		return ret;
	}
	if ( lineHeight ) *lineHeight = Layout.lineHeight;
	if ( baseLineY  ) *baseLineY  = Layout.baseLineY;
	
	return ret;
}

//J フォントの縦書きレイアウトを取得
int Fonts_GetFontVerticalLayout( CellFont* cf, float* lineWidth, float*baseLineX )
{
	CellFontVerticalLayout Layout;
	int ret;
	
	ret = cellFontGetVerticalLayout( cf, &Layout );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:cellFontGetVerticalLayout=", ret );
		return ret;
	}
	if ( lineWidth ) *lineWidth = Layout.lineWidth;
	if ( baseLineX  ) *baseLineX  = Layout.baseLineX;
	
	return ret;
}

//J CellFontGlyph作成に必要なバッファサイズの取得
int Fonts_GenerateCharGlyph( CellFont* cf, uint32_t code, CellFontGlyph** glyph )
{
	int ret;
	
	ret = cellFontGenerateCharGlyph( cf, code, glyph );
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:cellFontGenerateGlyph=", ret );
	}
	return ret;
}

//J CellFontGlyphの破棄
int Fonts_DeleteGlyph( CellFont* cf, CellFontGlyph* glyph )
{
	int ret;
	
	ret = cellFontDeleteGlyph( cf, glyph );
	
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:cellFontDelete=", ret );
	}
	return ret;
}

//J 使用し終わったフォントの返却
int Fonts_DetachFont( CellFont*cf )
{
	int ret = cellFontCloseFont( cf );
	
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:cellFontCloseFont=", ret );
	}
	return ret;
}

//J フォントのクローズ
int Fonts_CloseFonts( Fonts_t* fonts )
{
	int n;
	int ret, err = CELL_FONT_OK;

	if (! fonts ) return err;

	//J システム搭載フォントセットのクローズ
	for ( n=0; n < fonts->sysFontMax; n++ ) {
		uint32_t checkBit = (1<<n);
		
		if ( fonts->openState & checkBit ) {
			ret = cellFontCloseFont( &fonts->SystemFont[n] );
			if ( ret != CELL_OK ) {
				Fonts_PrintError( "    Fonts.SystemFont:cellFontCloseFont=", ret );
				err = ret;
				continue;
			}
			fonts->openState &= (~checkBit);
		}
	}
	//J ユーザーフォントのクローズ
	for ( n=0; n < fonts->userFontMax; n++ ) {
		uint32_t checkBit = (1<<(FONT_USER_FONT0+n));
		
		if ( fonts->openState & checkBit ) {
			ret = cellFontCloseFont( &fonts->UserFont[n] );
			if ( ret != CELL_OK ) {
				Fonts_PrintError( "    Fonts.UserFont:cellFontCloseFont=", ret );
				err = ret;
				continue;
			}
			fonts->openState &= (~checkBit);
		}
	}

	return err;
}

//J レンダラを破棄
int Fonts_DestroyRenderer( CellFontRenderer* renderer )
{
	int ret = cellFontDestroyRenderer( renderer );

	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:cellFontDestroyRenderer=", ret );
	}
	return ret;
}

//J フォントインターフェースライブラリ 終了
int Fonts_EndLibrary( const CellFontLibrary* lib )
{
	int ret = cellFontEndLibrary( lib );
	
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:cellFontEndLibrary=", ret );
	}
	return ret;
}

//J libfont ライブラリ終了
int Fonts_End()
{
	int ret = cellFontEnd();
	
	if ( ret != CELL_OK ) {
		Fonts_PrintError( "    Fonts:cellFontEnd=", ret );
	}
	return ret;
}

//J libfont のエラーをマクロ名でprintf出力
void Fonts_PrintError( const char*mess, int d )
{
	const char* s;
	switch( d ) {
	case CELL_FONT_OK                              : s="CELL_FONT_OK";                               break;
	case CELL_FONT_ERROR_FATAL                     : s="CELL_FONT_ERROR_FATAL";                      break;
	case CELL_FONT_ERROR_INVALID_PARAMETER         : s="CELL_FONT_ERROR_INVALID_PARAMETER";          break;
	case CELL_FONT_ERROR_UNINITIALIZED             : s="CELL_FONT_ERROR_UNINITIALIZED";              break;
	case CELL_FONT_ERROR_INITIALIZE_FAILED         : s="CELL_FONT_ERROR_INITIALIZE_FAILED";          break;
	case CELL_FONT_ERROR_INVALID_CACHE_BUFFER      : s="CELL_FONT_ERROR_INVALID_CACHE_BUFFER";       break;
	case CELL_FONT_ERROR_ALREADY_INITIALIZED       : s="CELL_FONT_ERROR_ALREADY_INITIALIZED";        break;
	case CELL_FONT_ERROR_ALLOCATION_FAILED         : s="CELL_FONT_ERROR_ALLOCATION_FAILED";          break;
	case CELL_FONT_ERROR_NO_SUPPORT_FONTSET        : s="CELL_FONT_ERROR_NO_SUPPORT_FONTSET";         break;
	case CELL_FONT_ERROR_OPEN_FAILED               : s="CELL_FONT_ERROR_OPEN_FAILED";                break;
	case CELL_FONT_ERROR_READ_FAILED               : s="CELL_FONT_ERROR_READ_FAILED";                break;
	case CELL_FONT_ERROR_FONT_OPEN_FAILED          : s="CELL_FONT_ERROR_FONT_OPEN_FAILED";           break;
	case CELL_FONT_ERROR_FONT_NOT_FOUND            : s="CELL_FONT_ERROR_FONT_NOT_FOUND";             break;
	case CELL_FONT_ERROR_FONT_OPEN_MAX             : s="CELL_FONT_ERROR_FONT_OPEN_MAX";              break;
	case CELL_FONT_ERROR_FONT_CLOSE_FAILED         : s="CELL_FONT_ERROR_FONT_CLOSE_FAILED";          break;
	case CELL_FONT_ERROR_NO_SUPPORT_FUNCTION       : s="CELL_FONT_ERROR_NO_SUPPORT_FUNCTION";        break;
	case CELL_FONT_ERROR_NO_SUPPORT_CODE           : s="CELL_FONT_ERROR_NO_SUPPORT_CODE";            break;
	case CELL_FONT_ERROR_NO_SUPPORT_GLYPH          : s="CELL_FONT_ERROR_NO_SUPPORT_GLYPH";           break;
	case CELL_FONT_ERROR_RENDERER_ALREADY_BIND     : s="CELL_FONT_ERROR_RENDERER_ALREADY_BIND";      break;
	case CELL_FONT_ERROR_RENDERER_UNBIND           : s="CELL_FONT_ERROR_RENDERER_UNBIND";            break;
	case CELL_FONT_ERROR_RENDERER_INVALID          : s="CELL_FONT_ERROR_RENDERER_INVALID";           break;
	case CELL_FONT_ERROR_RENDERER_ALLOCATION_FAILED: s="CELL_FONT_ERROR_RENDERER_ALLOCATION_FAILED"; break;
	case CELL_FONT_ERROR_ENOUGH_RENDERING_BUFFER   : s="CELL_FONT_ERROR_ENOUGH_RENDERING_BUFFER";    break;
	case CELL_FONT_ERROR_BUFFER_SIZE_NOT_ENOUGH    : s="CELL_FONT_ERROR_BUFFER_SIZE_NOT_ENOUGH";     break;
	case CELL_FONT_ERROR_NO_SUPPORT_SURFACE        : s="CELL_FONT_ERROR_NO_SUPPORT_SURFACE";         break;
	
	default:s="unknown!";
	}
	if (!mess) mess="";
	printf("%s%s\n",mess,s);
}

uint32_t Fonts_getUcs4FromUtf8( uint8_t*utf8, uint32_t*ucs4, uint32_t alterCode )
{
	uint64_t code = 0L;
	uint32_t len = 0;

	code = (uint64_t)*utf8;

	if ( code ) {
		utf8++;
		len++;
		if ( code >= 0x80 ) {
			while (1) {
				//J UTF-8 先頭コードチェック。
				if ( code & 0x40 ) { 
					uint64_t mask = 0x20L;
					uint64_t encode;
					uint64_t n;
					
					for ( n=2;;n++ ) {
						if ( (code & mask) == 0 ) {
							len = n;
							mask--;
							if ( mask == 0 ) { // 0xFE or 0xFF 
								//J 先頭コードエラー
								*ucs4 = 0x00000000;
								return 0;
							}
							break;
						}
						mask = (mask >> 1);
					}
					code &= mask;
					
					for ( n=1; n<len; n++ ) {
						encode = (uint64_t)*utf8;
						if ( (encode & 0xc0) != 0x80 ) {
							//J 文字コードが途中で切れている！
							if ( ucs4 ) *ucs4 = alterCode;
							return n;
						}
						code = ( ( code << 6 ) | (encode & 0x3f) );
						utf8++;
					}
					break;
				}
				else { //J 先頭コードエラー
					//J UTF-8の文字列であるならば文字コードの途中と判断。
					//J 次の文字までスキップ。
					for( ;; utf8++ ) {
						code = (uint64_t)*utf8;
						if ( code < 0x80 ) break;
						if ( code & 0x40 ) break;
						len++;
					}
					if ( code < 0x80 ) break;
				}
			}
		}
	}
	if ( ucs4 )  *ucs4 = (uint32_t)code;
	
	return len;
}
