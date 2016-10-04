/*   SCE CONFIDENTIAL                                        */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2009 Sony Computer Entertainment Inc.     */
/*   All Rights Reserved.                                    */

#include <stdio.h>
#include <assert.h>
#include <sys/timer.h>
#include <cell/gcm.h>

#include "SampleApp.h"
#include "FWCellGCMWindow.h"
#include "FWDebugFont.h"
#include "FWTime.h"
#include "cellutil.h"
#include "gcmutil.h"
#include "stdio.h"

#define PI  3.14159265358979 
#define PI2 ((PI)+(PI))

using namespace cell::Gcm;

// instantiate the class
SampleApp app;

// shader
extern struct _CGprogram _binary_vpshader_vpo_start;
extern struct _CGprogram _binary_fpshader_fpo_start;

#define LIBFONT_SAMPLE
#ifdef  LIBFONT_SAMPLE
#define LIBFONTGCM_SAMPLE  //J cellFontGraphicsXX を使用した処理

#include <sys/memory.h> // for sys_memory_allocate()
#include <cell/font.h>
#include <cell/fontFT.h>
#include <cell/fontGcm.h>
#include "fonts.h"
#include "fonts_graphics.h"
#include "fonts_bitmap.h"

typedef struct SampleRenderTarget {
	float scaleX, scaleY;
	CellFontRenderer      Renderer;  //J フォントレンダラーインターフェース構造体
	CellFontRenderSurface Surface;   //J レンダリングサーフェス情報構造体
}SampleRenderWork;

static const CellFontLibrary* freeType; //J フォントインターフェースライブラリ
static int sampleType;
static int sampleTimer=0;
static int sampleTime=5*60;

#ifdef  LIBFONTGCM_SAMPLE
//J libfontGcm 追加使用サンプル
static void*    fontGraphicsMemory;
static int      fontGraphicsMemorySize;
static void*    fontMappedMainMemory;
static uint32_t fontMappedMainMemorySize;

//J libfontGcm オブジェクト
static const CellFontGraphics*     fontGraphicsGcm;
static CellFontGraphicsDrawContext FontDrawContext;
static CellFontGraphicsDrawContext* fontDC = (CellFontGraphicsDrawContext*)0;

//J libfontGcm 描画サーフェス
CellFontRenderSurfaceGcm RenderSurfGcm;

//J 頂点グリフ保持バッファ
static uint32_t*fontVertexBuffer;
static uint32_t fontVertexBufferSize;

//J 頂点グリフを取り扱うデータ型サンプル
#define FONT_VERTEX_GLYPH_MAX (64)
typedef struct {
	
	struct {
		float scaleX, scaleY;
		float baseLineY, lineHeight;
	} Layout;
	float glyphScaleMax;
	
	int count;
	FontVertexGlyph_t Array[FONT_VERTEX_GLYPH_MAX];
	
} FontVertexesGlyphSample;

FontVertexesGlyphSample VertexesGlyphs;

#endif //LIBFONTGCM_SAMPLE

static Fonts_t* fonts;                  //J サンプルのフォント管理構造体
static FontBitmaps_t FontBitmaps;       //J サンプルのビットマップベースのフォント構造体

static SampleRenderWork RenderWork;     //J このサンプルで利用する
#endif //LIBFONT_SAMPLE
//-----------------------------------------------------------------------------
// Description: Constructor
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
SampleApp::SampleApp()
	: mTextureWidth(256), mTextureHeight(256), mTextureDepth(4),
	  mLabel(NULL), mLabelValue(0)
{
	// 
}

//-----------------------------------------------------------------------------
// Description: Destructor
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
SampleApp::~SampleApp()
{
	//
}

void SampleApp::initShader()
{
	mCGVertexProgram   = &_binary_vpshader_vpo_start;
	mCGFragmentProgram = &_binary_fpshader_fpo_start;

	// init
	cellGcmCgInitProgram(mCGVertexProgram);
	cellGcmCgInitProgram(mCGFragmentProgram);

	// allocate video memory for fragment program
	unsigned int ucodeSize;
	void *ucode;

	cellGcmCgGetUCode(mCGFragmentProgram, &ucode, &ucodeSize);

	mFragmentProgramUCode
		= (void*)cellGcmUtilAllocateLocalMemory(ucodeSize, 64);
	cellGcmAddressToOffset(mFragmentProgramUCode, &mFragmentProgramOffset);

	memcpy(mFragmentProgramUCode, ucode, ucodeSize); 

	// get and copy 
	cellGcmCgGetUCode(mCGVertexProgram, &ucode, &ucodeSize);
	mVertexProgramUCode = ucode;
}

static void generateTexture(uint8_t *texture, uint32_t *buffer,
							float *cosTable, uint32_t* palette,
							uint32_t width, uint32_t height)
{
	static uint32_t t0 = 0;
	static uint32_t t1 = 10;
	static uint32_t t2 = 900;
	static uint32_t t3 = 400;

	for (uint32_t i = 0; i < height; i++) {
		for (uint32_t j = 0; j < width; j++) {
			float c0 = cosTable[t0&0x3FF];
			float c1 = cosTable[t1&0x3FF];
			float c2 = cosTable[t2&0x3FF];
			float c3 = cosTable[t3&0x3FF];

			uint32_t Index = (uint32_t)(40.0f*(c0 + c0*c1 + c0*c3 + c2));
			buffer[i*width + j] = palette[Index&0xFF]; 
			t0 += 1;
			t1 += 3;
		}
		t2 += 3;
		t3 += 2;
	}
	t0 += 3;
	t1 += 2;
	t2 += 1;
	t3 += 3;
	#ifdef  LIBFONT_SAMPLE
	{
		//J レンダリングはメインメモリで行う
		static int sampleCounter=0;
		CellFontRenderer* renderer;
		CellFontRenderSurface* surf;
		CellFont Font[1];
		CellFont* cf;
		int fn;
		int ret;
		
		//J テクスチャバッファをフォントのレンダリング用サーフェス情報に設定 
		//------------------------------------------------------------------
		surf     = &RenderWork.Surface;
		cellFontRenderSurfaceInit( surf, 
		                           buffer, width*4, 4,
		                           width, height );
		//J レンダリングサーフェスに、シザリング矩形を設定。(デフォルトの
		//J シザリング矩形はサーフェスサイズと同じで以下の設定と等価。)
		cellFontRenderSurfaceSetScissor( surf, 0, 0, width, height );
		
		//J レンダラ選択
		//------------------------------------------------------------------
		renderer = &RenderWork.Renderer;
		
		//J フォントセット選択
		//------------------------------------------------------------------
		fn = FONT_SYSTEM_GOTHIC_LATIN; //J システムフォントセット
		//fn = FONT_SYSTEM_GOTHIC_JP;  //J システムフォントセット
		//fn = FONT_SYSTEM_SANS_SERIF; //J システムフォントセット
		//fn = FONT_SYSTEM_SERIF;      //J システムフォントセット
		//fn = FONT_USER_FONT0;        //J 独自に用意したフォントを使用する場合
		
		ret = Fonts_AttachFont( fonts, fn, &Font[0] );
		if ( ret == CELL_OK ) cf = &Font[0];
		else                  cf = (CellFont*)0;

		if ( cf ) {
			static float textScale = 1.0f;
			static float weight = 1.00f;
			static float slant = 0.0f;
			float surfW = (float)width;  //サーフェス幅
			float surfH = (float)height; //サーフェス高さ
			float textX, textW = surfW;
			float textY, textH = surfW;
			float scale;
			float step;
			float lineH, baseY;
			uint8_t* utf8Str0 = (uint8_t*)"Libfontライブラリ";
			uint8_t* utf8Str1 = (uint8_t*)"Cube with font sample";
			float x, y;
			float w1,w2;
			float w;
			
			sampleType = ((sampleCounter/sampleTime)%6);
			sampleTimer = (sampleCounter%sampleTime);
			
			if ( fn == FONT_SYSTEM_SANS_SERIF || fn == FONT_SYSTEM_SERIF ) {
				utf8Str0 = (uint8_t*)"Libfont Library";
			}
			else if ( sampleType ) {
				utf8Str0 = (uint8_t*)"サンプル‐2 …「縦書き。」";
			}
			//J 文字スケールと文字間の指定。
			//------------------------------
			step  =  0.f; //J 文字間
			scale = 48.f; //J ピクセル 
			{
				static float d = 0.002f;
				if ( textScale >= 1.00f || textScale <= 0.75f ) d = -d;
				textX = 0.5f * ( surfW - surfW * textScale );
				textY = 0.5f * ( surfH - surfH * textScale );
				textW = surfW * textScale;
				textH = surfH * textScale;
				textScale += d;
			}
			//J フォントのスケール設定
			ret = Fonts_SetFontScale( cf, scale );
			if ( ret == CELL_OK ) {
				//J フォントの太さ調整。
				static float d = 0.0005f;
				weight += d;
				if ( weight >= 1.04f || weight <= 0.93f ) d = -d;
				ret = Fonts_SetFontEffectWeight( cf, weight );
			}
			if ( ret == CELL_OK ) {
				//J フォントに傾きを設定
				static float d = 0.01f;
				slant += d;
				if ( slant >= 1.0f || slant <= -1.0f ) d = -d;
				ret = Fonts_SetFontEffectSlant( cf, slant );
			}
			
			if ( sampleType == 0 && ret == CELL_OK ) {
				//J 指定したフォントセット、スケール、ウェイトにおいての
				//J ベースライン位置と行高さの取得。
				ret = Fonts_GetFontHorizontalLayout( cf, &lineH, &baseY );
				
				if ( ret == CELL_OK ) {
					//J 文字列のレンダリング幅の算出
					//--------------------------------
					w1 = Fonts_GetPropTextWidth( cf, utf8Str0, scale, scale, slant, step, NULL, NULL );
					w2 = Fonts_GetPropTextWidth( cf, utf8Str1, scale, scale, slant, step, NULL, NULL );
				
					//J 描画範囲をチェック
					w = (( w1 > w2 )? w1:w2);
					//J 描画開始座標の小数部に文字列幅を足して小数部を切り上げた値が実際に
					//J 必要なピクセル幅なので１ピクセル差し引いて判断。(センター合わせ用)
					textW-=1.0f;
					if ( w > textW ) {
						float ratio;
						//J 範囲内に縮小して収める。
						scale = Fonts_GetPropTextWidthRescale( scale, w, textW, &ratio );
						w1    *= ratio;
						w2    *= ratio;
						baseY *= ratio;
						lineH *= ratio;
						step  *= ratio;
					}
					
					//J レンダラ接続
					Fonts_BindRenderer( cf, renderer );
					
					//J １行目レンダリング(センター合わせ)
					x = textX + 0.5f*(textW-w1);
					y = height/2 - lineH;
					Fonts_RenderPropText( cf, surf, x, y, utf8Str0, scale, scale, slant, step );
				
					//J ２行目レンダリング(センター合わせ)
					x = textX + 0.5f*(textW-w2);
					y = height/2;
					Fonts_RenderPropText( cf, surf, x, y, utf8Str1, scale, scale, slant, step );
				
					//J レンダラー接続解除
					Fonts_UnbindRenderer( cf );
				}
			}
			else 
			if ( sampleType == 1 && ret == CELL_OK ) {
				float lineW, baseX;
				float h;

				//J 指定したフォントセット、スケール、ウェイトにおいての
				//J 縦書きレイアウト情報の取得
				Fonts_SetFontEffectSlant( cf, 0.0f );
				
				ret = Fonts_GetFontVerticalLayout( cf, &lineW, &baseX );
				
				Fonts_SetFontEffectSlant( cf, slant );
				if ( ret == CELL_OK ) {
					h = Fonts_GetVerticalTextHeight( cf, utf8Str0, scale, scale, step, NULL, NULL );
					textH-=1.0f;
					if ( h > textH ) {
						float ratio;
						scale = Fonts_GetVerticalTextHeightRescale( scale, h, textH, &ratio );
						baseX *= ratio;
						lineW *= ratio;
						step  *= ratio;
					}
					
					//J レンダラ接続
					Fonts_BindRenderer( cf, renderer );
					
					x = 0.5f * surfW - 0.5f * lineW;
					y = textY + 0.0f;
					//J 縦書き
					Fonts_RenderVerticalText( cf, surf, x, y, utf8Str0, scale, scale, (slant<0.0f)?0.0f:slant, step );
					x += lineW;
					Fonts_RenderVerticalText( cf, surf, x, y, utf8Str0, scale, scale, slant, step );
					//J レンダラー接続解除
					Fonts_UnbindRenderer( cf );
				}
			}
			else 
			if ( sampleType == 2 && ret == CELL_OK ) {
				//J レンダリング済みフォントイメージを使用したテクスチャ生成例
				CellFontHorizontalLayout layout;
				char str[256];
				
				FontBitmaps_GetHorizontalLayout( &FontBitmaps, &layout );
				lineH = layout.lineHeight;
				
				//generate textrue
				x = 0.0f;
				y = 0.0f;
				FontBitmaps_RenderPropText( &FontBitmaps, surf, x, y, (uint8_t*)"Cached font bitmaps", 0.0f );
				
				x = 4.0f;
				
				y += lineH;
				sprintf( str, "ビットマップフォント\n" );
				FontBitmaps_RenderPropText( &FontBitmaps, surf, x, y, (uint8_t*)str, step );
				y += lineH;
				sprintf( str, "sampleCounter = %d\n", sampleCounter );
				FontBitmaps_RenderPropText( &FontBitmaps, surf, x, y, (uint8_t*)str, step );
				y += lineH;
				sprintf( str, "sampleCounter = %d\n", sampleCounter+1 );
				FontBitmaps_RenderPropText( &FontBitmaps, surf, x, y, (uint8_t*)str, step );
				y += lineH;
				sprintf( str, "sampleCounter = %d\n", sampleCounter+2 );
				FontBitmaps_RenderPropText( &FontBitmaps, surf, x, y, (uint8_t*)str, step );
				y += lineH;
				sprintf( str, "sampleCounter = %d\n", sampleCounter+3 );
				FontBitmaps_RenderPropText( &FontBitmaps, surf, x, y, (uint8_t*)str, step );
				y += lineH;
				sprintf( str, "sampleCounter = %d\n", sampleCounter+4 );
				FontBitmaps_RenderPropText( &FontBitmaps, surf, x, y, (uint8_t*)str, step );
				y += lineH;
				sprintf( str, "sampleCounter = %d\n", sampleCounter+5 );
				FontBitmaps_RenderPropText( &FontBitmaps, surf, x, y, (uint8_t*)str, step );
				y += lineH;
				sprintf( str, "sampleCounter = %d\n", sampleCounter+6 );
				FontBitmaps_RenderPropText( &FontBitmaps, surf, x, y, (uint8_t*)str, step );
				y += lineH;
				sprintf( str, "sampleCounter = %d\n", sampleCounter+7 );
				FontBitmaps_RenderPropText( &FontBitmaps, surf, x, y, (uint8_t*)str, step );
			}
			
			//J フォントセット選択解除
			//-------------------------
			Fonts_DetachFont( cf );
		}
		sampleCounter++;
	}
	#endif //LIBFONT_SAMPLE

	// copy directly. texture is not swizzled 
	memcpy(texture, buffer, width*height*4);
}

//-----------------------------------------------------------------------------
// Description: Command buffer Initialization
// Parameters: 
// Returns: boolean
// Notes: 
//   This function creates user-defined command buffer, which can be used
//   to restore application specific render state at every frame.
//-----------------------------------------------------------------------------
bool SampleApp::initStateBuffer(void)
{
	// allocate buffer on main memory
	mStateBufferAddress = FWCellGCMWindow::getInstance()->getStateCmdAddress();
	if( mStateBufferAddress == NULL ) return false;

	// map allocated buffer
	mStateBufferOffset = FWCellGCMWindow::getInstance()->getStateCmdOffset();
	
	return true;
}

//-----------------------------------------------------------------------------
// Description: Initialization callback
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
bool SampleApp::onInit(int argc, char **ppArgv)
{
	FWGCMCamControlApplication::onInit(argc, ppArgv);

	// Graphics initialization
	mTextureAddress
		= (void*)cellGcmUtilAllocateLocalMemory(mTextureWidth*mTextureHeight*mTextureDepth, 128);
	cellGcmAddressToOffset(mTextureAddress, &mTextureOffset);

	mVertexBuffer = (VertexData3D*)cellGcmUtilAllocateLocalMemory(sizeof(VertexData3D)*3*2*6, 128); // 2x6 triangles
	cellGcmAddressToOffset(mVertexBuffer, &mVertexOffset);

	// build verts
	mVertexCount = cellUtilGenerateCube(mVertexBuffer, 1.0f);

	// system texture.
	// This is used because writes to vid mem are not as fast compared to 
	// main memory.
	mTexBuffer = (uint32_t *)malloc(mTextureWidth * mTextureHeight * mTextureDepth);
	assert(mTexBuffer != NULL);

	// cos generate table
	for (uint32_t i = 0; i < 256*4; i++) {
		mCosTable[i] = (cosf(i*2.0*PI/256));
	}

	// generate palette 
	float R[] = { 255, 0 };
	float G[] = { 0,   0 };
	float B[] = { 0,   0 };
	for (uint32_t i = 0; i < 128; i++) {
		float t = (float)i / 128.0;
		uint32_t r0 = (uint32_t)(t*R[0] + (1-t)*R[1]);
		uint32_t g0 = (uint32_t)(t*G[0] + (1-t)*G[1]);
		uint32_t b0 = (uint32_t)(t*B[0] + (1-t)*B[1]);

		uint32_t r1 = (uint32_t)(t*R[1] + (1-t)*R[0]);
		uint32_t g1 = (uint32_t)(t*G[1] + (1-t)*G[0]);
		uint32_t b1 = (uint32_t)(t*B[1] + (1-t)*B[0]);

		mPalette[i]      = (r0<<16)| (g0<<8) | (b0<<0);
		mPalette[i+128]  = (r1<<16)| (g1<<8) | (b1<<0);
	}

	// reset sempahore
	mLabel = cellGcmGetLabelAddress(sLabelId);
	*mLabel = mLabelValue; // initial value: 0

	// shader setup
	// init shader
	initShader();

	mModelViewProj
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "modelViewProj");
	CGparameter position
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "position");
	CGparameter texcoord
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "texcoord");

	// get vertex attribute index
	mPosIndex = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, position) - CG_ATTR0);
	mTexIndex = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, texcoord) - CG_ATTR0);

	// set texture parameters
	CGparameter texture
		= cellGcmCgGetNamedParameter(mCGFragmentProgram, "texture");
	mTexUnit = (CGresource)(cellGcmCgGetParameterResource(mCGFragmentProgram, texture) - CG_TEXUNIT0);

	// initizalize command buffer for state 
	if(initStateBuffer() != true) return false;

#ifdef CELL_GCM_DEBUG // {
	gCellGcmDebugCallback = NULL;
#endif // }

	// inital state
	cellGcmSetCurrentBuffer(mStateBufferAddress, FWCellGCMWindow::getInstance()->getStateCmdSize());
	{
		cellGcmSetClearColor( 0x00FF0000 );

		cellGcmSetBlendEnable(CELL_GCM_FALSE);
		cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
		cellGcmSetDepthFunc(CELL_GCM_LESS);
		cellGcmSetShadeMode(CELL_GCM_SMOOTH);

		// Need to put Return command because this command buffer is called statically
		cellGcmSetReturnCommand();
	}
	// Get back to default command buffer
	cellGcmSetDefaultCommandBuffer();

#ifdef CELL_GCM_DEBUG // {
	gCellGcmDebugCallback = cellGcmDebugFinish;
#endif // }

	// set default camera position
	mCamera.setPosition(Point3(0.f, 0.f, 2.f));
	#ifdef  LIBFONT_SAMPLE 
	{
		int ret;
		
		//J libfontライブラリのロード
		//------------------------------------------
		ret = Fonts_LoadModules();
		if ( ret != CELL_OK ) {
			printf("App:Fonts LoadModule NG!\n");
			return false;
		}
		
		//J libfontライブラリ初期化
		//------------------------------------------
		fonts = Fonts_Init();
		
		#ifdef  LIBFONTGCM_SAMPLE
		//J libfontGcm を使用する処理
		
		//J libfontグラフィクスの初期化 (libfontGcm初期化)
		//------------------------------------------
		while ( fonts ) {
			const CellFontGraphics* graphicsGcm;
			
			//J CellFontGraphicsに必要なリーソースを確保
			{
				//J CellFontGraphicsに必要な、RSXメモリを確保
				fontGraphicsMemorySize     = 128*1024;
				fontGraphicsMemory         = (void*)cellGcmUtilAllocateLocalMemory( fontGraphicsMemorySize, 128 );
				
				//J CellFontGraphicsに必要な、メインメモリを確保
				//J (このサンプルでは、他にマップされたメモリがないためここで最小単位の1MBを確保します。)
				fontMappedMainMemorySize = 1*1024*1024;
				ret = sys_memory_allocate( fontMappedMainMemorySize, SYS_MEMORY_PAGE_SIZE_1M, (sys_addr_t*)(int)&fontMappedMainMemory );
				if ( ret == CELL_OK ) {
					uint32_t offset;
					//J RSX(R)で参照できるようマップします。マップできるの最小単位は1MBです。
					ret = cellGcmMapMainMemory( fontMappedMainMemory, fontMappedMainMemorySize, &offset );
					if ( ret == CELL_OK ) {
						//J 64KBをCellFontGlaphicsの初期化に、残りを頂点バッファワークとして使用することとします。
						fontVertexBufferSize = (fontMappedMainMemorySize - 64*1024);
						fontVertexBuffer = (uint32_t*)((uint8_t*)fontMappedMainMemory + 64*1024);
					}
				}
				printf("App:FontGraphics Memory Alocate %08x,%08x OK!\n", (int)fontGraphicsMemory, (int)fontMappedMainMemory );
			}
			
			
			//J CellFontGraphics 初期化
			ret = FontGraphics_InitGcm( fontGraphicsMemory, fontGraphicsMemorySize,
			                             fontMappedMainMemory, fontMappedMainMemorySize,
			                             0, &graphicsGcm );
			if ( ret == CELL_OK ) {
				//J 描画コンテキストの初期化
				ret = FontGraphics_SetupDrawContext( graphicsGcm, &FontDrawContext );
				if ( ret == CELL_OK ) {
					fontGraphicsGcm = graphicsGcm;
					fontDC = &FontDrawContext;
					break; //J 成功
				}
				//J エラー処理
				fontDC = (CellFontGraphicsDrawContext*)0;
				FontGraphics_End( graphicsGcm );
			}
			//J エラー処理
			Fonts_End();
			fonts = (Fonts_t*)0;
			break;
		}
		#endif //LIBFONTGCM_SAMPLE
		
		//J フォントインターフェースの初期化
		//------------------------------------------
		while ( fonts ) {
			//J FreeTypeインターフェースライブラリ初期化
			ret = Fonts_InitLibraryFreeType( &freeType );
			if ( ret == CELL_OK ) {
				//J フォントオープン
				ret = Fonts_OpenFonts( freeType, fonts );
				if ( ret == CELL_OK ) {
					//J レンダラーを１つ生成
					ret = Fonts_CreateRenderer( freeType, 0, &RenderWork.Renderer );
					if ( ret == CELL_OK ) {
						//J フォント初期化 正常終了!
						printf("App:Fonts Initialize All OK!\n");
						break;
					}
					//J 以下エラー処置
					//J フォントクローズ
					Fonts_CloseFonts( fonts );
				}
				Fonts_EndLibrary( freeType );
			}
			Fonts_End();
			fonts = (Fonts_t*)0;
			break;
		}

		//J ビットマップベースのフォント処理サンプルの初期化
		//---------------------------------------------------
		if ( fonts ) {
			//J ビットマップベースのフォント用にフォントを割り当て
			ret = Fonts_AttachFont( fonts, FONT_SYSTEM_GOTHIC_LATIN, &FontBitmaps.Font );
			Fonts_BindRenderer( &FontBitmaps.Font, &RenderWork.Renderer );
			
			//J ビットマップフォント初期化
			FontBitmaps_Init( &FontBitmaps, (CellFont*)0, 16.f, 16.f, 1.0f, 0.1f, 256 );
		}
		
		#ifdef  LIBFONTGCM_SAMPLE
		//J libfontGcm を使用する処理のサンプル
		
		//J 文字の輪郭線を、頂点に展開したグリフ情報の作成
		//---------------------------------------------------
		if ( fontGraphicsGcm ) {
			CellFont vFont;
			CellFont* cf;
			int fn;
			
			fn = FONT_SYSTEM_GOTHIC_LATIN; //J システムフォントセット
			//fn = FONT_USER_FONT0;        //J 独自に用意したフォントを使用する場合
			
			//J 頂点グリフを作成に使用するフォントセットを選択
			ret = Fonts_AttachFont( fonts, fn, &vFont );
			if ( ret == CELL_OK ) {
				cf = &vFont;
				printf("Fonts_AttachFont succeded\n");
			} else { 
				cf = (CellFont*)0;
				printf("Fonts_AttachFont failed\n");
			}
			
			//J 頂点グリフ(VertexesGlyph)を作成
			if ( cf ) {
				uint32_t* vBuffer = fontVertexBuffer;
				uint32_t vBufSize = fontVertexBufferSize;
				FontVertexGlyph_t *vGlyphs;
				float glyphBaseScale;
				uint8_t* utf8;
				int n;
				
				//J 頂点グリフを作成するフォントスケールを設定しておく
				glyphBaseScale = 64.f;
				Fonts_SetFontScale( cf, glyphBaseScale );
				
				//J 頂点グリフを作成するフォントスケールを設定しておく
				Fonts_GetFontHorizontalLayout( cf, &VertexesGlyphs.Layout.lineHeight, &VertexesGlyphs.Layout.baseLineY );
				VertexesGlyphs.Layout.lineHeight *= 1.1f;
				VertexesGlyphs.Layout.scaleX = glyphBaseScale;
				VertexesGlyphs.Layout.scaleY = glyphBaseScale;
				VertexesGlyphs.glyphScaleMax = getDisplayInfo().mHeight * 0.75f;
				
				utf8 = (uint8_t*)"この文章は CellFontGraphics(libfontGcm) を使用して表示しています。";
				vGlyphs = &VertexesGlyphs.Array[0];
				
				//J 頂点グリフを指定文字列分、作成
				for ( n=0; n<FONT_VERTEX_GLYPH_MAX; n++ ) {
					uint32_t code;
					uint32_t dataSize;
					
					utf8 += Fonts_getUcs4FromUtf8( utf8, &code, 0x3000 );
					
					if ( code == 0 ) break;
					
					//J 頂点グリフをセットアップ
					ret = FontGraphics_SetupFontVertexesGlyph( cf, code, VertexesGlyphs.glyphScaleMax,
					                                           vBuffer, vBufSize,
					                                           &vGlyphs[n].VertexesGlyph, &dataSize,
					                                           &vGlyphs[n].Metrics );
					if ( ret == CELL_OK ) {
						vGlyphs[n].BaseScale.x = glyphBaseScale;
						vGlyphs[n].BaseScale.y = glyphBaseScale;
						vBuffer = (uint32_t*)((uint8_t*)vBuffer + dataSize );
						vBufSize -= dataSize;
						if ( vBufSize <= 0 ) break;
					}
					else {
						break;
					}
				}
				VertexesGlyphs.count = n;
				if ( ret != CELL_OK ) {
					printf("App:FontGraphics init data NG!\n");
				}
				
				//J 使用を終えたフォントセットを返却
				Fonts_DetachFont( cf );
			}
		}
		#endif //LIBFONTGCM_SAMPLE
		
		if (! fonts ) {
			printf("App:Fonts Initialize NG!\n");
			return false;
		}
	}
	#endif //LIBFONT_SAMPLE

	return true;
}

//-----------------------------------------------------------------------------
// Description: Render callback
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------

bool render = 0;
void SampleApp::onRender()
{
	if (render++) {
		cellGcmFinish(0x13131313);
		exit(0);
	}
	// base implementation clears screen and sets up camera
	FWGCMCamControlApplication::onRender();

	// re-execute state commands created in onInit() 
	cellGcmSetCallCommand(mStateBufferOffset);

	while (*((volatile uint32_t *)mLabel) != mLabelValue) {
		sys_timer_usleep(10);
	}
	mLabelValue++;

	// build texture
	generateTexture((uint8_t*)mTextureAddress, mTexBuffer,
					mCosTable, mPalette, mTextureWidth, mTextureHeight);

	// set vertex pointer and draw
	cellGcmSetVertexDataArray(mPosIndex, 0, sizeof(VertexData3D), 3,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  mVertexOffset);
	cellGcmSetVertexDataArray(mTexIndex, 0, sizeof(VertexData3D), 2,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  mVertexOffset+sizeof(float)*4);

	cellGcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_TEXTURE);
	// bind texture
	uint32_t format, remap;
	cellGcmUtilGetTextureAttribute(CELL_GCM_UTIL_ARGB8, &format, &remap, 0, 1);

	CellGcmTexture tex;
	tex.format = format;
	tex.mipmap = 1;
	tex.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	tex.cubemap = CELL_GCM_FALSE;
	tex.remap = remap;
	tex.width = mTextureWidth;
	tex.height = mTextureHeight;
	tex.depth = 1;
	tex.pitch = mTextureWidth*mTextureDepth;
	tex.location = CELL_GCM_LOCATION_LOCAL;
	tex.offset = mTextureOffset;
	cellGcmSetTexture(mTexUnit, &tex);

	// bind texture and set filter
	cellGcmSetTextureControl(mTexUnit, CELL_GCM_TRUE, 0<<8, 12<<8, CELL_GCM_TEXTURE_MAX_ANISO_1); // MIN:0,MAX:12
	cellGcmSetTextureAddress(mTexUnit,
							 CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
							 CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
							 CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
							 CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL,
							 CELL_GCM_TEXTURE_ZFUNC_LESS, 0);
	cellGcmSetTextureFilter(mTexUnit, 0,
							#ifdef  LIBFONT_SAMPLE
							CELL_GCM_TEXTURE_LINEAR,
							#else
							CELL_GCM_TEXTURE_NEAREST_LINEAR,
							#endif //LIBFONT_SAMPLE
							CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);

	// bind Cg programs
	// NOTE: vertex program constants are copied here
	cellGcmSetVertexProgram(mCGVertexProgram, mVertexProgramUCode);
	cellGcmSetFragmentProgram(mCGFragmentProgram, mFragmentProgramOffset);

	// model rotate
	static float AngleX = 0.3f; 
	static float AngleY = 0.3f; 
	static float AngleZ = 0.0f;
	AngleX += 0.01f;
	AngleY += 0.01f;
	AngleZ += 0.000f;
	if( AngleX > PI2 ) AngleX -= PI2;
	if( AngleY > PI2 ) AngleY -= PI2;
	if( AngleZ > PI2 ) AngleZ -= PI2;

	// model matrix
	Matrix4 mat = Matrix4::rotationZYX(Vector3(AngleX, AngleY, AngleZ));
	#ifdef  LIBFONT_SAMPLE
	//AngleX = 0.00f;
	//AngleY = 0.00f;
	//AngleZ = 0.00f;
	mat.setTranslation(Vector3(0.0f, 0.0f, -2.0f));
	#else
	mat.setTranslation(Vector3(0.0f, 0.0f, -5.0f));
	#endif //LIBFONT_SAMPLE

	// final matrix
	Matrix4 MVP = transpose(getProjectionMatrix() * getViewMatrix() * mat);

	// set MVP matrix
	cellGcmSetVertexProgramParameter(mModelViewProj, (float*)&MVP);
	cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_TRIANGLES, 0, mVertexCount);

	#ifdef  LIBFONTGCM_SAMPLE
	if ( fontGraphicsGcm ) { //J libfontGcm を使用する処理のサンプル
		
		//J CellFontRenderSurfaceGcm の準備
		{
			CellGcmSurface surface;
			int ret;
			
			{//J SampleAppの描画フレームバッファを、CellGcmSurfaceに設定
				FWCellGCMWindow* win = FWCellGCMWindow::getInstance();
				uint32_t surfIndex = win->getFrameIndex();
				
				memset(&surface, 0, sizeof(surface));
				
				surface.colorFormat	 = CELL_GCM_SURFACE_A8R8G8B8;
				surface.colorTarget		 = CELL_GCM_SURFACE_TARGET_0;
				surface.colorLocation[0] = CELL_GCM_LOCATION_LOCAL;
				surface.colorOffset[0]   = FWCellGCMWindow::getInstance()->getFrameOffset( surfIndex );
				surface.colorPitch[0] 	 = FWCellGCMWindow::getInstance()->getFramePitch( surfIndex );
				surface.colorLocation[1] = CELL_GCM_LOCATION_LOCAL;
				surface.colorOffset[1]   = 0;
				surface.colorPitch[1] 	 = 64;
				surface.colorLocation[2] = CELL_GCM_LOCATION_LOCAL;
				surface.colorOffset[2]   = 0;
				surface.colorPitch[2] 	 = 64;
				surface.colorLocation[3] = CELL_GCM_LOCATION_LOCAL;
				surface.colorOffset[3]   = 0;
				surface.colorPitch[3] 	 = 64;
				surface.depthFormat 	= CELL_GCM_SURFACE_Z24S8;
				surface.depthLocation 	= CELL_GCM_LOCATION_LOCAL;
				surface.depthOffset 	= win->getFrameOffset( 2 );
				surface.depthPitch 		= win->getFramePitch( 2 );
				surface.type			= CELL_GCM_SURFACE_PITCH;
				surface.antialias 		= CELL_GCM_SURFACE_CENTER_1;
				surface.width 			= getDisplayInfo().mWidth;
				surface.height 			= getDisplayInfo().mHeight;
				surface.x 		 		= 0;
				surface.y 		 		= 0;
			}
			
			//J CellGcmSurfaceから、CellFontRenderSurfaceGcm を作成
			ret = cellFontRenderSurfaceGcmInit( &RenderSurfGcm, &surface );
			if ( ret == CELL_OK ) {
				printf("setting scissors: %d, %d\n", surface.width, surface.height);
				//J 必要であればサーフェスにシザリング領域を設定。
				cellFontRenderSurfaceGcmSetScissor( &RenderSurfGcm, 0, 0, surface.width, surface.height );
			}
		}
		
		//J 頂点グリフ(VertexesGlyph)の描画
		{
			static float color_none[4]  = { 1.0f, 1.0f, 1.0f, 0.0f };
			static float color_white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			static float x = 0;
			float endx;
			float *fontRGBA;
			float *lineRGBA;
			
			CellGcmContextData* gcm = gCellGcmCurrentContext;

			if ( sampleType < 3 || true ) {
				printf("sampleType < 3\n");
			 //J 画面下段に文字をテロップの様に表示
				float baseLineY = getDisplayInfo().mHeight * 0.90f;
				float fontScale = getDisplayInfo().mHeight / 18.0f;
				float lineY = baseLineY - VertexesGlyphs.Layout.baseLineY  * fontScale / VertexesGlyphs.Layout.scaleY;
				float lineH = VertexesGlyphs.Layout.lineHeight * fontScale / VertexesGlyphs.Layout.scaleY;
				float scrollStep = getDisplayInfo().mWidth / 160;
				float color_anti[4];
				
				if ( sampleType == 0 && sampleTimer == 0 ) {
					x = getDisplayInfo().mWidth;
				}

				x = 100;
				
				//J 文字の描画スケールを設定
				cellFontGraphicsSetScalePixel( fontDC, fontScale, fontScale );
				//J 文字の描画方法を設定
				//cellFontGraphicsSetDrawType( fontDC, CELL_FONT_GRAPHICS_DRAW_TYPE_COLOR );
				cellFontGraphicsSetDrawType( fontDC, CELL_FONT_GRAPHICS_DRAW_TYPE_MONO );
				
				//J 文字の描画色を設定
				fontRGBA = color_white;
				lineRGBA = color_anti;
				{
					int anti_aliasing_level = sampleType;
					
					color_anti[0] = fontRGBA[0];
					color_anti[1] = fontRGBA[1];
					color_anti[2] = fontRGBA[2];
					//J 細身のフォントを小さく表示する場合に効果があります。
					color_anti[3] = 0.125f * anti_aliasing_level;  //J for light weight fonts
				}
				//cellFontGraphicsSetFontRGBA( fontDC, fontRGBA );
				//cellFontGraphicsSetLineRGBA( fontDC, color_anti );
				
				//J 描画対象のサーフェスにシザリング領域を設定
				//cellFontRenderSurfaceGcmSetScissor( &RenderSurfGcm, 0, (int)lineY, getDisplayInfo().mWidth, (uint32_t)lineH );
				//J 描画対象のサーフェスをクリアするGcmコマンドパケットをセット


				/*cellFontGraphicsGcmSetClearSurface( gcm, &RenderSurfGcm,
				                                         (CELL_GCM_CLEAR_R|CELL_GCM_CLEAR_G|CELL_GCM_CLEAR_B|CELL_GCM_CLEAR_A),
				                                         fontDC );*/
				cellGcmSetBlendEnable(1);
				cellGcmSetBlendEnable(1);
				cellGcmSetBlendEnable(1);
				//J サンプル文字列の描画を行うGcmコマンドパケットをセット
				/*endx = FontGraphics_GcmSetDrawGlyphArray( gcm, &RenderSurfGcm, x, baseLineY,
				                                               VertexesGlyphs.Array, VertexesGlyphs.count, fontDC );*/
				endx = FontGraphics_GcmSetDrawGlyphArray( gcm, &RenderSurfGcm, x, baseLineY,
				                                               VertexesGlyphs.Array, 1, fontDC );
				cellGcmSetBlendEnable(1);
				cellGcmSetBlendEnable(1);
				cellGcmSetBlendEnable(1);

				x -= scrollStep;
			}
			else {
				printf("sampleType else\n");
			 //J 巨大サイズの文字をスクロール表示
				float baseLineY = getDisplayInfo().mHeight * 0.79f;
				float fontScale = getDisplayInfo().mHeight * 0.75f;
				float scrollStep = getDisplayInfo().mWidth / 60;
				float color_alpha[4];
				
				//J 文字の描画スケールを設定
				cellFontGraphicsSetScalePixel( fontDC, fontScale, fontScale );
				
				fontRGBA = color_white;
				lineRGBA = color_none;
				
				//J 文字の描画方法を設定
				if ( sampleType == 3 ) {
					if ( sampleTimer == 0 ) {
						x = getDisplayInfo().mWidth;
					}
					cellFontGraphicsSetDrawType( fontDC, CELL_FONT_GRAPHICS_DRAW_TYPE_COLOR );
				}
				else
				if ( sampleType == 4 ) {
					cellFontGraphicsSetDrawType( fontDC, CELL_FONT_GRAPHICS_DRAW_TYPE_COLOR_REVERSE );
					float blendRate, blendTime = 60.f;
					
					if ( sampleTimer <= blendTime )                 blendRate = 1.0f - (float)sampleTimer/blendTime;
					else if ( sampleTimer >= sampleTime-blendTime ) blendRate = (float)(sampleTimer-(sampleTime-blendTime))/blendTime;
					else                                            blendRate = 0.0f;
					color_alpha[0] = fontRGBA[0];
					color_alpha[1] = fontRGBA[1];
					color_alpha[2] = fontRGBA[2];
					color_alpha[3] = blendRate;
					fontRGBA = color_alpha;
					lineRGBA = color_white;
				}
				else {
					cellFontGraphicsSetDrawType( fontDC, CELL_FONT_GRAPHICS_DRAW_TYPE_COLOR );
					color_alpha[0] = fontRGBA[0];
					color_alpha[1] = fontRGBA[1];
					color_alpha[2] = fontRGBA[2];
					color_alpha[3] = 1.0f - ( (float)sampleTimer / (float)(sampleTime-1) );
					fontRGBA = color_alpha;
				}
				//J 文字の描画色を設定
				cellFontGraphicsSetFontRGBA( fontDC, fontRGBA );
				cellFontGraphicsSetLineRGBA( fontDC, lineRGBA );
				
				//J サンプル文字列の描画を行うGcmコマンドパケットをセット
				endx = FontGraphics_GcmSetDrawGlyphArray( gcm, &RenderSurfGcm, x, baseLineY,
				                                               VertexesGlyphs.Array, VertexesGlyphs.count, fontDC );
				x -= scrollStep;
			}
			if ( endx < 0.0f ) {
				x = getDisplayInfo().mWidth;
			}
		}
		
		//J ビューポートと描画サーフェス、および、シザリング領域をSampleAppの設定に戻す。
		setViewport();
		FWCellGCMWindow::getInstance()->resetRenderTarget();
		//J 文字描画前にセットされていたシェーダーや描画設定は失われているため
		//J 必要な場合は、適宜、再設定が必要です。
	}
	#endif //LIBFONTGCM_SAMPLE
	cellGcmSetWriteTextureLabel(sLabelId, mLabelValue);

	// flush semaphore command bacause semaphore value is not updated
	cellGcmFlush();

	static int frame = 0;
	static float fFPS = 0.f;

	// Draw FPS on screen
	{
		frame++;

		if(frame == 60) 
		{
			FWTimeVal	time = FWTime::getCurrentTime();

			fFPS = 60.f / (float)((time - mLastTime));
			mLastTime = time;

			frame = 0;
		}

		// print some messages
		FWDebugFont::setPosition(0, 0);
		FWDebugFont::setColor(1.f, 1.f, 1.f, 1.0f);

		//FWDebugFont::print("Cube Sample Application\n\n");
		//FWDebugFont::printf("FPS: %.2f\n", fFPS);
	}
}

//-----------------------------------------------------------------------------
// Description: Resize callback
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
void SampleApp::onSize(const FWDisplayInfo& rDispInfo)
{
	FWGCMCamControlApplication::onSize(rDispInfo);
}

//-----------------------------------------------------------------------------
// Description: Shutdown callback
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
void SampleApp::onShutdown()
{
	#ifdef  LIBFONT_SAMPLE
	if ( fonts ) {
		FontBitmaps_End( &FontBitmaps );
		Fonts_UnbindRenderer( &FontBitmaps.Font );
		Fonts_DetachFont( &FontBitmaps.Font );
	
		Fonts_CloseFonts( fonts );
		Fonts_DestroyRenderer( &RenderWork.Renderer );
		Fonts_EndLibrary( freeType );
		#ifdef  LIBFONTGCM_SAMPLE
		//J libfontGcm を使用する処理のサンプル
		FontGraphics_End( fontGraphicsGcm );
		{
			int ret = cellGcmUnmapEaIoAddress( fontMappedMainMemory );
			if ( ret == CELL_OK ) {
				sys_memory_free(  (sys_addr_t)fontMappedMainMemory );
				fontMappedMainMemory = (void*)0;
			}
		}
		#endif //LIBFONTGCM_SAMPLE
		Fonts_End();
	}
	//J libfontライブラリのアンロード
	Fonts_UnloadModules();
	#endif //LIBFONT_SAMPLE
	FWGCMCamControlApplication::onShutdown();
}
