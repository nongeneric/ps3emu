/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2009 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */
/*   File: main.c
 *   Description:
 *     how to modify from gcm basic sample for using resc
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <sys/timer.h>
#include <sys/return_code.h>

#include <sysutil/sysutil_sysparam.h>
#include <cell/gcm.h>
#include <cell/resc.h>

#include "snaviutil.h"

using namespace cell::Gcm;

/*E these tags are evaporated when compiled, just for being focalized */
#define RESC_BLOCK_BEGIN(N)
#define RESC_BLOCK_END


RESC_BLOCK_BEGIN(1)
/*E single buffering */
#define COLOR_BUFFER_NUM 1
RESC_BLOCK_END

#define RENDERING_BUF_WIDTH  1280
#define RENDERING_BUF_HEIGHT  720

#define COLOR_DEPTH  4
#define TILE_INDEX   14

typedef struct
{
	float x, y, z;
	uint32_t rgba; 
} Vertex_t;

//E For exit routine
static void initExitRoutine(void);
static void programExitCallback(void);
static void sysutilExitCallback(uint64_t status, uint64_t param, void* userdata);
static bool sKeepRunning = true;

/*E prototypes */
extern "C" int32_t userMain(void);

uint32_t display_width;
uint32_t display_height;

uint32_t color_pitch;
uint32_t depth_pitch;
uint32_t color_offset[4];
uint32_t depth_offset;

extern uint32_t _binary_vpshader_vpo_start;
extern uint32_t _binary_vpshader_vpo_end;
extern uint32_t _binary_fpshader_fpo_start;
extern uint32_t _binary_fpshader_fpo_end;

static unsigned char *vertex_program_ptr = 
(unsigned char *)&_binary_vpshader_vpo_start;
static unsigned char *fragment_program_ptr = 
(unsigned char *)&_binary_fpshader_fpo_start;

static CGprogram vertex_program;
static CGprogram fragment_program;

static void *vertex_program_ucode;
static void *fragment_program_ucode;

static Vertex_t *vertex_array_buffer;


/*E local memory allocation */
static uint32_t local_mem_heap = 0;
static void *localMemoryAlloc(const uint32_t size) 
{
	uint32_t allocated_size = (size + 1023) & (~1023);
	uint32_t base = local_mem_heap;
	local_mem_heap += allocated_size;
	return(void*)base;
}

static void *localMemoryAlign(const uint32_t alignment, 
							  const uint32_t size)
{
	local_mem_heap = (local_mem_heap + alignment-1) & (~(alignment-1));
	return(void*)localMemoryAlloc(size);
}

static void buildProjection(float *M, 
							const float top, 
							const float bottom, 
							const float left, 
							const float right, 
							const float near, 
							const float far)
{
	memset(M, 0, 16*sizeof(float)); 

	M[0*4+0] = (2.0f*near) / (right - left);
	M[1*4+1] = (2.0f*near) / (bottom - top);

	float A = (right + left) / (right - left);
	float B = (top + bottom) / (top - bottom);
	float C = -(far + near) / (far - near);
	float D = -(2.0f*far*near) / (far - near);

	M[0*4 + 2] = A;
	M[1*4 + 2] = B;
	M[2*4 + 2] = C;
	M[3*4 + 2] = -1.0f;
	M[2*4 + 3] = D;
}

static void matrixMul(float *Dest, float *A, float *B)
{
	for (int i=0; i < 4; i++) {
		for (int j=0; j < 4; j++) {
			Dest[i*4+j] 
			= A[i*4+0]*B[0*4+j] 
			  + A[i*4+1]*B[1*4+j] 
			  + A[i*4+2]*B[2*4+j] 
			  + A[i*4+3]*B[3*4+j];
		}
	}
}

static void matrixTranslate(float *M, 
							const float x, 
							const float y, 
							const float z)
{
	memset(M, 0, sizeof(float)*16);
	M[0*4+3] = x;
	M[1*4+3] = y;
	M[2*4+3] = z;

	M[0*4+0] = 1.0f;
	M[1*4+1] = 1.0f;
	M[2*4+2] = 1.0f;
	M[3*4+3] = 1.0f;
}

static void unitMatrix(float *M)
{
	M[0*4+0] = 1.0f;
	M[0*4+1] = 0.0f;
	M[0*4+2] = 0.0f;
	M[0*4+3] = 0.0f;

	M[1*4+0] = 0.0f;
	M[1*4+1] = 1.0f;
	M[1*4+2] = 0.0f;
	M[1*4+3] = 0.0f;

	M[2*4+0] = 0.0f;
	M[2*4+1] = 0.0f;
	M[2*4+2] = 1.0f;
	M[2*4+3] = 0.0f;

	M[3*4+0] = 0.0f;
	M[3*4+1] = 0.0f;
	M[3*4+2] = 0.0f;
	M[3*4+3] = 1.0f;
}

static void setVertex(Vertex_t* vertex_buffer)
{
	vertex_buffer[0].x = -1.0f;
	vertex_buffer[0].y = -1.0f;
	vertex_buffer[0].z = -1.0f;
	vertex_buffer[0].rgba=0x00ff0000;

	vertex_buffer[1].x =  1.0f;
	vertex_buffer[1].y = -1.0f;
	vertex_buffer[1].z = -1.0f;
	vertex_buffer[1].rgba=0x0000ff00;

	vertex_buffer[2].x = -1.0f;
	vertex_buffer[2].y =  1.0f;
	vertex_buffer[2].z = -1.0f;
	vertex_buffer[2].rgba=0xff000000;
}


static int32_t initLocalBuffer(uint32_t width, uint32_t height)
{
	uint32_t color_depth=4;	// ARGB8
	uint32_t z_depth=4;		// COMPONENT24
	void *color_base_addr;
	void *depth_base_addr;
	void *color_addr[4];
	void *depth_addr;
	int32_t ret;
	display_width = width;
	display_height = height;
	color_pitch = display_width*color_depth;
	depth_pitch = display_width*z_depth;

	uint32_t color_size = color_pitch*display_height;
	uint32_t depth_size = depth_pitch*display_height;

	//E get config
	CellGcmConfig config;
	cellGcmGetConfiguration(&config);
	//E buffer memory allocation
	local_mem_heap = (uint32_t)config.localAddress;

	//E color buffer
	color_base_addr= localMemoryAlign(16, COLOR_BUFFER_NUM*color_size);

	for (int i = 0; i < COLOR_BUFFER_NUM; i++) {
		color_addr[i]
		= (void *)((uint32_t)color_base_addr+ (i*color_size));
		ret = cellGcmAddressToOffset(color_addr[i], &color_offset[i]);
		if (ret != CELL_OK)	return -1;
	}

	//E depth buffer
	depth_base_addr = localMemoryAlign(0x10000, depth_size);
	depth_addr = depth_base_addr;
	ret = cellGcmAddressToOffset(depth_addr, &depth_offset);
	if (ret != CELL_OK)	return -1;

	return 0;
}

static void initShader(void)
{
	vertex_program   = (CGprogram)vertex_program_ptr;
	fragment_program = (CGprogram)fragment_program_ptr;

	//E init
	cellGcmCgInitProgram(vertex_program);
	cellGcmCgInitProgram(fragment_program);

	uint32_t ucode_size;
	void *ucode;
	cellGcmCgGetUCode(fragment_program, &ucode, &ucode_size);
	//E require 64 bytes alignment
	void *ret = localMemoryAlign(64, ucode_size);
	fragment_program_ucode = ret;
	memcpy(fragment_program_ucode, ucode, ucode_size);

	cellGcmCgGetUCode(vertex_program, &ucode, &ucode_size);
	vertex_program_ucode = ucode;
}

static void initRenderObject(void)
{
	vertex_array_buffer = (Vertex_t*)localMemoryAlign(0x100, sizeof(Vertex_t)*3);

	setVertex(vertex_array_buffer);
}

static bool sFlipped=true;
static void FlipCallback(uint32_t head)
{
	(void)head;
	sFlipped=true;
}

/*E wait until flip */
static void waitFlip(void)
{
	while (!sFlipped){
		sys_timer_usleep(300);
	}
	sFlipped=false;
}

static void flip(void)
{
	RESC_BLOCK_BEGIN(5) {
		//E It is important to skip issuring WaitFlip if ConvertAndFlip failed.
		//J ConverAndFlip が何らかの理由で失敗した時には、WaitFlip を発行してはならない
		if (cellRescSetConvertAndFlip(0) != CELL_OK) return;

		cellGcmFlush();

		cellRescSetWaitFlip();
	} RESC_BLOCK_END
}

static int32_t getRescDestsIndex(CellRescBufferMode dstMode) {
	if(dstMode == CELL_RESC_720x480)         return 0;
	else if (dstMode == CELL_RESC_720x576)   return 1;
	else if (dstMode == CELL_RESC_1280x720)  return 2;
	else if (dstMode == CELL_RESC_1920x1080) return 3;
	else                                     return -1;
}

RESC_BLOCK_BEGIN(2)
static void initResc(void)
{
	//E Here we show the libresc initialization
	//J libresc の初期化を行なう
	CellRescInitConfig conf;
	CellRescDsts dsts[4];
	CellRescBufferMode bufMode;
	CellVideoOutState videoState;
	int32_t colorBuffersSize, vertexArraySize, fragmentShaderSize;
	void *pColorBuffers, *pVertexArray, *pFragmentShader;

	//E Get video out state by sysutil.
	//J sysutil を使用して、ビデオ出力設定を取得する
	cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState);


	//E Setup initConfig structure
	//J 初期化構造体の設定
	memset(&conf, 0, sizeof(CellRescInitConfig));
	conf.size            = sizeof(CellRescInitConfig);
	conf.resourcePolicy  = CELL_RESC_MINIMUM_GPU_LOAD | CELL_RESC_CONSTANT_VRAM;
	conf.supportModes    = CELL_RESC_1920x1080 | CELL_RESC_1280x720 
							| CELL_RESC_720x576 | CELL_RESC_720x480;
	conf.ratioMode       = (videoState.displayMode.aspect == CELL_VIDEO_OUT_ASPECT_4_3) ? 
							CELL_RESC_LETTERBOX : CELL_RESC_FULLSCREEN;
	conf.palTemporalMode = CELL_RESC_PAL_50;
	conf.interlaceMode   = CELL_RESC_NORMAL_BILINEAR;
	conf.flipMode        = CELL_RESC_DISPLAY_VSYNC;

	cellRescInit(&conf);

	cellRescVideoOutResolutionId2RescBufferMode(
		(CellVideoOutResolutionId)videoState.displayMode.resolutionId, 
		&bufMode);

	//E Configure the destination buffers. We use CELL_RESC_CONSTANT_VRAM here, to let libresc estimate memory 
	//E amount correctly, we should configure Dsts for *all* supportModes that the application support.
	//E While, we use specific value to pitch and heightAlign, it is for tiling.
	//E See also libresc overview, section "Example for Setting the Tiled Region and the Compression Mode".
	//J 出力先バッファーの設定をする。CELL_RESC_CONSTANT_VRAMを使用するので、
	//J 必要バッファ量を正しく計算するために、アプリケーションで対応する*全て*のsupportModesについてDstsを適切に設定する。
	//J また、pitch, heightAlign は、タイル設定用に構成する。詳細はlibresc概要「タイルと圧縮の設定例」を参照。
	{
		dsts[0].format      = CELL_RESC_SURFACE_A8R8G8B8;
		dsts[0].pitch       = 0xC00;
		dsts[0].heightAlign = 64;
		cellRescSetDsts(CELL_RESC_720x480, &dsts[0]);
	}
	{
		dsts[1].format      = CELL_RESC_SURFACE_A8R8G8B8;
		dsts[1].pitch       = 0xC00;
		dsts[1].heightAlign = 64;
		cellRescSetDsts(CELL_RESC_720x576, &dsts[1]);
	}
	{
		dsts[2].format      = CELL_RESC_SURFACE_A8R8G8B8;
		dsts[2].pitch       = 0x1400;
		dsts[2].heightAlign = 64;
		cellRescSetDsts(CELL_RESC_1280x720, &dsts[2]);
	}
	{
		dsts[3].format      = CELL_RESC_SURFACE_A8R8G8B8;
		dsts[3].pitch       = 0x2000;
		dsts[3].heightAlign = 32;
		cellRescSetDsts(CELL_RESC_1920x1080, &dsts[3]);
	}
	
	cellRescSetDisplayMode(bufMode);

	cellRescGetBufferSize(&colorBuffersSize, &vertexArraySize, &fragmentShaderSize);

	//E Allocate all the buffers on local heap.
	//J ローカルメモリー上から任意の方法でバッファ用のメモリを確保します。
	pColorBuffers   = localMemoryAlign(0x10000, colorBuffersSize);
	pVertexArray    = localMemoryAlign(0x10000, vertexArraySize);
	pFragmentShader = localMemoryAlign(0x80,    fragmentShaderSize);

	//E Tell resc where he can use
	//J libresc が使用できるバッファを設定する
	cellRescSetBufferAddress(pColorBuffers, pVertexArray, pFragmentShader);

	printf("Required size,  color:0x%x vertex:0x%x fragment:0x%x\n", 
		   colorBuffersSize, vertexArraySize, fragmentShaderSize);
	printf("Buf addr given, color:0x%p vertex:0x%p fragment:0x%p\n", 
		   pColorBuffers, pVertexArray, pFragmentShader);

	//E Set tile onto output buffers.(However tiling is optional, make sure to use it for good performance!)
	//J 出力先バッファにタイルを設定する(タイルはオプションですが、性能的には必ずかけるようにして下さい）
	{
		uint32_t colorOffset;
		uint32_t tileSize = (char*)pVertexArray - (char*)pColorBuffers;
		int32_t dstsIndex = getRescDestsIndex(bufMode);
		cellGcmAddressToOffset(pColorBuffers, &colorOffset);
		cellGcmSetTileInfo(TILE_INDEX, CELL_GCM_LOCATION_LOCAL, colorOffset,
						   tileSize, dsts[dstsIndex].pitch, CELL_GCM_COMPMODE_C32_2X1,
						   colorOffset/0x10000, 0);
		cellGcmBindTile(TILE_INDEX);
		printf("Dst Tile:  offset 0x%x, size 0x%x, base 0x%x\n",
			   colorOffset, tileSize, colorOffset/0x10000);
	}


	cellRescSetFlipHandler(FlipCallback);
}
RESC_BLOCK_END

static void setRenderTarget(const uint32_t Index)
{
	CellGcmSurface sf;
	sf.colorFormat      = CELL_GCM_SURFACE_A8R8G8B8;
	sf.colorTarget      = CELL_GCM_SURFACE_TARGET_0;
	sf.colorLocation[0] = CELL_GCM_LOCATION_LOCAL;
	sf.colorOffset[0]   = color_offset[Index];
	sf.colorPitch[0]    = color_pitch;

	sf.colorLocation[1] = CELL_GCM_LOCATION_LOCAL;
	sf.colorLocation[2] = CELL_GCM_LOCATION_LOCAL;
	sf.colorLocation[3] = CELL_GCM_LOCATION_LOCAL;
	sf.colorOffset[1]   = 0;
	sf.colorOffset[2]   = 0;
	sf.colorOffset[3]   = 0;
	sf.colorPitch[1]    = 64;
	sf.colorPitch[2]    = 64;
	sf.colorPitch[3]    = 64;

	sf.depthFormat      = CELL_GCM_SURFACE_Z24S8;
	sf.depthLocation    = CELL_GCM_LOCATION_LOCAL;
	sf.depthOffset      = depth_offset;
	sf.depthPitch       = depth_pitch;

	sf.type             = CELL_GCM_SURFACE_PITCH;
	sf.antialias        = CELL_GCM_SURFACE_CENTER_1;

	sf.width            = display_width;
	sf.height           = display_height;
	sf.x                = 0;
	sf.y                = 0;
	cellGcmSetSurface(&sf);

	RESC_BLOCK_BEGIN(3) {
		//E in addition, set the rendering surface as resc src buffer.
		//J 追加で、gcm のサーフェイスを resc の入力バッファとしてセットします。
		CellRescSrc rescSrc;
		cellRescGcmSurface2RescSrc(&sf, &rescSrc);
		cellRescSetSrc(Index, &rescSrc);
	} RESC_BLOCK_END
}

static void setDrawEnv(void)
{
	cellGcmSetColorMask(CELL_GCM_COLOR_MASK_B|
						CELL_GCM_COLOR_MASK_G|
						CELL_GCM_COLOR_MASK_R|
						CELL_GCM_COLOR_MASK_A);

	cellGcmSetColorMaskMrt(0);
	uint16_t x,y,w,h;
	float min, max;
	float scale[4],offset[4];

	x = 0;
	y = 0;
	w = display_width;
	h = display_height;
	min = 0.0f;
	max = 1.0f;
	scale[0] = w * 0.5f;
	scale[1] = h * -0.5f;
	scale[2] = (max - min) * 0.5f;
	scale[3] = 0.0f;
	offset[0] = x + scale[0];
	offset[1] = y + h * 0.5f;
	offset[2] = (max + min) * 0.5f;
	offset[3] = 0.0f;

	cellGcmSetViewport(x, y, w, h, min, max, scale, offset);
	cellGcmSetClearColor((64<<0)|(64<<8)|(64<<16)|(64<<24));

	cellGcmSetDepthMask(CELL_GCM_TRUE);

	cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
	cellGcmSetDepthFunc(CELL_GCM_LESS);

	RESC_BLOCK_BEGIN(4) {
		cellGcmSetAlphaTestEnable(CELL_GCM_FALSE);
		cellGcmSetBlendEnable(CELL_GCM_FALSE);
		cellGcmSetBlendEnableMrt(CELL_GCM_FALSE, CELL_GCM_FALSE, CELL_GCM_FALSE);
		cellGcmSetCullFaceEnable(CELL_GCM_FALSE);
		cellGcmSetDepthBoundsTestEnable(CELL_GCM_FALSE);
		cellGcmSetDitherEnable(CELL_GCM_TRUE);
		cellGcmSetLogicOpEnable(CELL_GCM_FALSE);
		cellGcmSetPointSpriteControl(CELL_GCM_FALSE, 0, 0);
		cellGcmSetPolygonOffsetFillEnable(CELL_GCM_FALSE);
		cellGcmSetScissor(0, 0, display_width, display_height);
		cellGcmSetShadeMode(CELL_GCM_SMOOTH);
		cellGcmSetStencilTestEnable(CELL_GCM_FALSE);
		cellGcmSetTwoSidedStencilTestEnable(CELL_GCM_FALSE);
	} RESC_BLOCK_END
}

static int32_t setRenderObject(void)
{
	static float phase = 0.f;

	//E transform
	float M[16];
	float P[16];
	float V[16];
	float VP[16];
	float MVP[16];

	phase += 0.05f;

	//E projection 
	buildProjection(P, -1.0f, 1.0f + sinf(phase), -1.0f, 1.0f, 1.0, 10000.0f);

	//E 16:9 scale
	matrixTranslate(V, 0.0f, 0.0f, -4.0);
	V[0*4 + 0] = 9.0f / 16.0f; 
	V[1*4 + 1] = 1.0f; 

	//E model view 
	matrixMul(VP, P, V);

	unitMatrix(M);
	matrixMul(MVP, VP, M);

	CGparameter model_view_projection 
		= cellGcmCgGetNamedParameter(vertex_program, "modelViewProj");
	CGparameter position 
		= cellGcmCgGetNamedParameter(vertex_program, "position");
	CGparameter color 
		= cellGcmCgGetNamedParameter(vertex_program, "color");
	//E get Vertex Attribute index
	uint32_t position_index 
		= cellGcmCgGetParameterResource(vertex_program, position) - CG_ATTR0;
	uint32_t color_index 
		= cellGcmCgGetParameterResource(vertex_program, color) - CG_ATTR0;

	cellGcmSetVertexProgramParameter(model_view_projection, MVP);

	cellGcmSetVertexProgram(vertex_program, vertex_program_ucode);

	//E fragment program offset
	uint32_t fragment_offset;
	if (cellGcmAddressToOffset(fragment_program_ucode, &fragment_offset) != CELL_OK)
		return -1;

	cellGcmSetFragmentProgram(fragment_program, fragment_offset);

	uint32_t vertex_offset[2];
	if (cellGcmAddressToOffset(&vertex_array_buffer->x, &vertex_offset[0]) != CELL_OK)
		return -1;
	if (cellGcmAddressToOffset(&vertex_array_buffer->rgba, &vertex_offset[1]) != CELL_OK)
		return -1;
	cellGcmSetVertexDataArray(position_index,
							  0, 
							  sizeof(Vertex_t), 
							  3, 
							  CELL_GCM_VERTEX_F, 
							  CELL_GCM_LOCATION_LOCAL, 
							  (uint32_t)vertex_offset[0]);
	cellGcmSetVertexDataArray(color_index,
							  0, 
							  sizeof(Vertex_t), 
							  4, 
							  CELL_GCM_VERTEX_UB, 
							  CELL_GCM_LOCATION_LOCAL, 
							  (uint32_t)vertex_offset[1]);
	return 0;
}


#define HOST_SIZE (1*1024*1024)
#define CB_SIZE	(0x10000)


RESC_BLOCK_BEGIN(6)
int32_t userMain(void)
{
	void *host_addr = memalign(1024*1024, HOST_SIZE);
	if (cellGcmInit(CB_SIZE, HOST_SIZE, host_addr) != CELL_OK) return -1;

	sFlipped=true;

	if (initLocalBuffer(RENDERING_BUF_WIDTH, RENDERING_BUF_HEIGHT))	return -1;

	initShader();

	initRenderObject();

	initResc();

	initExitRoutine();

	assert(sKeepRunning);
	//E rendering loop
	while (sKeepRunning) {
		//E check system event
		int ret = cellSysutilCheckCallback();
		if( ret != CELL_OK ) {
			printf( "cellSysutilCheckCallback() failed...: error=0x%x\n", ret );
		}

		setRenderTarget(0);

		setDrawEnv();

		if (setRenderObject())
			return -1;

		//E clear frame buffer
		cellGcmSetClearSurface(CELL_GCM_CLEAR_Z|
							   CELL_GCM_CLEAR_R|
							   CELL_GCM_CLEAR_G|
							   CELL_GCM_CLEAR_B|
							   CELL_GCM_CLEAR_A);

		//E set draw command
		cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_TRIANGLES, 0, 3);

		waitFlip();

		flip();
		
		//E for sample navigator
		if(cellSnaviUtilIsExitRequested(NULL)){
			break;
		}

		sKeepRunning = false;
	}

	//E Let PPU wait for all commands done (include waitFlip)
	cellGcmFinish(0);

	//E Terminate resc.
	cellRescExit();

	return 0;
}
RESC_BLOCK_END


//E Exit routines
void initExitRoutine(void)
{
	//E register exit callback
	int ret = atexit( programExitCallback );
	if( ret != 0 ) {
		printf( "Registering exit callback failed...\n" );
		exit(-1);
	}
	//E register sysutil exit callback
	ret = cellSysutilRegisterCallback(0, sysutilExitCallback, NULL);
	if( ret != CELL_OK ) {
		printf( "Registering sysutil callback failed...: error=0x%x\n", ret );
		exit(-1);
	}
}

//E this function is registered by atexit()
void programExitCallback(void)
{
	//E Let RSX wait for final flip
	//E Because libresc was unloaded, gcm's SetWaitFlip must be called.
	cellGcmSetWaitFlip();

	//E Let PPU wait for all commands done (include waitFlip)
	cellGcmFinish(1);
}

void sysutilExitCallback(uint64_t status, 
						 uint64_t param __attribute__ ((unused)), 
						 void* userdata __attribute__ ((unused)))
{
	switch(status) {
	case CELL_SYSUTIL_REQUEST_EXITGAME:
		sKeepRunning = false;	
		break;
	case CELL_SYSUTIL_DRAWING_BEGIN:
	case CELL_SYSUTIL_DRAWING_END:
		break;
	default:
		printf( ">> Unknown status: 0x%llx << \n", status );
	}
}

