/*   SCE CONFIDENTIAL                                       */ 
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#define __CELL_ASSERT__

#include <assert.h>
#include <stdio.h>
#include <sys/sys_time.h>
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

#define PRIMITIVE_RESTART_INDEX (0x7FFFFFFF)
#define FAULT_LINE_ITERATION (300)

// instantiate the class
SampleApp app;

const float SampleApp::sDeadTime  = 0.3f;

// shader
extern struct _CGprogram _binary_vpshader_vpo_start;
extern struct _CGprogram _binary_fpshader_fpo_start;


//-----------------------------------------------------------------------------
// 
//  Helper function to generate vertices, texture coordinates, and indices.
// 
//-----------------------------------------------------------------------------
static uint32_t GeneratePlane(VertexData3D *point, uint32_t *indices, const float size,
							   const uint32_t row, const uint32_t col)
{
	uint32_t index_count=0;
	float xoffset, zoffset;

	for (uint32_t j = 0; j < col; ++j) {
		zoffset =  -size*col/2 + size*j;

		for (uint32_t i = 0; i < (row+1); ++i) {
			xoffset = -size*row/2 + size*i;

			// Vertices
			point[(j*(row+1)+i)*2].pos
				= Point3( xoffset, 0.f, zoffset );
			point[(j*(row+1)+i)*2+1].pos
				= Point3( xoffset, 0.f, zoffset+size );

			// Texture coordinate
			point[(j*(row+1)+i)*2].u = (float)i/(float)row;   
			point[(j*(row+1)+i)*2].v = (float)j/(float)col;

			point[(j*(row+1)+i)*2+1].u = (float)i/(float)row;   
			point[(j*(row+1)+i)*2+1].v = (float)(j+1)/(float)col;
		}
	}
	// generate indices with primitive restart
	for (uint32_t j = 0; j < col; ++j) {
		for (uint32_t i = 0; i < (row+1)*2; ++i) {
			// Indices
			indices[index_count++] = (j*(row+1)*2)+i;
		}
		indices[index_count++] = PRIMITIVE_RESTART_INDEX;
	}

	uint8_t* bs = (uint8_t*)point;
	uint32_t sum = 0;
	for (unsigned i = 0; i < index_count; ++i) {
		sum += bs[i];
	}
	printf("vertices sum: %d\n", sum);

	return index_count;
}

//-----------------------------------------------------------------------------
// Description: Constructor
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
SampleApp::SampleApp()
	: mCameraDefaultPos(Point3(0.f, 1.f, (float)sColumn / 2)),
	  mCameraDefaultTilt(1.3f), mCameraDefaultPan(0.0f),
	  mDrawPrimitive( CELL_GCM_PRIMITIVE_TRIANGLE_STRIP ),
	  mAnimationParameter( 0.f, 0.f, 0.f, 0.f ),
	  mHeightFactor( 1.07f ), mIsTexSingleComponent( false ),
	  mAnimate( false )
{
	
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

	// get and copy
	cellGcmCgGetUCode(mCGFragmentProgram, &ucode, &ucodeSize);

	mFragmentProgramUCode
		= (void*)cellGcmUtilAllocateLocalMemory(ucodeSize, 64);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mFragmentProgramUCode, &mFragmentProgramOffset));

	memcpy(mFragmentProgramUCode, ucode, ucodeSize); 

	// get and copy
	cellGcmCgGetUCode(mCGVertexProgram, &ucode, &ucodeSize);
	mVertexProgramUCode = ucode;
}

// random value between 0.f to 1.f
#define GET_RAND() ((float)rand()/(float)RAND_MAX)

void SampleApp::createFloatHeightTexture( void )
{
	float* texture;
	srand(7);

	// zero clear
	memset(mTextureAddress, 0x0, sRow*sColumn*sizeof(float)*(mIsTexSingleComponent? 1:4));

	texture = (float*)mTextureAddress;

	for( int i=0; i<FAULT_LINE_ITERATION; i++ ) {
		// get two random point
		int32_t  x1 = (int32_t)(GET_RAND() * (float)sColumn);
		int32_t  y1 = (int32_t)(GET_RAND() * (float)sRow);
		int32_t  x2 = (int32_t)(GET_RAND() * (float)sColumn);
		int32_t  y2 = (int32_t)(GET_RAND() * (float)sRow);

		int32_t ax = x2 - x1;
		int32_t ay = y2 - y1;

		for( int32_t r=0; r<(int32_t)sRow; r++ ) {
			for( int32_t c=0; c<(int32_t)sColumn; c++ ) {
				
				int32_t bx = x2 - c;
				int32_t by = y2 - r;

				if( (ay * bx) - (ax * by) > 0 ) {
					if( mIsTexSingleComponent ) {
						texture[(r*sColumn+c)] += mHeightFactor;	// height
					}
					else {
						texture[(r*sColumn+c)*4+3] += mHeightFactor;	// height
						texture[(r*sColumn+c)*4+0] += 2.5f / (float)FAULT_LINE_ITERATION; // R
						texture[(r*sColumn+c)*4+1] += 2.0f / (float)FAULT_LINE_ITERATION; // G
						texture[(r*sColumn+c)*4+2] += 1.5f / (float)FAULT_LINE_ITERATION; // B
					}
				}
				else {
					if( mIsTexSingleComponent ) {
						texture[(r*sColumn+c)] -= mHeightFactor;	// height
					}
					else {
						texture[(r*sColumn+c)*4+3] -= mHeightFactor;	// height
						texture[(r*sColumn+c)*4+0] -= 2.0f / (float)FAULT_LINE_ITERATION; // R
						texture[(r*sColumn+c)*4+1] -= 1.5f / (float)FAULT_LINE_ITERATION; // G
						texture[(r*sColumn+c)*4+2] -= 1.0f / (float)FAULT_LINE_ITERATION; // B
					}
				}
			}
		}
	}

	uint8_t* bs = (uint8_t*)texture;
	uint32_t sum = 0;
	for (unsigned i = 0; i < sRow*sColumn*sizeof(float)*(mIsTexSingleComponent? 1:4); ++i) {
		sum += bs[i];
	}
	printf("texture sum: %d\n", sum);

	// setup vertex texture structure

	mTexture.format = CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR 
	                | (mIsTexSingleComponent?
					  CELL_GCM_TEXTURE_X32_FLOAT:
					  CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT);
	mTexture.mipmap = 1;
	mTexture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	mTexture.cubemap = CELL_GCM_FALSE;  // Cubemap is ignored
	mTexture.remap = 0xaae4;            // Remap is ignored for vertex texture
	mTexture.width = sRow;
	mTexture.height = sColumn;
	mTexture.depth = 1;	                // Depth is ignored for vertex texture
	mTexture.pitch = sRow * sizeof(float) * 
	                 (mIsTexSingleComponent? 1:4);
	mTexture.location = CELL_GCM_LOCATION_MAIN;
	mTexture.offset = mTextureOffset;
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

	FWInputDevice	*pPad = FWInput::getDevice(FWInput::DeviceType_Pad, 0);

	if(pPad != NULL) {
		mpTriangle = pPad->bindFilter();
		mpTriangle->setChannel(FWInput::Channel_Button_Triangle);
		mpCircle = pPad->bindFilter();
		mpCircle->setChannel(FWInput::Channel_Button_Circle);
		mpCross = pPad->bindFilter();
		mpCross->setChannel(FWInput::Channel_Button_Cross);
		mpSquare = pPad->bindFilter();
		mpSquare->setChannel(FWInput::Channel_Button_Square);
		mpStart = pPad->bindFilter();
		mpStart->setChannel(FWInput::Channel_Button_Start);
		mpUp = pPad->bindFilter();
		mpUp->setChannel(FWInput::Channel_Button_Up);
		mpDown = pPad->bindFilter();
		mpDown->setChannel(FWInput::Channel_Button_Down);
	}

	/* Memory initialization */
	uint32_t min_mem_size = ((sColumn*sRow*sizeof(float)*4)+0xFFFFF) & ~0xFFFFF;
	cellGcmUtilInitializeMainMemory( min_mem_size );

	// Get texture size 
	mTextureAddress
		= (void*)cellGcmUtilAllocateMainMemory(sColumn*sRow*sizeof(float)*4, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mTextureAddress, &mTextureOffset));


	// Vertices
	mVertexBuffer = (VertexData3D*)cellGcmUtilAllocateLocalMemory(sizeof(VertexData3D)*sRow*sColumn*4, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mVertexBuffer, &mVertexOffset));

	// allocate indices buffer (add sRow for primitive restart index at end of each row)
	mIndicesBuffer = (uint32_t*)cellGcmUtilAllocateLocalMemory(sizeof(uint32_t)*sRow*sColumn*4+sRow, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mIndicesBuffer, &mIndicesOffset));

	// build quad verts
	mVertexCount = GeneratePlane(mVertexBuffer, mIndicesBuffer, 1.0f, sRow, sColumn);

	// build texture
	createFloatHeightTexture();

	// shader setup
	// init shader
	initShader();

	mModelViewProj
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "modelViewProj");
	/*mCgAnimation
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "animation");*/
	CGparameter position
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "position");
	CGparameter texcoord
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "texcoord");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( mModelViewProj );
	//CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( mCgAnimation );
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( position );
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( texcoord );

	// get vertex attribute index
	mPosIndex = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, position) - CG_ATTR0);
	mTexIndex = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, texcoord) - CG_ATTR0);

	// set texture parameters
	mVertTexture
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "texture");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( mVertTexture );
	mTexUnit = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, mVertTexture) - CG_TEXUNIT0);

	// initizalize command buffer for state 
	if(initStateBuffer() != true) return false;

#ifdef CELL_GCM_DEBUG // {
	gCellGcmDebugCallback = NULL;
#endif // }

	// inital state
	cellGcmSetCurrentBuffer(mStateBufferAddress, FWCellGCMWindow::getInstance()->getStateCmdSize());
	{
		cellGcmSetBlendEnable(CELL_GCM_FALSE);
		cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
		cellGcmSetDepthFunc(CELL_GCM_LESS);
		//cellGcmSetShadeMode(CELL_GCM_SMOOTH);
		cellGcmSetShadeMode(CELL_GCM_FLAT);
		
		// Need to put Return command because this command buffer is called statically
		cellGcmSetReturnCommand();
	}
	// Get back to default command buffer
	cellGcmSetDefaultCommandBuffer();

#ifdef CELL_GCM_DEBUG // {
	gCellGcmDebugCallback = cellGcmDebugFinish;
#endif // }

	// set default camera position
	mCamera.setPosition(mCameraDefaultPos);
	mCamera.setTilt(mCameraDefaultTilt);
	mCamera.setPan(mCameraDefaultPan);

	int32_t dead_time = (int32_t)(sDeadTime * 1000);
	mDeadTimeCount = (uint64_t)(sys_time_get_timebase_frequency() * dead_time / 1000);

	return true;
}

void SampleApp::updateTexture( void ) 
{
	createFloatHeightTexture();
	cellGcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_VERTEX_TEXTURE);
}

bool SampleApp::onUpdate()
{
	if(FWGCMCamControlApplication::onUpdate() == false) return false;

	// pad input
	uint64_t now;
	static uint64_t lastUpdate = 0;
	// asm __volatile__("mftb %0" : "=r" (now) : : "memory");
	SYS_TIMEBASE_GET(now);

	// triangle:  chagne draw primitive
	if (mpTriangle->getBoolValue() && (now > lastUpdate+mDeadTimeCount)) {
		if( mDrawPrimitive == CELL_GCM_PRIMITIVE_TRIANGLE_STRIP ) 
			mDrawPrimitive = CELL_GCM_PRIMITIVE_LINE_STRIP;
		else
			mDrawPrimitive = CELL_GCM_PRIMITIVE_TRIANGLE_STRIP;
		lastUpdate = now;
	}
	// circle:  Animation on/off
	if (mpCircle->getBoolValue() && (now > lastUpdate+mDeadTimeCount)) {
		mAnimate = !mAnimate;
		lastUpdate = now;
	}
	// square: switch between W32_Z32_Y32_X32 and X32
	if (mpSquare->getBoolValue() && (now > lastUpdate+mDeadTimeCount)) {
		mIsTexSingleComponent = mIsTexSingleComponent? false: true;
		mAnimationParameter[2] = mIsTexSingleComponent? 1.1f: 0.f;
		updateTexture();
		lastUpdate = now;
	}
	// cross: reset lerp value to 0
	if (mpCross->getBoolValue() && (now > lastUpdate+mDeadTimeCount)) {
		updateTexture();
		mAnimationParameter[1] = 0.f;
		lastUpdate = now;
	}
	// Up
	if (mpUp->getBoolValue() && (now > lastUpdate+(mDeadTimeCount/10))) {
		mHeightFactor += 0.01f;
		lastUpdate = now;
	}
	// Down
	if (mpDown->getBoolValue() && (now > lastUpdate+(mDeadTimeCount/10))) {
		mHeightFactor -= 0.01f;
		lastUpdate = now;
	}
	// start: 
	if (mpStart->getBoolValue() && (now > lastUpdate + mDeadTimeCount)) {
		mCamera.setPosition(mCameraDefaultPos);
		mCamera.setTilt(mCameraDefaultTilt);
		mCamera.setPan(mCameraDefaultPan);
		lastUpdate = now;
	}

	// for sample navigator
	FWInputDevice *pPad = FWInput::getDevice(FWInput::DeviceType_Pad, 0);
	if(pPad != NULL){
		if(cellSnaviUtilIsExitRequested(&pPad->padData)){
			return false;
		}
	}
	
	return true;
}

void print_matrix(Matrix4 matrix) {
	/*float* m = (float*)&matrix;
	printf("matrix: %g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\n",
		m[0], m[1], m[2], m[3], 
		m[4], m[5], m[6], m[7], 
		m[8], m[9], m[10], m[11], 
		m[12], m[13], m[14], m[15]);
	fflush(stdout);*/
}

void print_vector(Vector4 vector) {
	//float* m = (float*)&vector;
	//printf("vector: %g,%g,%g,%g\n", m[0], m[1], m[2], m[3]);
	//fflush(stdout);
}

bool render = 0;
void SampleApp::onRender()
{
	if (render++)
		exit(0);
	// base implementation clears screen and sets up camera
	FWGCMCamControlApplication::onRender();

	// re-execute state commands created in onInit() 
	cellGcmSetCallCommand(mStateBufferOffset);

	// set vertex pointer and draw
	cellGcmSetVertexDataArray(mPosIndex, 0, sizeof(VertexData3D), 3,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  mVertexOffset);
	cellGcmSetVertexDataArray(mTexIndex, 0, sizeof(VertexData3D), 2,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  mVertexOffset+sizeof(float)*4);
	
	cellGcmSetVertexTexture(mTexUnit, &mTexture);
	cellGcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_VERTEX_TEXTURE);

	// bind texture and set filter
	cellGcmSetVertexTextureControl(mTexUnit, CELL_GCM_TRUE, 0<<8, 12<<8);
	cellGcmSetVertexTextureAddress(mTexUnit,
							 CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
							 CELL_GCM_TEXTURE_CLAMP_TO_EDGE );
	cellGcmSetVertexTextureFilter(mTexUnit, 0<<8);

	// bind Cg programs
	// NOTE: vertex program constants are copied here
	cellGcmSetVertexProgram(mCGVertexProgram, mVertexProgramUCode);
	cellGcmSetFragmentProgram(mCGFragmentProgram, mFragmentProgramOffset);

	Vector4 vector = Vector4::Vector4(2);

	print_vector(vector);

	Vector4 yaxis = Vector4::yAxis();
	print_vector(yaxis);

	// model matrix
	Matrix4 mat = Matrix4::identity();

	print_matrix(mat);

	// final matrix
	Matrix4 proj = getProjectionMatrix();

	print_matrix(proj);

	Matrix4 viewm = getViewMatrix();

	print_matrix(viewm);

	Matrix4 MVP = transpose(proj * viewm * mat);

	print_matrix(MVP);

	// set MVP matrix
	cellGcmSetVertexProgramParameter(mModelViewProj, (float*)&MVP);

	// update animation parameter
	static float lerp = 0.01f;

	mAnimationParameter[1] = 0.5f;

	if( mAnimate ) {
		if(mAnimationParameter[1] >= 1.f)
			lerp = -0.01f;
		if(mAnimationParameter[1] <= 0.f)
			lerp = 0.01f;

		mAnimationParameter[1] += lerp;
	}
	
	// set Animation parameter
	//cellGcmSetVertexProgramParameter(mCgAnimation, (float*)&mAnimationParameter);

	//cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_QUADS, 0, mVertexCount);
	// Enable primitive restart
	cellGcmSetRestartIndex( PRIMITIVE_RESTART_INDEX );
	cellGcmSetRestartIndexEnable( CELL_GCM_TRUE );

	cellGcmSetDrawIndexArray(mDrawPrimitive, mVertexCount, 
	                         CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32, 
							 CELL_GCM_LOCATION_LOCAL, 
							 mIndicesOffset);

	#if 0
	{
		// calc fps
		FWTimeVal	time = FWTime::getCurrentTime();
		float fFPS = 1.f / (float)(time - mLastTime);
		mLastTime = time;

		// print some messages
		FWDebugFont::setPosition(0, 0);
		FWDebugFont::setColor(1.f, 1.f, 1.f, 1.0f);

		FWDebugFont::print("Vertex Texture Sample Application\n\n");
		FWDebugFont::printf("FPS: %.2f\n\n", fFPS);

		FWDebugFont::printf("Height Factor (Up/Down): %.2f\n", mHeightFactor);
		FWDebugFont::printf("Vertex Texture Format (Square): %s\n", 
						  mIsTexSingleComponent? 
						  "CELL_GCM_TEXTURE_X32_FLOAT" :
						  "CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT" );
		FWDebugFont::printf("Animation (Circle): %s\n", mAnimate? "ON": "OFF" );
		FWDebugFont::print("Press Cross to recreate texture\n" );
	}
	#endif
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

	// unbind input filters
	FWInputDevice	*pPad = FWInput::getDevice(FWInput::DeviceType_Pad, 0);

	if(pPad != NULL) {
		pPad->unbindFilter(mpTriangle);
		pPad->unbindFilter(mpCircle);
		pPad->unbindFilter(mpCross);
		pPad->unbindFilter(mpSquare);
		pPad->unbindFilter(mpUp);
		pPad->unbindFilter(mpDown);
		pPad->unbindFilter(mpStart);
	}
}
