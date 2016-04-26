/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#define __CELL_ASSERT__

#include <stdio.h>
#include <assert.h>
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
//#define USE_STATIC_TEXTURE

const float PI = 3.14159265358979f;
const float PI2 = PI + PI;

// instantiate the class
SampleApp app;

// shader
extern struct _CGprogram _binary_vpshader_vpo_start;
extern struct _CGprogram _binary_fpshader_fpo_start;
extern struct _CGprogram _binary_rtt_vpshader_vpo_start;
extern struct _CGprogram _binary_rtt_fpshader_fpo_start;

//-----------------------------------------------------------------------------
// Description: Constructor
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
SampleApp::SampleApp()
	: mTextureWidth(256), mTextureHeight(256), mTextureDepth(4),
#ifndef USE_STATIC_TEXTURE
// following 4 combinations of texture format are allowed for render to texture
//	  mTextureType(ARGB8), mEnableSwizzle(true), // only ARGB8 can be swizzled
	  mTextureType(ARGB8), mEnableSwizzle(false),
//	  mTextureType(FP16), mEnableSwizzle(false),
//	  mTextureType(FP32), mEnableSwizzle(false),
#else
// any combination is allowed when render to texture is not processed,
// and static texture is used for pass2
	  mTextureType(FP32), mEnableSwizzle(true),
#endif
	  mLabelValue(0)
{
	switch (mTextureType) {
	case ARGB8:
		mTextureDepth = 4; // unsigned byte
		break;
	case FP16:
		mTextureDepth = 4 * 2; // half float
		break;
	case FP32:
		mTextureDepth = 4 * sizeof(float); // float
		break;
	}
	mPitch = mEnableSwizzle ? 0 : mTextureWidth * mTextureDepth;
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
	mCGVertexProgram[0]   = &_binary_rtt_vpshader_vpo_start;
	mCGFragmentProgram[0] = &_binary_rtt_fpshader_fpo_start;
	mCGVertexProgram[1]   = &_binary_vpshader_vpo_start;
	mCGFragmentProgram[1] = &_binary_fpshader_fpo_start;

	for (uint32_t i = 0; i < 2; i++) { // there are two pass shaders
		// init
		cellGcmCgInitProgram(mCGVertexProgram[i]);
		cellGcmCgInitProgram(mCGFragmentProgram[i]);

		// allocate video memory for fragment program
		unsigned int ucodeSize;
		void *ucode;

		// get and copy 
		cellGcmCgGetUCode(mCGFragmentProgram[i], &ucode, &ucodeSize);

		mFragmentProgramUCode[i]
			= (void*)cellGcmUtilAllocateLocalMemory(ucodeSize, 64);
		CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mFragmentProgramUCode[i], &mFragmentProgramOffset[i]));

		memcpy(mFragmentProgramUCode[i], ucode, ucodeSize); 

		// get and copy 
		cellGcmCgGetUCode(mCGVertexProgram[i], &ucode, &ucodeSize);
		mVertexProgramUCode[i] = ucode;

		mModelViewProj[i]
			= cellGcmCgGetNamedParameter(mCGVertexProgram[i], "modelViewProj");
		CGparameter position
			= cellGcmCgGetNamedParameter(mCGVertexProgram[i], "position");
		CGparameter texcoord
			= cellGcmCgGetNamedParameter(mCGVertexProgram[i], "texcoord");
		CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(mModelViewProj[i]);
		CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(position);
		CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(texcoord);


		// get vertex attribute index
		mPosIndex[i] = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram[i], position) - CG_ATTR0);
		mTexIndex[i] = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram[i], texcoord) - CG_ATTR0);

		// set texture parameters
		CGparameter texture
			= cellGcmCgGetNamedParameter(mCGFragmentProgram[i], "texture");
		CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(texture);
		mTexUnit[i] = (CGresource)(cellGcmCgGetParameterResource(mCGFragmentProgram[i], texture) - CG_TEXUNIT0);
	}
}

void SampleApp::setTextureTarget()
{
	CellGcmSurface surface;

	memset(&surface, 0, sizeof(surface));

	switch (mTextureType) {
	case ARGB8:
		surface.colorFormat	 = CELL_GCM_SURFACE_A8R8G8B8;
		break;
	case FP16:
		surface.colorFormat	 = CELL_GCM_SURFACE_F_W16Z16Y16X16;
		assert(!mEnableSwizzle); // illegal combination of texture format
		break;
	case FP32:
		surface.colorFormat	 = CELL_GCM_SURFACE_F_W32Z32Y32X32;
		assert(!mEnableSwizzle); // illegal combination of texture format
		break;
	}

	surface.colorTarget		 = CELL_GCM_SURFACE_TARGET_0;
	surface.colorLocation[0] = CELL_GCM_LOCATION_LOCAL;
	surface.colorOffset[0]   = mRttColorOffset;

	// This must not be zero even if surface type is swizzle, don't use mPitch
	surface.colorPitch[0] 	 = mTextureWidth * mTextureDepth;

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
	surface.depthOffset 	= mRttDepthOffset;
	surface.depthPitch 		= mTextureWidth * 4;
	surface.type			= mEnableSwizzle ?
		CELL_GCM_SURFACE_SWIZZLE : CELL_GCM_SURFACE_PITCH;
	surface.width 			= mTextureWidth;
	surface.height 			= mTextureHeight;
	surface.x 		 		= 0;
	surface.y 		 		= 0;

	cellGcmSetSurface(&surface);
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
	mStateBufferAddress = (uint32_t*)memalign( 0x100000, STATE_BUFSIZE );
	CELL_GCMUTIL_ASSERTS(mStateBufferAddress != NULL,"memalign()");

	// map allocated buffer
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmMapMainMemory(mStateBufferAddress, STATE_BUFSIZE, &mStateBufferOffset));
	
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

	// label initialization
	uint32_t *mLabel = cellGcmGetLabelAddress(sLabelId);
	*mLabel = mLabelValue; // initial value: 0
	++mLabelValue;

	// buffer allocation
	mCubeVertices = (VertexData3D*)cellGcmUtilAllocateLocalMemory(sizeof(VertexData3D)*6*6, 128); // Cube
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mCubeVertices, &mCubeOffset));

	mQuadVertices = (VertexData2D*)cellGcmUtilAllocateLocalMemory(sizeof(VertexData2D)*4, 128); // Quad
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mQuadVertices, &mQuadOffset));

	mRttColorAddress = cellGcmUtilAllocateLocalMemory(mTextureWidth*mTextureHeight*mTextureDepth, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mRttColorAddress, &mRttColorOffset));

	mRttDepthAddress = cellGcmUtilAllocateLocalMemory(mTextureWidth*mTextureHeight*mTextureDepth, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mRttDepthAddress, &mRttDepthOffset));

	mTextureAddress = cellGcmUtilAllocateLocalMemory(mTextureWidth*mTextureHeight*4*sizeof(float), 128); // allocation for maximum float size
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mTextureAddress, &mTextureOffset));

	// build quad verts
	cellUtilGenerateQuad(mQuadVertices, 1.0f);
	// build cube verts
	mVertexCount = cellUtilGenerateCube(mCubeVertices, 1.0f);

	// static texture is always swizzled
	switch (mTextureType) {
	case ARGB8:
		if (mEnableSwizzle) {
			cellUtilGenerateSwizzledGradationTexture((uint8_t*)mTextureAddress,
													 mTextureWidth,
													 mTextureHeight,
													 mTextureDepth);
		}
		else {
			cellUtilGenerateGradationTexture((uint8_t*)mTextureAddress,
											 mTextureWidth, mTextureHeight,
											 mTextureDepth);
		}
		break;
	case FP16:
		if (mEnableSwizzle) {
			cellUtilGenerateSwizzledGradationTextureFP16((uint8_t*)mTextureAddress,
														 mTextureWidth,
														 mTextureHeight);
		}
		else {
			cellUtilGenerateGradationTextureFP16((uint8_t*)mTextureAddress,
												 mTextureWidth, mTextureHeight);
		}
		break;
	case FP32:
		if (mEnableSwizzle) {
			cellUtilGenerateSwizzledGradationTextureFP32((uint8_t*)mTextureAddress,
														 mTextureWidth,
														 mTextureHeight);
		}
		else {
			cellUtilGenerateGradationTextureFP32((uint8_t*)mTextureAddress,
												 mTextureWidth, mTextureHeight);
		}
		break;
	}

	// shader setup
	initShader();

	// initizalize command buffer for state 
	if(initStateBuffer() != true) return false;

#ifdef CELL_GCM_DEBUG // {
	gCellGcmDebugCallback = NULL;
#endif // }

	// inital state
	cellGcmSetCurrentBuffer(mStateBufferAddress, mStateBufferOffset);
	{
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
int frame = 0;
void SampleApp::onRender()
{
	if (frame == 2) {
		cellGcmFinish(0x13131313);
		exit(1);
	}
	frame++;
	// base implementation clears screen and sets up camera
	FWGCMCamControlApplication::onRender();

	// re-execute state commands created in onInit() 
	cellGcmSetCallCommand(mStateBufferOffset);

#ifndef USE_STATIC_TEXTURE
	// pass1: render to texture
	{
		setTextureTarget();

		// ser viewport for rendered texture
		float scale[4] = { mTextureWidth * 0.5f, mTextureHeight * -0.5f, 0.5f, 0.0f };
		float offset[4] = { scale[0], mTextureHeight + scale[1], 0.5f, 0.0f };

		cellGcmSetViewport(0, 0, mTextureWidth, mTextureHeight, 0.0f, 1.0f, scale, offset);
		cellGcmSetScissor(0, 0, mTextureWidth, mTextureHeight);

		cellGcmSetDepthTestEnable(CELL_GCM_FALSE);

		cellGcmSetVertexDataArray(mPosIndex[0], 0, sizeof(VertexData2D), 2,
								  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
								  mQuadOffset);
		cellGcmSetVertexDataArray(mTexIndex[0], 0, sizeof(VertexData2D), 2,
								  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
								  mQuadOffset+sizeof(float)*2);

		// bind texture
		uint32_t format, remap;
		switch (mTextureType) {
		case ARGB8:
			cellGcmUtilGetTextureAttribute(CELL_GCM_UTIL_ARGB8,
										   &format, &remap,
										   mEnableSwizzle ? 1 : 0, 1);
			break;
		case FP16:
			cellGcmUtilGetTextureAttribute(CELL_GCM_UTIL_FP16,
										   &format, &remap,
										   mEnableSwizzle ? 1 : 0, 1);
			break;
		case FP32:
			cellGcmUtilGetTextureAttribute(CELL_GCM_UTIL_FP32,
										   &format, &remap,
										   mEnableSwizzle ? 1 : 0, 1);
			break;
		}

		CellGcmTexture tex;
		tex.format = format;
		tex.mipmap = 1;
		tex.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
		tex.cubemap = CELL_GCM_FALSE;
		tex.remap = remap;
		tex.width = mTextureWidth;
		tex.height = mTextureHeight;
		tex.depth = 1;
		tex.pitch = mPitch;
		tex.location = CELL_GCM_LOCATION_LOCAL;
		tex.offset = mTextureOffset;
		cellGcmSetTexture(mTexUnit[0], &tex);

		// bind texture and set filter
		cellGcmSetTextureControl(mTexUnit[0], CELL_GCM_TRUE, 0<<8, 12<<8, CELL_GCM_TEXTURE_MAX_ANISO_1);
		cellGcmSetTextureAddress(mTexUnit[0],
								 CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
								 CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
								 CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
								 CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL,
								 CELL_GCM_TEXTURE_ZFUNC_LESS, 0);
		cellGcmSetTextureFilter(mTexUnit[0], 0,
								CELL_GCM_TEXTURE_NEAREST,
								CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);

		// bind Cg programs
		// NOTE: vertex program constants are copied here
		cellGcmSetVertexProgram(mCGVertexProgram[0], mVertexProgramUCode[0]);
		cellGcmSetFragmentProgram(mCGFragmentProgram[0], mFragmentProgramOffset[0]);

		// model matrix
		Matrix4 MVP = Matrix4::identity();

		// set MVP matrix
		cellGcmSetVertexProgramParameter(mModelViewProj[0], (float*)&MVP);
		cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_QUADS, 0, 4);
	}

	// wait for texture rendering to be done
	cellGcmSetWriteBackEndLabel(sLabelId, mLabelValue);
	cellGcmFlush();
	cellGcmSetWaitLabel(sLabelId, mLabelValue);
	++mLabelValue;
#endif

	// pass2: render to color surface
	{
		// set inital target
		FWCellGCMWindow::getInstance()->resetRenderTarget();

		// ser viewport for real display
		float scale[4] = {
			mDispInfo.mWidth * 0.5f, mDispInfo.mHeight * -0.5f, 0.5f, 0.0f
		};
		float offset[4] = {
			scale[0], mDispInfo.mHeight + scale[1], 0.5f, 0.0f
		};

		cellGcmSetViewport(0, 0, mDispInfo.mWidth, mDispInfo.mHeight,
						   0.0f, 1.0f, scale, offset);
		cellGcmSetScissor(0, 0, mDispInfo.mWidth, mDispInfo.mHeight);

		cellGcmSetDepthTestEnable(CELL_GCM_TRUE);

		cellGcmSetVertexDataArray(mPosIndex[1], 0, sizeof(VertexData3D), 3,
								  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
								  mCubeOffset);
		cellGcmSetVertexDataArray(mTexIndex[1], 0, sizeof(VertexData3D), 2,
								  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
								  mCubeOffset+sizeof(float)*4);

		// bind texture
		uint32_t format, remap;
		switch (mTextureType) {
		case ARGB8:
			cellGcmUtilGetTextureAttribute(CELL_GCM_UTIL_ARGB8,
										   &format, &remap,
										   mEnableSwizzle ? 1 : 0, 1);
			break;
		case FP16:
			cellGcmUtilGetTextureAttribute(CELL_GCM_UTIL_FP16,
										   &format, &remap,
										   mEnableSwizzle ? 1 : 0, 1);
			break;
		case FP32:
			cellGcmUtilGetTextureAttribute(CELL_GCM_UTIL_FP32,
										   &format, &remap,
										   mEnableSwizzle ? 1 : 0, 1);
			break;
		}

		CellGcmTexture tex;
		tex.format = format;
		tex.mipmap = 1;
		tex.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
		tex.cubemap = CELL_GCM_FALSE;
		tex.remap = remap;
		tex.width = mTextureWidth;
		tex.height = mTextureHeight;
		tex.depth = 1;
		tex.pitch = mPitch;
		tex.location = CELL_GCM_LOCATION_LOCAL;
#ifndef USE_STATIC_TEXTURE
		tex.offset = mRttColorOffset;
#else
		tex.offset = mTextureOffset;
#endif
		cellGcmSetTexture(mTexUnit[1], &tex);

		// bind texture and set filter
		cellGcmSetTextureControl(mTexUnit[1], CELL_GCM_TRUE, 1*0xff, 1*0xff, CELL_GCM_TEXTURE_MAX_ANISO_1);
		cellGcmSetTextureAddress(mTexUnit[1],
								 CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
								 CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
								 CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
								 CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL,
								 CELL_GCM_TEXTURE_ZFUNC_LESS, 0);
		cellGcmSetTextureFilter(mTexUnit[1], 0,
								CELL_GCM_TEXTURE_NEAREST,
								CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);

		cellGcmSetVertexProgram(mCGVertexProgram[1], mVertexProgramUCode[1]);
		cellGcmSetFragmentProgram(mCGFragmentProgram[1], mFragmentProgramOffset[1]);
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
		mat.setTranslation(Vector3(0.0f, 0.0f, -5.0f));

		// final matrix
		Matrix4 MVP = transpose(getProjectionMatrix() * getViewMatrix() * mat);

		// set MVP matrix
		cellGcmSetVertexProgramParameter(mModelViewProj[1], (float*)&MVP);
		cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_TRIANGLES, 0, mVertexCount);
	}

	{
		// calc fps
		FWTimeVal	time = FWTime::getCurrentTime();
		float fFPS = 1.f / (float)(time - mLastTime);
		mLastTime = time;

		// print some messages
		FWDebugFont::setPosition(0, 0);
		FWDebugFont::setColor(1.f, 1.f, 1.f, 1.0f);

		FWDebugFont::print("Render to Texture Sample Application\n\n");
		//FWDebugFont::printf("FPS: %.2f\n\n", fFPS);

		switch (mTextureType) {
		case ARGB8:
			FWDebugFont::printf("Texture Format: A8R8G8B8\n");
			break;
		case FP16:
			FWDebugFont::printf("Texture Format: F_W16Z16Y16X16\n");
			break;
		case FP32:
			FWDebugFont::printf("Texture Format: F_W32Z32Y32X32\n");
			break;
		}
		if (mEnableSwizzle) {
			FWDebugFont::printf("Texture Type:   Swizzled\n");
		}
		else {
			FWDebugFont::printf("Texture Type:   Linear\n");
		}
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
	FWGCMCamControlApplication::onShutdown();
}
