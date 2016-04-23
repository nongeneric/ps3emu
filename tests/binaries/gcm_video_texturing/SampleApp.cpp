/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#define __CELL_ASSERT__

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/timer.h>
#include <cell/gcm.h>

#include "SampleApp.h"
#include "FWCellGCMWindow.h"
#include "FWDebugFont.h"
#include "FWTime.h"
#include "cellutil.h"
#include "gcmutil.h"

#include "snaviutil.h"

using namespace cell::Gcm;

#define STATE_BUFSIZE (0x100000) // 1MB

// Texture size
#define TEXTURE_WIDTH  1980
#define TEXTURE_HEIGHT 1020

#define roundup(x, a) (((x)+((a)-1))&(~((a)-1)))

#define PI 3.14159265358979323846264f

#define CB_SIZE (16 << 20)    // Command Buffer Size
#define HOST_SIZE ((16 << 20) + CB_SIZE ) // Total size of system memory to map

// instantiate the class
SampleApp app;

// shader binary
extern struct _CGprogram _binary_vpshader_vpo_start;
extern struct _CGprogram _binary_fpshader_fpo_start;

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

	// get and copy fragment shader
	cellGcmCgGetUCode(mCGFragmentProgram, &ucode, &ucodeSize);
	mFragmentProgramUCode = (void*)cellGcmUtilAllocateLocalMemory(ucodeSize, 64);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mFragmentProgramUCode, &mFragmentProgramOffset));
	memcpy(mFragmentProgramUCode, ucode, ucodeSize);

	// get and copy vertex shader
	cellGcmCgGetUCode(mCGVertexProgram, &mVertexProgramUCode, &ucodeSize);
}

static inline void setColor( uint32_t* pixel, uint32_t color )
{
	*pixel = color;
}

static void createTextureMichenerCircle( uint32_t* buffer, 
			int32_t x, int32_t y, uint32_t hres, uint32_t vres, int32_t radius, 
			int color ) 
{
	uint32_t xcent = x; 
	uint32_t ycent = y;
	int32_t drop  = 3 - 2*radius;
	
	y = radius;
	x = 0;

	while( x <= y ) {

		// michener's algorithm to draw circle
		{{
			int	minx = 0, miny = 0, maxx = hres-1, maxy = vres-1;
			int	xlo, xhi, v, h;

			xlo = xcent-x, xhi = xcent+x;
			if ( xlo < minx ) xlo = minx;
			if ( xhi > maxx ) xhi = maxx;

			if ( ( v = ycent-y ) >= miny ) {
				for (h = xlo; h <= xhi; h++) 
					setColor(&buffer[ v*hres + h ], color);
			}
			if ( ( v = ycent+y ) <= maxy ) {
				for (h = xlo; h <= xhi; h++) 
					setColor(&buffer[ v*hres + h ], color);
			}

			xlo = xcent-y, xhi = xcent+y;
			if ( xlo < minx ) xlo = minx;
			if ( xhi > maxx ) xhi = maxx;

			if ( ( v = ycent-x ) >= miny ) {
				for (h = xlo; h <= xhi; h++) 
					setColor(&buffer[ v*hres + h ], color);
			}
			if ( ( v = ycent+x ) <= maxy ) {
				for (h = xlo; h <= xhi; h++) 
					setColor(&buffer[ v*hres + h ], color);
			}
		}}

		if( drop < 0 ) {
			drop += 4 * x + 6;
		}
		else {
			drop += 4 * (x-y) + 10;
			y--;
		}
		x++;
	}
}

void SampleApp::initCircle(void)
{
	srand( 43 );

	for( int i=0; i<NUM_CIRCLE; i++ ) {
		mCircle[i].x_center = rand() % TEXTURE_WIDTH;
		mCircle[i].y_center = rand() % TEXTURE_HEIGHT;
		mCircle[i].x_velocity = rand() % 10;
		mCircle[i].y_velocity = rand() % 10;
		mCircle[i].radius_max = (rand() % (TEXTURE_WIDTH/15))+TEXTURE_WIDTH/100;
		mCircle[i].radius     = mCircle[i].radius_max / 2;
		mCircle[i].scale      = 1;
		mCircle[i].color = ((rand() % 256)<<24 
		                 |  (rand() % 256)<<16
		                 |  (rand() % 256)<< 8
						 |  0xFF );
	}
}

void SampleApp::updateCircle(void)
{
	for( int i=0; i<NUM_CIRCLE; i++ ) {
		// position
		mCircle[i].x_center += mCircle[i].x_velocity;
		mCircle[i].y_center += mCircle[i].y_velocity;
		
		if( mCircle[i].x_center >= TEXTURE_WIDTH ) {
			mCircle[i].x_center = TEXTURE_WIDTH - 1;
			mCircle[i].x_velocity = -mCircle[i].x_velocity;
		}
		if( mCircle[i].y_center >= TEXTURE_HEIGHT ) {
			mCircle[i].y_center = TEXTURE_HEIGHT - 1;
			mCircle[i].y_velocity = -mCircle[i].y_velocity;
		}
		if( mCircle[i].x_center <= 0 ) {
			mCircle[i].x_center = 0;
			mCircle[i].x_velocity = -mCircle[i].x_velocity;
		}
		if( mCircle[i].y_center <= 0 ) {
			mCircle[i].y_center = 0;
			mCircle[i].y_velocity = -mCircle[i].y_velocity;
		}

		// size
		mCircle[i].radius += mCircle[i].scale;

		if( mCircle[i].radius >= mCircle[i].radius_max ||
		    mCircle[i].radius <= 1 ) 
		{
			mCircle[i].scale = -mCircle[i].scale;
		}
	}
}

void SampleApp::updateTexture( void )
{
	updateCircle();

	memset( mTextureAddress, 0x7F, TEXTURE_WIDTH*TEXTURE_HEIGHT*4 );
	for( int i=0; i<NUM_CIRCLE; i++ ) {
		createTextureMichenerCircle( (uint32_t*) mTextureAddress,
				mCircle[i].x_center, mCircle[i].y_center, 
				TEXTURE_WIDTH, TEXTURE_HEIGHT, mCircle[i].radius,
				mCircle[i].color );
	}
}

//-----------------------------------------------------------------------------
// Description: Constructor
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
SampleApp::SampleApp()
	 : mLabel( NULL ), mLabelValue( 0 )
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
	FWGCMCamControlApplication::onInit( argc, ppArgv );

	printf( "Begin Program!\n" );

	// initialize circle
	initCircle();

	// Create Vertex Buffer & send vertex data to vram
#define MINX -0.9f
#define MINY -0.9f
#define MAXX  0.9f
#define MAXY  0.9f
	VertexData3D vertices[4] = {
		  // vertex                 tex coord
		{ Point3(MINX,MINY,0.f),    0.f, 0.f },
		{ Point3(MAXX,MINY,0.f),    1.f, 0.f },
		{ Point3(MINX,MAXY,0.f),    0.f, 1.f },
		{ Point3(MAXX,MAXY,0.f),    1.f, 1.f }
	};
#undef MINX
#undef MINY
#undef MAXX
#undef MAXY

	// Initialize Main Memory
	uint32_t main_memory_size = roundup((TEXTURE_WIDTH*TEXTURE_HEIGHT*4 
	                          + sizeof(VertexData3D)*4), (1<<20));
	cellGcmUtilInitializeMainMemory( main_memory_size );

	mVertexBuffer = (VertexData3D*)cellGcmUtilAllocateMainMemory(sizeof(VertexData3D)*4, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset( (uint64_t *) mVertexBuffer, &mVertexBufferOffset ));
	memcpy( mVertexBuffer, vertices, sizeof(VertexData3D)*4 );

	// create texture buffer
	mTextureAddress = (void*)cellGcmUtilAllocateMainMemory(TEXTURE_WIDTH * TEXTURE_HEIGHT * 4, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset( (uint64_t *)mTextureAddress, &mTextureOffset ));

	// Reset Label
	mLabel = cellGcmGetLabelAddress(sLabelId);
	*mLabel = mLabelValue; // initial value: 0

	// Shader Setup
	initShader();
	
	// Get Cg Parameters
	CGparameter texture = cellGcmCgGetNamedParameter( mCGFragmentProgram, "texture" );
	mObjCoord      = cellGcmCgGetNamedParameter(mCGVertexProgram, "a2v.objCoord");
	mTexCoord      = cellGcmCgGetNamedParameter(mCGVertexProgram, "a2v.texCoord");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( texture );
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( mObjCoord );
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( mTexCoord );

	// Get attribute index
	mObjCoordIndex = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, mObjCoord) - CG_ATTR0);
	mTexCoordIndex = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, mTexCoord) - CG_ATTR0);
	mSampler = (CGresource)(cellGcmCgGetParameterResource( mCGFragmentProgram, texture) - CG_TEXUNIT0);

	// initizalize command buffer for state 
	if(initStateBuffer() != true) return false;

#ifdef CELL_GCM_DEBUG // {
	gCellGcmDebugCallback = NULL;
#endif // }

	// inital state
	cellGcmSetCurrentBuffer(mStateBufferAddress, FWCellGCMWindow::getInstance()->getStateCmdSize());
	{
		// Bind shaders
		cellGcmSetVertexProgram( mCGVertexProgram, mVertexProgramUCode);
		cellGcmSetFragmentProgram( mCGFragmentProgram, mFragmentProgramOffset );

		// Bind Texture
		mTexture.format = CELL_GCM_TEXTURE_A8R8G8B8 
						| CELL_GCM_TEXTURE_LN 
						| CELL_GCM_TEXTURE_NR; 
		mTexture.mipmap = 1;
		mTexture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
		mTexture.cubemap = CELL_GCM_FALSE;
		mTexture.remap =  CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
					 CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
					 CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
					 CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
					 CELL_GCM_TEXTURE_REMAP_FROM_G << 6 |
					 CELL_GCM_TEXTURE_REMAP_FROM_R << 4 |
					 CELL_GCM_TEXTURE_REMAP_FROM_A << 2 |
					 CELL_GCM_TEXTURE_REMAP_FROM_B;

		mTexture.width  = TEXTURE_WIDTH;
		mTexture.height = TEXTURE_HEIGHT;
		mTexture.depth  = 1;
		mTexture.pitch = TEXTURE_WIDTH * 4;
		mTexture.location = CELL_GCM_LOCATION_MAIN;
		mTexture.offset = mTextureOffset;
		cellGcmSetTexture( mSampler, &mTexture );
		
		// bind texture and set filter
		cellGcmSetTextureControl( mSampler, CELL_GCM_TRUE, 0<<8, 12<<8, CELL_GCM_TEXTURE_MAX_ANISO_1);
		cellGcmSetTextureAddress( mSampler, 
								  CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
								  CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
								  CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
								  CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL, 
								  CELL_GCM_TEXTURE_ZFUNC_LESS, 0);
		cellGcmSetTextureFilter( mSampler, 0,
								 CELL_GCM_TEXTURE_NEAREST,
								 CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);
		// State setting
		cellGcmSetDepthFunc(CELL_GCM_ALWAYS);
		cellGcmSetDepthTestEnable( CELL_GCM_TRUE );
		cellGcmSetShadeMode(CELL_GCM_SMOOTH);

		cellGcmSetFlipMode( CELL_GCM_DISPLAY_VSYNC );

		// Need to put Return command because this command buffer is called statically
		cellGcmSetReturnCommand();
	}
	// Get back to default command buffer
	cellGcmSetDefaultCommandBuffer();

#ifdef CELL_GCM_DEBUG // {
	gCellGcmDebugCallback = cellGcmDebugFinish;
#endif // }

	return true;
}

//-----------------------------------------------------------------------------
// Description: Check Input
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
bool SampleApp::onUpdate()
{
	if(FWGCMCamControlApplication::onUpdate() == false) return false;
	
	// for sample navigator
	FWInputDevice *pPad = FWInput::getDevice(FWInput::DeviceType_Pad, 0);
	if(pPad != NULL){
		if(cellSnaviUtilIsExitRequested(&pPad->padData)){
			return false;
		}
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Description: Render callback
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
int frames = 0;
void SampleApp::onRender()
{
	if (frames) {
		cellGcmFinish(0x13131313);
		exit(0);
	}
	frames++;
	FWGCMCamControlApplication::onRender();

	// re-execute state commands created in onInit() 
	//  - this will set texture.
	//
	cellGcmSetCallCommand(mStateBufferOffset);

	while (*((volatile uint32_t *)mLabel) != mLabelValue) {
		sys_timer_usleep(10);
	}
	mLabelValue++;

	// create/update texture image at every frame
	// 
	updateTexture();

	// Attribute pointers
	cellGcmSetVertexDataArray(mObjCoordIndex, 0, sizeof(VertexData3D), 3, 
	                          CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, 
							  mVertexBufferOffset );
	// Note: tc offset needs to be 4 because it uses VMX 128 bit data type
	cellGcmSetVertexDataArray(mTexCoordIndex, 0, sizeof(VertexData3D), 2, 
	                          CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, 
							  mVertexBufferOffset + sizeof(Point3) ); 

	// Texture cache
	cellGcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_TEXTURE);

	// Draw kick
	cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_TRIANGLE_STRIP,0,4);

	// label command executed after ROP completion
	cellGcmSetWriteBackEndLabel(sLabelId, mLabelValue);

	// flush label command
	cellGcmFlush();


}
