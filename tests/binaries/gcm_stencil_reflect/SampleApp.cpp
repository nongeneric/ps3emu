/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#define __CELL_ASSERT__

#include <stdio.h>
#include <assert.h>
#include <sys/sys_time.h>
#include <sys/time_util.h>
#include <cell/gcm.h>
#include <cell/gcm_pm.h>

#include "SampleApp.h"
#include "FWCellGCMWindow.h"
#include "FWDebugFont.h"
#include "FWTime.h"
#include "cellutil.h"
#include "gcmutil.h"

#include "snaviutil.h"

using namespace cell::Gcm;

// data for torus/sphere object
#include "torus.h"
#include "sphere.h"

#define PI 3.141592653

#define STATE_BUFSIZE (0x100000) // 1MB

#define PLANE_DEFAULT_X   1.f
#define PLANE_DEFAULT_Y   1.f
#define PLANE_DEFAULT_Z   0.f
#define PLANE_DEFAULT_W   0.f

#define DEFAULT_CLIP_FUNC CELL_GCM_USER_CLIP_PLANE_ENABLE_GE

// instantiate the class
SampleApp app;
const float SampleApp::sDeadTime = 0.5f;

// shader
extern struct _CGprogram _binary_vpshader_vpo_start;
extern struct _CGprogram _binary_fpshader_fpo_start;

//-----------------------------------------------------------------------------
// Description: Constructor
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
SampleApp::SampleApp()
	: mClipPlaneFunc( DEFAULT_CLIP_FUNC ),
	  mClipPlaneEnable( false ),
	  mCameraDefaultPos(Point3(0.f, 5.f, 5.f)),
	  mCameraDefaultTilt(3.1415f / 4.f), mCameraDefaultPan(0.0f),
	  mSelectedParameter(0), mObjectRotation(true)
{
	mClearRed   = 1.f;
	mClearGreen = 1.f;
	mClearBlue  = 1.f;
	mClearAlpha = 0.f;

	for( int i=0; i<NUM_OBJECTS; i++ ) {
		memset( &mTorus[i], 0, sizeof(RenderObject));
		memset( &mSphere[i], 0, sizeof(RenderObject));
	}
	memset( &mPlane, 0, sizeof(RenderObject));
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

void SampleApp::DrawObject(RenderObject* object, Matrix4* M, Matrix4* IM, Matrix4* VP) 
{
	// Set vertexprogram parameter
	{
		cellGcmSetVertexProgramParameter(mModelMatrixParameter, (float*)M);
		cellGcmSetVertexProgramParameter(mViewProjMatrixParameter, (float*)VP);
	}

	// set fragment program parameter
	{
		Vector4 light_pos_local;
		Vector3 eye_pos_local;

		// get local space light position
		light_pos_local = (*IM) * mLightPos;

		// get local space eye position
		Matrix4 invView = mCamera.getInverseMatrix();
		eye_pos_local = Vector3( invView[3][0], invView[3][1], invView[3][2] );

		// set parameter in microcode
		cellGcmSetFragmentProgramParameter(mCGFragmentProgram,
	                                   mLightPosParameter,
	                                   (float*)&light_pos_local, 
									   mFragmentProgramOffset );
		cellGcmSetFragmentProgramParameter(mCGFragmentProgram,
	                                   mLightPosParameter,
	                                   (float*)&light_pos_local, 
									   mFragmentProgramOffset );

		// invalidate instruction cache in shader pipe
		cellGcmSetUpdateFragmentProgramParameter(mFragmentProgramOffset);
	}

	// set vertex pointer and draw
	cellGcmSetVertexDataArray(mPosIndex, 0, sizeof(float)*3, 3,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  object->vert.offset);
	cellGcmSetVertexDataArray(mNormIndex, 0, sizeof(float)*3, 3,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  object->normal.offset );
	cellGcmSetVertexData4f(mColorIndex, object->color );

	cellGcmSetInvalidateVertexCache();
	cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_TRIANGLES, 0, object->v_count);
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
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mFragmentProgramUCode, &mFragmentProgramOffset));

	memcpy(mFragmentProgramUCode, ucode, ucodeSize); 

	// get and copy 
	cellGcmCgGetUCode(mCGVertexProgram, &ucode, &ucodeSize);
	mVertexProgramUCode = ucode;

	// Update attribute output mask in order to use user clip functionality.
	// This is done by doing:
	//  1. Get vertex binary program's attribute output mask by 
	//     cellGcmCgGetAttribOutputMask().
	//  2. Enable user clipx attribute by setting bit in mask to 1.
	//  3. Set new mask to fragment shader's 'input' mask by
	//     cellGcmCgSetAttribOutputMask().
	//

	uint32_t new_mask = 0;

	new_mask = cellGcmCgGetAttribOutputMask( mCGVertexProgram );
	// Enable User Clip 0 attribute
	new_mask |= CELL_GCM_ATTRIB_OUTPUT_MASK_UC0; 

	cellGcmCgSetAttribOutputMask( mCGFragmentProgram, new_mask );
}

// random value between 0.f to 1.f
#define GET_RAND() ((float)rand()/(float)RAND_MAX)
#define GET_RAND_MINMAX(min, max) (min + (GET_RAND() * (max - min)))

static void getRandomPosition( float* x, float* y, float* z )
{
	// set up defalt position
	float minx, miny, minz;
	float maxx, maxy, maxz;

	maxx = (float)NUM_OBJECTS/1.f;
	minx = -maxx; 
	maxy = (float)NUM_OBJECTS/1.f;
	miny = -maxy; 
	maxz = (float)NUM_OBJECTS/1.f;
	minz = -maxz; 

	*x = GET_RAND_MINMAX(minx, maxx);

	*y = GET_RAND_MINMAX(miny, maxy);
	// threshold for y (not to intersect with plane)
	if( *y < 1.f && *y > -1.f ) {
		*y += (*y > 0.f)? 1.f: -1.f;
	}

	*z = GET_RAND_MINMAX(minz, maxz);
}

void SampleApp::resetGeomData(void) 
{
	float width;
	width = (float)NUM_OBJECTS / 2.f;

	// copy plane dimenstion
	static float plane[] = {
		-width, 0.f, -width, 
		 width, 0.f, -width, 
		-width, 0.f,  width, 

		-width, 0.f,  width, 
		 width, 0.f, -width, 
		 width, 0.f,  width, 
	};
	memcpy(mPlane.vert.address, plane, mPlane.vert.size);

	// copy plane normal 
	static float plane_normal[] = { 0.f, 1.f, 0.f };
	for( uint32_t i=0; i<mPlane.v_count; i++ ) {
		memcpy((void*)((float*)mPlane.normal.address+i*3), plane_normal, sizeof(float)*3);
	}

	// torus and sphere
	for( int i=0; i<NUM_OBJECTS; i++ ) {
		float x, y, z;

		getRandomPosition( &x, &y, &z );
		mTorus[i].pos  = Vector3( x, y, z );

		getRandomPosition( &x, &y, &z );
		mSphere[i].pos = Vector3( x, y, z );

		mTorus[i].color[0] = GET_RAND_MINMAX(0.f, 1.f);
		mTorus[i].color[1] = GET_RAND_MINMAX(0.f, 1.f);
		mTorus[i].color[2] = GET_RAND_MINMAX(0.f, 1.f);
		mTorus[i].color[3] = 1.f;

		mSphere[i].color[0] = GET_RAND_MINMAX(0.f, 1.f);
		mSphere[i].color[1] = GET_RAND_MINMAX(0.f, 1.f);
		mSphere[i].color[2] = GET_RAND_MINMAX(0.f, 1.f);
		mSphere[i].color[2] = 1.f;
	}

	// plane
	mPlane.pos  = Vector3( 0.0f, 0.0f, 0.0f );
	mPlane.color[0] = 0.0f; // r
	mPlane.color[1] = 0.8f; // g
	mPlane.color[2] = 0.8f; // b
	mPlane.color[3] = 1.0f; // a
}


void SampleApp::initGeomData(void)
{
	// Copy torus data to attribute buffer
	memcpy(mTorus[0].vert.address, torusVert,   mTorus[0].vert.size);
	memcpy(mTorus[0].normal.address, torusNorm, mTorus[0].normal.size);

	// Copy sphere data to attribute buffer
	memcpy(mSphere[0].vert.address, sphereVert,   mSphere[0].vert.size);
	memcpy(mSphere[0].normal.address, sphereNorm, mSphere[0].normal.size);

	// copy 1 geometry to rest of object
	for( int i=1; i<NUM_OBJECTS; i++ ) {
		memcpy( &mTorus[i], &mTorus[0], sizeof(RenderObject) ); 
		memcpy( &mSphere[i], &mSphere[0], sizeof(RenderObject) ); 
	}

	// set new position and color
	resetGeomData();
}


void SampleApp::initGeometry(void)
{
	// Allocate attribute buffers for torus
	{
		mTorus[0].v_count = TORUSVCOUNT;

		mTorus[0].vert.size     = sizeof(float) * 3 * mTorus[0].v_count;
		mTorus[0].normal.size   = sizeof(float) * 3 * mTorus[0].v_count;

		mTorus[0].vert.address     = (void*)cellGcmUtilAllocateLocalMemory(mTorus[0].vert.size, 128); 
		mTorus[0].normal.address   = (void*)cellGcmUtilAllocateLocalMemory(mTorus[0].normal.size, 128); 

		CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mTorus[0].vert.address, &mTorus[0].vert.offset));
		CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mTorus[0].normal.address, &mTorus[0].normal.offset));
	}

	// Allocate attribute buffers for sphere
	{
		mSphere[0].v_count = SPHEREVCOUNT;

		mSphere[0].vert.size     = sizeof(float) * 3 * mSphere[0].v_count;
		mSphere[0].normal.size   = sizeof(float) * 3 * mSphere[0].v_count;

		mSphere[0].vert.address     = (void*)cellGcmUtilAllocateLocalMemory(mSphere[0].vert.size, 128); 
		mSphere[0].normal.address   = (void*)cellGcmUtilAllocateLocalMemory(mSphere[0].normal.size, 128); 

		CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mSphere[0].vert.address, &mSphere[0].vert.offset));
		CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mSphere[0].normal.address, &mSphere[0].normal.offset));
	}

	// Allocate attribute buffers for plane
	{
		mPlane.v_count = 6;

		mPlane.vert.size   = sizeof(float) * 3 * 6;
		mPlane.normal.size = sizeof(float) * 3 * 6;

		mPlane.vert.address     = (void*)cellGcmUtilAllocateLocalMemory(mPlane.vert.size, 128); 
		mPlane.normal.address   = (void*)cellGcmUtilAllocateLocalMemory(mPlane.normal.size, 128);

		CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mPlane.vert.address, &mPlane.vert.offset));
		CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mPlane.normal.address, &mPlane.normal.offset));
	}

	// this will copy first object to rest of objects
	initGeomData();
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

	// Controller init
	FWInputDevice	*pPad = FWInput::getDevice(FWInput::DeviceType_Pad, 0);

	if(pPad != NULL) {
		mpTriangle = pPad->bindFilter();
		mpTriangle->setChannel(FWInput::Channel_Button_Triangle);
		mpUp = pPad->bindFilter();
		mpUp->setChannel(FWInput::Channel_Button_Up);
		mpDown = pPad->bindFilter();
		mpDown->setChannel(FWInput::Channel_Button_Down);
		mpLeft = pPad->bindFilter();
		mpLeft->setChannel(FWInput::Channel_Button_Left);
		mpRight = pPad->bindFilter();
		mpRight->setChannel(FWInput::Channel_Button_Right);
		mpL1 = pPad->bindFilter();
		mpL1->setChannel(FWInput::Channel_Button_L1);
		mpL2 = pPad->bindFilter();
		mpL2->setChannel(FWInput::Channel_Button_L2);
		mpR1 = pPad->bindFilter();
		mpR1->setChannel(FWInput::Channel_Button_R1);
		mpR2 = pPad->bindFilter();
		mpR2->setChannel(FWInput::Channel_Button_R2);
		mpSelect = pPad->bindFilter();
		mpSelect->setChannel(FWInput::Channel_Button_Select);
		mpStart = pPad->bindFilter();
		mpStart->setChannel(FWInput::Channel_Button_Start);
	}

	// allocate buffers for geometry
	initGeometry();

	// shader setup
	initShader();

	// get fragment program parameter index
	mLightPosParameter
		= cellGcmCgGetNamedParameter(mCGFragmentProgram, "lightPosLocal");
	mEyePosParameter
		= cellGcmCgGetNamedParameter(mCGFragmentProgram, "eyePosLocal");
	//CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(mLightPosParameter);
	//CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(mEyePosParameter);

	// get vertex program parameter index
	mModelMatrixParameter
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "ModelMatrix");
	mViewProjMatrixParameter
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "ViewProjMatrix");
	mCgClipPlane
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "clipPlane");
	CGparameter position
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "position");
	CGparameter normal
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "normal");
	CGparameter color
		= cellGcmCgGetNamedParameter(mCGVertexProgram, "color");
	//CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( mModelMatrixParameter );
	//CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( mViewProjMatrixParameter );
	//CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( mCgClipPlane );
	//CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( position );
	//CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( normal );
	//CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( color );

	// get vertex attribute index
	mPosIndex  = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, position) - CG_ATTR0);
	mNormIndex = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, normal) - CG_ATTR0);
	mColorIndex= (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, color) - CG_ATTR0);

	// initizalize command buffer for state 
	if(initStateBuffer() != true) return false;

#ifdef CELL_GCM_DEBUG // [
	gCellGcmDebugCallback = NULL;
#endif // ]

	// inital state
	cellGcmSetCurrentBuffer(mStateBufferAddress, FWCellGCMWindow::getInstance()->getStateCmdSize());
	{
		cellGcmSetBlendEnable(CELL_GCM_FALSE);
		cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
		cellGcmSetDepthFunc(CELL_GCM_LESS);
		cellGcmSetShadeMode(CELL_GCM_SMOOTH);

		cellGcmSetStencilTestEnable( CELL_GCM_FALSE );

		// clear color
		cellGcmSetClearColor( 0x00FFFFFF );

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

	// set default light and eye position (in camera space)
	mLightPos = Vector3( 0.f, 4.f, 2.f );

	int32_t dead_time = (int32_t)(sDeadTime * 1000);
	mDeadTimeCount = (uint64_t)(sys_time_get_timebase_frequency() * dead_time / 1000);

	return true;
}

// onUpdate
bool SampleApp::onUpdate()
{
	if(FWGCMCamControlApplication::onUpdate() == false) return false;

	// pad input
	uint64_t now;
	static uint64_t lastUpdate = 0;
	SYS_TIMEBASE_GET(now);

	// up: 
	if (mpUp->getBoolValue()) {
		mPlane.pos[2] -= 0.05f;
	}
	// down:
	if (mpDown->getBoolValue()) {
		mPlane.pos[2] += 0.05f;
	}
	// Left: 
	if (mpLeft->getBoolValue()) {
		mPlane.pos[0] -= 0.05f;
	}
	// Right: 
	if (mpRight->getBoolValue()) {
		mPlane.pos[0] += 0.05f;
	}
	// triangle:
	if (mpTriangle->getBoolValue() && (now > lastUpdate + mDeadTimeCount)) {
		mClipPlaneEnable = mClipPlaneEnable? false : true;
		lastUpdate = now;
	}
	// select: back to default state
	if (mpSelect->getBoolValue() && (now > lastUpdate + mDeadTimeCount)) {
		mClipPlaneFunc = DEFAULT_CLIP_FUNC;
		mClipPlaneEnable = true;
		mCamera.setPosition(mCameraDefaultPos);
		mCamera.setTilt(mCameraDefaultTilt);
		mCamera.setPan(mCameraDefaultPan);
		resetGeomData();

		lastUpdate = now;
	}
	// start: rotation on/off
	if (mpStart->getBoolValue() && (now > lastUpdate + mDeadTimeCount)) {
		mObjectRotation = mObjectRotation ? false: true;
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

	// bind Cg programs
	// NOTE: vertex program constants are copied here
	cellGcmSetVertexProgram(mCGVertexProgram, mVertexProgramUCode);
	cellGcmSetFragmentProgram(mCGFragmentProgram, mFragmentProgramOffset);

	// model rotate
	static float AngleX = 0.3f; 
	static float AngleY = 0.3f; 
	static float AngleZ = 0.0f;

	if( mObjectRotation ) {
		AngleX += 0.01f;
		AngleY += 0.01f;
		AngleZ += 0.000f;
	}

	Matrix4 mat; // model matrix
	Matrix4 inv; // inverse model matrix
	Matrix4 VP;  // view projection matrix

	Matrix4 reflectMatrix = Matrix4::scale(Vector3(1.f, -1.f, 1.f));

	// Draw Plane:
	//   This will create stencil buffer to be used for next draw pass.
	cellGcmSetStencilTestEnable( CELL_GCM_TRUE );

	cellGcmSetStencilMask( (uint32_t)0xFF );
	cellGcmSetStencilFunc(CELL_GCM_ALWAYS, 0, 0xFFFFFFFF);
	cellGcmSetStencilOp( CELL_GCM_KEEP, CELL_GCM_KEEP, CELL_GCM_INCR );

	{
		mat = Matrix4::translation(  mPlane.pos );
		inv = Matrix4::translation( -mPlane.pos );
		VP = getProjectionMatrix() * getViewMatrix();

		DrawObject( &mPlane, &mat, &inv, &VP );
	}

	// Define user clip plane that will clip all the primitives 
	// located above/below the plane.
	static float clipplane[4] = { 0.f, 1.f, 0.f, 0.f };  // y = 0
	cellGcmSetVertexProgramParameter(mCgClipPlane, clipplane);

	if( mClipPlaneEnable ) {
		// Taking dot product of plane equation and camera position,
		// if negative then camera is at 'less' side of plane,
		// if positive then camera is at 'greater' side of plane. 
		if( dot( Vector4(mCamera.getPosition()), 
				 Vector4(clipplane[0], clipplane[1], clipplane[2], clipplane[3])) < 0.f )
			mClipPlaneFunc = CELL_GCM_USER_CLIP_PLANE_ENABLE_LT;
		else
			mClipPlaneFunc = CELL_GCM_USER_CLIP_PLANE_ENABLE_GE;
	}
	else 
		mClipPlaneFunc = CELL_GCM_USER_CLIP_PLANE_DISABLE;

	// Only one clip plane (clp0) is to be used for this sample.
	cellGcmSetUserClipPlaneControl( 
				mClipPlaneFunc,                   CELL_GCM_USER_CLIP_PLANE_DISABLE, 
				CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, 
				CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE);


	// Set stencil operation.
	// Each tiled fragments is tested against stencil buffer created at first draw pass.
	cellGcmSetStencilFunc(CELL_GCM_LESS, 0, 0xFFFFFFFF);
	cellGcmSetStencilOp( CELL_GCM_KEEP, CELL_GCM_KEEP, CELL_GCM_KEEP );

	// Clear only depth buffer.
	// Color buffer needs to be preserved for reflection effect.
	cellGcmSetClearSurface( CELL_GCM_CLEAR_Z );

	// Draw with reflection matrix with stencil test
	{
		Matrix4 rot = Matrix4::rotationZYX(Vector3(AngleX, AngleY, AngleZ));
		VP = getProjectionMatrix() * (getViewMatrix() * reflectMatrix);

		// Draw Mirror Torus & Sphere
		for( int i=0; i<NUM_OBJECTS; i++ ) 
		{
			// draw torus
			mat = Matrix4::translation(mTorus[i].pos) * rot;
			inv = transpose(rot) * Matrix4::translation( -mTorus[i].pos );

			DrawObject( &mTorus[i], &mat, &inv, &VP );

			// draw sphere
			mat = Matrix4::translation(mSphere[i].pos) * rot;
			inv = transpose(rot) * Matrix4::translation( -mSphere[i].pos );

			DrawObject( &mSphere[i], &mat, &inv, &VP );
		}
	}

	// disable stencil test
	cellGcmSetStencilTestEnable( CELL_GCM_FALSE );
	// disable user clip0
	cellGcmSetUserClipPlaneControl( 
				CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, 
				CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, 
				CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE);

	// Draw plane again to restore depth value for final geometry rendering.
	// Setting the color mask to zero, only depth value is drawn to depth buffer.
	cellGcmSetColorMask( 0 );
	{
		mat = Matrix4::translation(  mPlane.pos);
		inv = Matrix4::translation( -mPlane.pos);
		VP = getProjectionMatrix() * getViewMatrix();

		DrawObject( &mPlane, &mat, &inv, &VP );
	}

	// Restore color mask for final pass.
	cellGcmSetColorMask( CELL_GCM_COLOR_MASK_R | 
	                     CELL_GCM_COLOR_MASK_G |
						 CELL_GCM_COLOR_MASK_B |
						 CELL_GCM_COLOR_MASK_A );

	// Rendering geometry.
	{
		Matrix4 rot = Matrix4::rotationZYX(Vector3(AngleX, AngleY, AngleZ));
		VP = getProjectionMatrix() * getViewMatrix();

		for( int i=0; i<NUM_OBJECTS; i++ ) 
		{
			// draw torus
			mat = Matrix4::translation(  mTorus[i].pos ) * rot;
			inv = transpose(rot) * Matrix4::translation( -mTorus[i].pos );

			DrawObject( &mTorus[i], &mat, &inv, &VP );

			// draw sphere
			mat = Matrix4::translation(  mSphere[i].pos ) * rot;
			inv = transpose(rot) * Matrix4::translation( -mSphere[i].pos );

			DrawObject( &mSphere[i], &mat, &inv, &VP );
		}
	}

	// flush semaphore command bacause semaphore value is not updated
	cellGcmFlush();

	// Debug font 
	#if 1  // {	
	{
		// calc fps
		FWTimeVal	time = FWTime::getCurrentTime();
		float fFPS = 1.f / (float)(time - mLastTime);
		mLastTime = time;

		// print some messages
		FWDebugFont::setPosition(0, 0);
		FWDebugFont::setColor(0.f, 0.f, 0.f, 1.0f);

		FWDebugFont::print("Userclip Sample Application\n\n");
		FWDebugFont::printf("FPS: %.2f\n\n", fFPS);
		FWDebugFont::printf("Clip Plane\n\n");

		// restore color
		FWDebugFont::setColor(0.f, 0.f, 0.f, 1.f);

		// print clip plane comparison function
		switch( mClipPlaneFunc ) {
		case CELL_GCM_USER_CLIP_PLANE_DISABLE:
			FWDebugFont::print("Clip Plane Func : CELL_GCM_USER_CLIP_PLANE_DISABLE\n" );
			break;
		case CELL_GCM_USER_CLIP_PLANE_ENABLE_LT:
			FWDebugFont::print("Clip Plane Func : CELL_GCM_USER_CLIP_PLANE_ENABLE_LT\n" );
			break;
		case CELL_GCM_USER_CLIP_PLANE_ENABLE_GE:
			FWDebugFont::print("Clip Plane Func : CELL_GCM_USER_CLIP_PLANE_ENABLE_GE\n" );
			break;
		default:
			FWDebugFont::print("Clip Plane Func : Unknown\n" );
		}

	}
	#endif // }
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

	pPad->unbindFilter(mpTriangle);
	pPad->unbindFilter(mpUp);
	pPad->unbindFilter(mpDown);
	pPad->unbindFilter(mpLeft);
	pPad->unbindFilter(mpRight);
	pPad->unbindFilter(mpL1);
	pPad->unbindFilter(mpL2);
	pPad->unbindFilter(mpR1);
	pPad->unbindFilter(mpR2);
	pPad->unbindFilter(mpSelect);
	pPad->unbindFilter(mpStart);
}
