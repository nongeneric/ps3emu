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

SampleApp app;

const float SampleApp::sDeadTime  = 0.3f;

// shader
extern struct _CGprogram _binary_vpshader_vpo_start;
extern struct _CGprogram _binary_fpshader_fpo_start;

static uint32_t GeneratePlane(VertexData3D *point, uint32_t *indices, const float size,
							   const uint32_t row, const uint32_t col)
{
	uint32_t index_count=0;
	float xoffset, zoffset;
	
	int i = 0;
	int p = 0;

	int rm = 2;
	int cm = 9;

	for (int r = 0; r <= rm; ++r) {
		for (int c = 0; c <= cm; ++c) {
			float dx = 2.f / (float)cm;
			float dy = 2.f / (float)rm;
			float x = c * dx - 1;
			float y = 1 - r * dy;

			point[p].pos[0] = x;
			point[p].pos[1] = y;
			point[p].pos[2] = 0;
			point[p].pos[3] = 1;
			point[p].v = 0;

			p++;
		}
	}

	for (int n = 0; n < (rm + 1) * (cm + 1); ++n) {
		indices[i + 0] = n + (1 + cm);
		indices[i + 1] = n + (2 + cm);
		indices[i + 2] = n + 0;
		indices[i + 3] = n + 1;
		indices[i + 4] = n + (2 + cm);
		indices[i + 5] = n + 0;
		if ((n + 1) % (cm + 1) == 0)
			i += 0;
		else 
			i += 6;
	}

	float xs[] = {
		0.2, 0.5, 0.8, 1.2, 1.5, 1.8, 2.2, 2.5, 2.8, 0
		-2.8, -2.5, -2.2, -1.8, -1.5, -1.2, -0.8, -0.5, -0.2
	};
	/*float t = 1.0f/3.0f + 0.01f;
	float b = 1.0f/3.0f - 0.01f;
	float xs[] = {
		0, t, 2*t, 1, 1 + t, 1 + 2*t, 2, 2 + t, 2 + 2*t, 0
		-2 - 2*b, -2 - b, -2, -1 - 2*b, -1 - b, -1, -2 * b, -b, 0
	};*/
	for (int n = 0; n < sizeof(xs) / 4; ++n) {
		point[n].u = xs[n];
	}

	return i;
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

	mTexture.format = CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR |
					  CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT;
	mTexture.mipmap = 1;
	mTexture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	mTexture.cubemap = CELL_GCM_FALSE;  // Cubemap is ignored
	mTexture.remap = 0xaae4;            // Remap is ignored for vertex texture
	mTexture.depth = 1;	                // Depth is ignored for vertex texture
	mTexture.location = CELL_GCM_LOCATION_MAIN;
	mTexture.offset = mTextureOffset;

	texture[0] = 1;
	texture[1] = 0;
	texture[2] = 0;
	texture[3] = 1;

	texture[4] = 0;
	texture[5] = 1;
	texture[6] = 0;
	texture[7] = 1;

	texture[8] = 1;
	texture[9] = 1;
	texture[10] = 1;
	texture[11] = 1;

	mTexture.width = 3;
	mTexture.height = 1;
	mTexture.pitch = 3 * 4 * sizeof(float);
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

int render = 0;
//-----------------------------------------------------------------------------
// Description: Render callback
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
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

	cellGcmSetVertexProgram(mCGVertexProgram, mVertexProgramUCode);
	cellGcmSetFragmentProgram(mCGFragmentProgram, mFragmentProgramOffset);

	cellGcmSetRestartIndex( PRIMITIVE_RESTART_INDEX );
	cellGcmSetRestartIndexEnable( CELL_GCM_TRUE );
	
	cellGcmSetVertexTextureBorderColor(mTexUnit, 0xFF555555);

	for (int wraps = 1; wraps <= 8; ++wraps) {
		for (int wrapt = 1; wrapt <= 8; ++wrapt) {
			Matrix4 scale = Matrix4::scale(Vector3(1.f/8.2,1.f/8.2,0));
			Matrix4 translate = Matrix4::translation(Vector3(1, -1, 0));
			float dx = 2.f/8;
			float dy = -2.f/8;
			Matrix4 translate2 = Matrix4::translation(Vector3(-1, 1, 0));
			Matrix4 translate3 = Matrix4::translation(Vector3((wraps - 1) * dx, (wrapt - 1) * dy, 0));
			Matrix4 MVP = transpose( translate3 * translate2 * scale * translate );
			cellGcmSetVertexProgramParameter(mModelViewProj, (float*)&MVP);

			cellGcmSetVertexTextureAddress(mTexUnit, wraps, wrapt);

			cellGcmSetDrawIndexArray(CELL_GCM_PRIMITIVE_TRIANGLES, mVertexCount,
									 CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32, 
									 CELL_GCM_LOCATION_LOCAL, 
									 mIndicesOffset);	
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
