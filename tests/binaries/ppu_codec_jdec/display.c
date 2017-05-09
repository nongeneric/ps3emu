/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cell/gcm.h>
#include <sys/timer.h>
#include <cell/error.h>
#include <cell/sysmodule.h>

#include <sysutil/sysutil_sysparam.h> 

#include "display.h"

#define BUF_NUM 2

#define END_LABEL_ID 128

#define CODEC_SAMPLE_SYSUTIL_CALLBACK_SLOT_EXITGAME 0

#define MODE_RUN  0
#define MODE_EXIT 1

#define ROUNDUP(x, a) (((x)+((a)-1))&(~((a)-1)))

/*E vertex structure. */
typedef struct {
	float px, py, pz;
	float tx, ty;
} my_vertex;

/*E user defined size. */
static uint32_t g_width;
static uint32_t g_height;

static uint32_t g_color_pitch;
static uint32_t g_depth_pitch;

static uint32_t g_color_offs[BUF_NUM];
static uint32_t g_depth_offs;

static my_vertex *g_vertex_buf;
static uint32_t g_vertex_buf_offs;

static uint32_t g_obj_coord_idx;
static uint32_t g_tex_coord_idx;

static uint32_t g_frame_idx;
static uintptr_t g_vram_heap;

static CGresource g_sampler;
static CGprogram g_vertex_prg;
static CGprogram g_fragment_prg;
static CellGcmTexture g_tex_param;

static void *g_vertex_prg_ucode;
static void *g_fragment_prg_ucode;
static uint32_t g_fragment_prg_offs;

static int g_mode_flag;

/*E shader binary */
extern struct _CGprogram _binary_vpshader_vpo_start;
extern struct _CGprogram _binary_fpshader_fpo_start;

static void _set_context_param( void );

static inline void*
_vheap_alloc( uint32_t size )
{
	void *base;
	size += (size+1023)&(~1023);
	base = (void*)g_vram_heap;
	g_vram_heap += size;
	return base;
}

static inline void
_vheap_align( uint32_t align )
{
	if(align%16){
		fprintf( stderr, "invalid alignment value.\n" );
		return;
	}
	--align;
	g_vram_heap =
		(g_vram_heap+align)&(~align);
}

static void
_init_shader( void )
{

	void *ucode;
	uint32_t ucode_size;

	g_vertex_prg = &_binary_vpshader_vpo_start;
	g_fragment_prg = &_binary_fpshader_fpo_start;

	/*E init */
	cellGcmCgInitProgram( g_vertex_prg );
	cellGcmCgInitProgram( g_fragment_prg );

	/*E get and copy fragment shader */
	_vheap_align( 64 );
	cellGcmCgGetUCode( g_fragment_prg,
					   &ucode, &ucode_size );

	/*E allocate video memory for fragment program */
	g_fragment_prg_ucode = _vheap_alloc( ucode_size );

	cellGcmAddressToOffset( g_fragment_prg_ucode,
							&g_fragment_prg_offs );

	memcpy( g_fragment_prg_ucode,
			ucode, ucode_size );

	/*E get and copy vertex shader */
	cellGcmCgGetUCode( g_vertex_prg,
					   &g_vertex_prg_ucode,
					   &ucode_size );

}

static void
_set_render_tgt( const uint32_t index )
{

	CellGcmSurface surface;

	surface.colorFormat      = CELL_GCM_SURFACE_A8R8G8B8;
	surface.colorTarget      = CELL_GCM_SURFACE_TARGET_0;
	
	surface.colorLocation[0] = CELL_GCM_LOCATION_LOCAL;
	surface.colorOffset[0]   = (uint32_t)g_color_offs[index];
	surface.colorPitch[0]    = g_color_pitch;

	surface.colorLocation[1] = CELL_GCM_LOCATION_LOCAL;
	surface.colorOffset[1]   = 0;
	surface.colorPitch[1]    = 64;

	surface.colorLocation[2] = CELL_GCM_LOCATION_LOCAL;
	surface.colorOffset[2]   = 0;
	surface.colorPitch[2]    = 64;

	surface.colorLocation[3] = CELL_GCM_LOCATION_LOCAL;
	surface.colorOffset[3]   = 0;
	surface.colorPitch[3]    = 64;

	surface.depthFormat      = CELL_GCM_SURFACE_Z24S8;
	surface.depthLocation    = CELL_GCM_LOCATION_LOCAL;
	surface.depthOffset      = g_depth_offs;
	surface.depthPitch       = g_depth_pitch;
	surface.type             = CELL_GCM_SURFACE_PITCH;
	surface.antialias        = CELL_GCM_SURFACE_CENTER_1;
	surface.x                = 0;
	surface.y                = 0;
	surface.width            = g_width;
	surface.height           = g_height;

	cellGcmSetSurface( gCellGcmCurrentContext, &surface );

}

static void
_get_gpu_sync(void)
{
	static uint32_t *label;
	static int32_t first = 1;
	static uint32_t cpuCounter=0;

	if ( 1 == first ) {
		/*E initialize GPU-progress counter. */
		label = cellGcmGetLabelAddress( END_LABEL_ID );
		*label = 0;
	}

	cellGcmSetWriteBackEndLabel( gCellGcmCurrentContext, 
	                             END_LABEL_ID,
								 cpuCounter              );

	cellGcmFlush( gCellGcmCurrentContext );

	/*E not to let CPU work too fast */
	/*E this code would not work when 32bit counter overflows */
	while ( cpuCounter > *((volatile uint32_t*) label) + 1 ) {
		sys_timer_usleep(100);
	}

	cpuCounter++;

	first = 0;
}

static void
_buffer_flip(void)
{

	static int first = 1;

	/*E triple buffering. */
	if( 2 < BUF_NUM ){
		/*E wait real flip before starting the draw after the next. */
		if( 1 != first ){
			cellGcmSetWaitFlip( gCellGcmCurrentContext );
		}else{
			first = 0;
		}
	}

	if( cellGcmSetFlip( gCellGcmCurrentContext, g_frame_idx ) != CELL_OK ) 
		return;

	cellGcmFlush( gCellGcmCurrentContext );
	
	/*E double buffering. */
	if ( 2 == BUF_NUM ){
		/*E wait real flip before starting the next draw. */
		cellGcmSetWaitFlip( gCellGcmCurrentContext );
	}
	
	_get_gpu_sync();

	/*E new render target */
	g_frame_idx = ( g_frame_idx+1 ) % BUF_NUM;
	_set_render_tgt( g_frame_idx );

	first = 0;
}

static void cb_exitgame( uint64_t status,
						 uint64_t param,
						 void*    userdata )
{
	(void)param;
	(void)userdata;

	switch ( status ) {
	case CELL_SYSUTIL_REQUEST_EXITGAME:
		printf( "CELL_SYSUTIL_REQUEST_EXITGAME\n" );
		g_mode_flag = MODE_EXIT;
		break;
	default:
		printf( "CellSysutilCallback:status 0x%Lx\n", status );
		break;
	}

}

int32_t my_disp_create( uint32_t xsize,
						uint32_t ysize )
{

	int32_t i_cnt;
	int32_t sys_ret;
	int32_t gcm_ret;
	int32_t prx_ret;

	void *gcm_buf;
	void *depth_addr;
	void *color_base_addr;
	void *color_addr[BUF_NUM];

	uint32_t color_size;
	uint32_t color_limit;
	uint32_t depth_size;
	uint32_t depth_limit;
	uint32_t buffer_width;

	CellVideoOutState state;
	CellVideoOutResolution resolution;

	/*E load libgcm_sys */
	prx_ret = cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	if( prx_ret < CELL_OK ){
		fprintf( stderr, "disp_create> cellSysmoduleLoadModule failed ... %#x\n", prx_ret );
		return -1;
	}

	sys_ret = cellVideoOutGetState( CELL_VIDEO_OUT_PRIMARY, 0, &state );
	if( sys_ret != CELL_OK ){
		fprintf( stderr, "disp_create> cellVideoOutGetState failed ... %#x\n", sys_ret );
	}

	sys_ret = cellVideoOutGetResolution( state.displayMode.resolutionId, &resolution );
	if( sys_ret != CELL_OK ){
		fprintf( stderr, "disp_create> cellVideoOutGetResolution failed ... %#x\n", sys_ret );
	}

	if( xsize > resolution.width ||
		ysize > resolution.height ){
		fprintf( stderr, "disp_create> !!WARNING!! NOT ENOUGH DISPLAY SIZE.\n" );
		fprintf( stderr, "disp_create> !!WARNING!! actual size:%dx%d < requested size:%dx%d\n",
				 resolution.width, resolution.height, xsize, ysize );
		fprintf( stderr, "disp_create> !!WARNING!! please reconfirm your monitor configuration.\n" );
		return -1;
	}

	g_width = resolution.width;
	g_height = resolution.height;

	buffer_width = ROUNDUP( g_width, 64 );
	gcm_buf = memalign( 0x100000, 0x100000 );

	/*E initialize the gcm. */
	gcm_ret = cellGcmInit( 0x100000 - 0x10000, 0x100000, gcm_buf );

	if( CELL_OK != gcm_ret ){
		fprintf( stderr, "disp_create> cellGcmInit failed ... %d\n", gcm_ret );	
		return gcm_ret;
	}

	g_mode_flag = MODE_RUN;

	gcm_ret = cellSysutilRegisterCallback( CODEC_SAMPLE_SYSUTIL_CALLBACK_SLOT_EXITGAME, cb_exitgame, NULL );
	if ( gcm_ret != CELL_OK ) {
		fprintf( stderr, "disp_create> cellSysutilRegisterCallback failed ... %#x\n", gcm_ret );
		return gcm_ret;
	}

	g_color_pitch =
		cellGcmGetTiledPitchSize( buffer_width*4 );
	if( 0 == g_color_pitch ){
		fprintf( stderr, "disp_create> cellGcmGetTiledPitchSize failed ...\n" );	
		return -1;
	}
	
	g_depth_pitch =
		cellGcmGetTiledPitchSize( buffer_width*4 );
	if( 0 == g_depth_pitch ){
		fprintf( stderr, "disp_create> cellGcmGetTiledPitchSize failed ...\n" );	
		return -1;
	}
	
	/*E color buffer settings. */
	color_size = g_color_pitch*ROUNDUP( g_height, 64 );
	color_limit = ROUNDUP( BUF_NUM*color_size,
						   0x10000 );
	
	/*E depth buffer settings. */
	depth_size = g_depth_pitch*ROUNDUP( g_height, 64 );
	depth_limit = ROUNDUP( depth_size,
						   0x10000 );

	CellVideoOutConfiguration config;
	memset( &config, 0, sizeof(CellVideoOutConfiguration) );
	config.pitch = g_color_pitch;
	config.resolutionId = state.displayMode.resolutionId;
	config.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;

	sys_ret = cellVideoOutConfigure( CELL_VIDEO_OUT_PRIMARY, &config, NULL, 0 );
	if( CELL_OK != sys_ret ){
		fprintf( stderr, "disp_create> cellVideoOutConfigure failed ... %d\n", sys_ret );
		return sys_ret;
	}

	cellGcmSetFlipMode( CELL_GCM_DISPLAY_VSYNC );

	/*E get GPU configuration. */
	CellGcmConfig gcm_cfg;
	cellGcmGetConfiguration( &gcm_cfg );
	g_vram_heap = (uintptr_t)gcm_cfg.localAddress;
	
	/*E allocate color buffers. */
	_vheap_align( 0x10000 );
	color_base_addr = _vheap_alloc( color_limit );

	for( i_cnt=0; i_cnt<BUF_NUM; i_cnt++ ){
		color_addr[i_cnt] = (void*)
			((uintptr_t)color_base_addr + (i_cnt*color_size));
		cellGcmAddressToOffset( color_addr[i_cnt],
								&g_color_offs[i_cnt] );
	}
	
	/*E allocate depth buffers. */
	_vheap_align( 0x10000 );

	depth_addr = _vheap_alloc( depth_limit );

	cellGcmAddressToOffset( depth_addr,
							&g_depth_offs );
	
	/*E set tiled region. */
	cellGcmSetTile( 0, CELL_GCM_LOCATION_LOCAL,
					g_color_offs[0], color_limit, g_color_pitch,
					CELL_GCM_COMPMODE_DISABLED, 0, 0 );
	
	cellGcmSetTile( 1, CELL_GCM_LOCATION_LOCAL,
					g_depth_offs, depth_limit, g_depth_pitch,
					CELL_GCM_COMPMODE_DISABLED, 0, 0 );
	
	cellGcmSetZcull( 0, g_depth_offs, buffer_width,
					 ROUNDUP(g_height, 64),
					 0, CELL_GCM_ZCULL_Z24S8,
					 CELL_GCM_SURFACE_CENTER_1,
					 CELL_GCM_LESS, CELL_GCM_ZCULL_LONES,
					 CELL_GCM_LESS, 0x80, 0xff );
	
	/*E regist surface */
	for( i_cnt=0; i_cnt<BUF_NUM; i_cnt++ ){
		gcm_ret = cellGcmSetDisplayBuffer( i_cnt,
										   g_color_offs[i_cnt],
										   g_color_pitch,
										   g_width,
										   g_height );
		if( CELL_OK != gcm_ret ) return -1;
	}
	
	/*E set initial target */
	_set_render_tgt( 0 );

	/*E load shaders. */
	_init_shader();

	_vheap_align( 512*1024 );

	/*E create vertex buffer & send vertex data to vram. */
	my_vertex vertices[4] = {
		/*E vertex           tex coord */
		{ -1.f, -1.f, 0.f,  0.f, 1.f },
		{  1.f, -1.f, 0.f,  1.f, 1.f },
		{ -1.f,  1.f, 0.f,  0.f, 0.f },
		{  1.f,  1.f, 0.f,  1.f, 0.f }
	};

	/*E create small vertex buffer & send small vertex data to vram. */
	g_vertex_buf = (my_vertex*)_vheap_alloc( sizeof(my_vertex)*4 );

	memcpy( g_vertex_buf, vertices, sizeof(my_vertex)*4 );

	cellGcmAddressToOffset( (void*)g_vertex_buf,
							&g_vertex_buf_offs );

	/*E tex parameter */
	g_tex_param.format  = CELL_GCM_TEXTURE_A8R8G8B8;
	/*E if it's not swizzle. */
	g_tex_param.format |= CELL_GCM_TEXTURE_LN;

	g_tex_param.remap =
		CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
		CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
		CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
		CELL_GCM_TEXTURE_REMAP_REMAP <<  8 |
		CELL_GCM_TEXTURE_REMAP_FROM_G << 6 |
		CELL_GCM_TEXTURE_REMAP_FROM_R << 4 |
		CELL_GCM_TEXTURE_REMAP_FROM_A << 2 |
		CELL_GCM_TEXTURE_REMAP_FROM_B;

	g_tex_param.mipmap = 1;
	g_tex_param.cubemap = CELL_GCM_FALSE;
	g_tex_param.dimension = CELL_GCM_TEXTURE_DIMENSION_2;

	/*E Get Cg Parameters */
	CGparameter objCoord =
		cellGcmCgGetNamedParameter( g_vertex_prg,
									"a2v.objCoord" );
	if( 0 == objCoord ){
		fprintf( stderr, "disp_create> cellGcmCgGetNamedParameter... fail\n" );
		return -1;
	}

	CGparameter texCoord =
		cellGcmCgGetNamedParameter( g_vertex_prg,
									"a2v.texCoord" );
	if( 0 == texCoord ){
		fprintf( stderr, "disp_create> cellGcmCgGetNamedParameter... fail\n" );
		return -1;
	}

	CGparameter texture =
		cellGcmCgGetNamedParameter( g_fragment_prg,
									"texture" );
	if( 0 == texture ){
		fprintf( stderr, "disp_create> cellGcmCgGetNamedParameter... fail\n" );
		return -1;
	}
	
	/*E Get attribute index */
	g_obj_coord_idx =
		( cellGcmCgGetParameterResource( g_vertex_prg,
										 objCoord) - CG_ATTR0 );
	g_tex_coord_idx =
		( cellGcmCgGetParameterResource( g_vertex_prg,
										 texCoord) - CG_ATTR0 );
	g_sampler = (CGresource)
		( cellGcmCgGetParameterResource( g_fragment_prg,
										 texture ) - CG_TEXUNIT0 );

	return 0;

}

/*E map the buffer. */
int32_t my_disp_mapmem( uint8_t *buffer,
						size_t buf_size,
						uint32_t *buf_offset )
{
	int32_t gcm_ret;

	gcm_ret = cellGcmMapMainMemory( buffer,
									buf_size,
									buf_offset );
	if( CELL_OK != gcm_ret ){
		fprintf( stderr, "disp_mapmem> cellGcmMapMainMemory... fail %d\n",
				gcm_ret );
		return gcm_ret;
	}

	return 0;

}

/*E unmap the buffer. */
int32_t my_disp_unmapmem( uint32_t buf_offset )
{
	int32_t gcm_ret;
	gcm_ret = cellGcmUnmapIoAddress( buf_offset );
	if( CELL_OK != gcm_ret ){
		fprintf( stderr, "disp_mapmem> cellGcmUnmapEaIoAddress... fail %d\n",
				gcm_ret );
		return gcm_ret;
	}
	return 0;
}


int32_t my_disp_settex( uint8_t *buffer,
						uint32_t xsize,
						uint32_t ysize )
{

	int32_t gcm_ret;
	uint32_t buf_offs;

	float min_x = -1.f*(float)xsize/(float)g_width;
	float min_y = -1.f*(float)ysize/(float)g_height;
	float max_x =  1.f*(float)xsize/(float)g_width;
	float max_y =  1.f*(float)ysize/(float)g_height;

	g_vertex_buf[0].px = min_x;
	g_vertex_buf[0].py = min_y;
	g_vertex_buf[1].px = max_x;
	g_vertex_buf[1].py = min_y;
	g_vertex_buf[2].px = min_x;
	g_vertex_buf[2].py = max_y;
	g_vertex_buf[3].px = max_x;
	g_vertex_buf[3].py = max_y;

	gcm_ret = cellGcmAddressToOffset( buffer, &buf_offs );
	if( CELL_OK != gcm_ret ){
		fprintf( stderr, "disp_settex> cellGcmAddressToOffset... fail %d\n",
				gcm_ret );
		return gcm_ret;
	}

	g_tex_param.depth  = 1;
	g_tex_param.width  = xsize;
	g_tex_param.height = ysize;
	g_tex_param.pitch  = xsize*4;
	g_tex_param.offset = buf_offs;
	g_tex_param.location = CELL_GCM_LOCATION_MAIN;

	/*E set texture. */
	cellGcmSetTexture( gCellGcmCurrentContext, g_sampler, &g_tex_param );

	/*E bind texture and set filter */
	cellGcmSetTextureControl( gCellGcmCurrentContext, g_sampler, 1, 0, 15, 1 );

	cellGcmSetTextureAddress( gCellGcmCurrentContext,
	                          g_sampler,
	                          CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
	                          CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
	                          CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
							  CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL, 
							  CELL_GCM_TEXTURE_ZFUNC_LESS, 0 );

	cellGcmSetTextureFilter( gCellGcmCurrentContext,
	                         g_sampler, 0,
	                         CELL_GCM_TEXTURE_NEAREST,
							 CELL_GCM_TEXTURE_NEAREST,
							 CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX );

	return gcm_ret;

}

static void _set_context_param( void )
{
	/*E setup viewport & clear color */
	float scale[4] =  { g_width*0.5f, g_height*(-0.5f), 0.5f, 0.0f };
	float offset[4] = { scale[0], g_height + scale[1], 0.5f, 0.0f };
	
	cellGcmSetViewport( gCellGcmCurrentContext,
	                    0, 0,
						g_width,
						g_height,
						0.0f, 1.0f,
						scale, offset );
	
	cellGcmSetScissor( gCellGcmCurrentContext,
	                   0, 0,
					   g_width,
					   g_height );
	
	cellGcmSetClearColor( gCellGcmCurrentContext, 0 );

	/*E Bind shaders */
	cellGcmSetVertexProgram( gCellGcmCurrentContext,
	                         g_vertex_prg,
							 g_vertex_prg_ucode );
	cellGcmSetFragmentProgram( gCellGcmCurrentContext,
	                           g_fragment_prg,
							   g_fragment_prg_offs );

	/*E state setting */
	cellGcmSetDepthFunc( gCellGcmCurrentContext, CELL_GCM_LESS );
	cellGcmSetDepthTestEnable( gCellGcmCurrentContext, CELL_GCM_TRUE );
	cellGcmSetShadeMode( gCellGcmCurrentContext, CELL_GCM_SMOOTH );
	
	return;
}

int my_disp_display( void )
{
	int32_t ret;
	_set_context_param();

	/*E clear surface. */
	cellGcmSetClearSurface( gCellGcmCurrentContext,
	                        CELL_GCM_CLEAR_R |
							CELL_GCM_CLEAR_G |
							CELL_GCM_CLEAR_B |
							CELL_GCM_CLEAR_A |
							CELL_GCM_CLEAR_Z        );

	/*E invalidate texture chache. */
	cellGcmSetInvalidateTextureCache( gCellGcmCurrentContext, 
	                                   CELL_GCM_INVALIDATE_TEXTURE );
	
	/*E attribute pointers. */
	cellGcmSetVertexDataArray( gCellGcmCurrentContext,
	                           g_obj_coord_idx, 0,
							   sizeof(my_vertex), 3,
							   CELL_GCM_VERTEX_F,
							   CELL_GCM_LOCATION_LOCAL,
							   g_vertex_buf_offs );

	cellGcmSetVertexDataArray( gCellGcmCurrentContext,
	                           g_tex_coord_idx, 0,
	                           sizeof(my_vertex), 2,
	                           CELL_GCM_VERTEX_F,
	                           CELL_GCM_LOCATION_LOCAL,
	                           ( g_vertex_buf_offs +
	                             sizeof(float)*3 ) );

	/*E kick drawing. */
	cellGcmSetDrawArrays( gCellGcmCurrentContext,
	                      CELL_GCM_PRIMITIVE_TRIANGLE_STRIP,
						  0, 4 );
	
	/*E swap buffers. */
	_buffer_flip();

	/*E check callback status */
	ret = cellSysutilCheckCallback();
	if ( ret != CELL_OK ) {
		fprintf( stdout, "disp_display> cellSysutilCheckCallback ... %#x\n", ret );
		return ret;
	}

	if ( g_mode_flag == MODE_EXIT ) {
		my_disp_destroy();
		return 1;
	}

	return 0;
}


void my_disp_destroy( void )
{
	int32_t  ret;

	cellGcmFinish( gCellGcmCurrentContext, 0 );
	fprintf( stdout, "disp_destroy> cellGcmFinish done ...\n" );

	/*E unregist sysutil callback function */
	ret = cellSysutilUnregisterCallback( CODEC_SAMPLE_SYSUTIL_CALLBACK_SLOT_EXITGAME );
	if ( ret != CELL_OK )
		fprintf( stdout, "disp_destroy> cellSysutilUnregisterCallback ... %#x\n", ret );

	/*E unload libgcm_sys */
	ret = cellSysmoduleUnloadModule( CELL_SYSMODULE_GCM_SYS );
	
	if( ret != CELL_OK )
		fprintf( stdout, "disp_destroy> cellSysmoduleUnloadModule ... %#x\n", ret );
}
