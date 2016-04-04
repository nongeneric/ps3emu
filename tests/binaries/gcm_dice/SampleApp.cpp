/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#define __CELL_ASSERT__

#include <stdio.h>
#include <assert.h>

//#define CELL_GCM_DEBUG
#include <cell/gcm.h>

using namespace cell::Gcm;

#define PI 3.141592653
#define HEAP_SIZE 1048576
#define SQUARE_COUNT 2000

struct square
{
	float h, v;
	float hvel, vvel;
	float r,g,b,a;
	int32_t angle;
	int32_t rotvel;
	float size;
	uint32_t texnum;
	uint32_t bucket;
};

#include "SampleApp.h"
#include "FWCellGCMWindow.h"
#include "FWDebugFont.h"
#include "FWTime.h"
#include "cellutil.h"
#include "gcmutil.h"

#include "snaviutil.h"

// instantiate the class
SampleApp app;

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
: mTextureWidth(256), mTextureHeight(256), mTextureDepth(4)
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

	mFragmentProgramUCode = (void*)cellGcmUtilAllocateLocalMemory(ucodeSize, 64);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mFragmentProgramUCode, &mFragmentProgramOffset));

	memcpy(mFragmentProgramUCode, ucode, ucodeSize); 

	// get and copy 
	cellGcmCgGetUCode(mCGVertexProgram, &ucode, &ucodeSize);
	mVertexProgramUCode = ucode;
}

static void makeDotTexture(uint32_t *texture, 
	uint32_t width, uint32_t height,
	float *dotTable, uint32_t dotCount, float dotRadius,
	uint32_t faceColor, uint32_t dotColor)
{
	for (uint32_t i = 0; i < height; ++i) 
	{
		for (uint32_t j = 0; j < width; ++j) 
		{
			uint32_t color = faceColor;
			float x = (float)j/(float)width;
			float y = (float)i/(float)height;

			for(uint32_t d = 0; d < dotCount; ++d)
			{
				float xDelta = x - dotTable[2 * d];
				float yDelta = y - dotTable[2 * d + 1];
				if( xDelta * xDelta + yDelta * yDelta < dotRadius * dotRadius) 
				{
					color = dotColor;
					break;
				}
			}

			texture[i*width + j] = color;
		}
	}
}

static float frand(float min, float max)
{
	float randval = float(rand()) / float(RAND_MAX);
	randval = min + (max - min) * randval;
	return randval;
}

void SampleApp::initSquares (void) 
{
	for(uint32_t i = 0; i < SQUARE_COUNT; ++i)
	{
		uint32_t bucket;
		uint32_t bucketrand = rand() & 0xff;
		if(bucketrand < 0x10)
			bucket = 0; // "solid" -- opaque
		else if(bucketrand < 0xff)
			bucket = 1; // "trans" -- translucent
		else
			bucket = 2; // "minim" -- minimum

		square *psq = &mSquareTable[i];
		psq->h = frand(-9.0f, 9.0f);
		psq->v = frand(-5.5f, 5.5f);
		psq->angle = rand() & 0xff;
		psq->texnum = rand() % 6;
		psq->bucket = bucket;

		if(bucket == 0) 
		{
			psq->r = frand(0.7f, 1.0f);
			psq->g = frand(0.7f, 1.0f);
			psq->b = frand(0.7f, 1.0f);
			psq->a = 1.0f;
			psq->size = 0.35f;
			psq->hvel = frand(-0.01f, 0.01f);
			psq->vvel = frand(-0.01f, 0.01f);
			psq->rotvel = ((rand() & 0x1f)-0x10) << 11;
		} 
		else if (bucket == 1) 
		{
			psq->r = frand(0.1f, 0.9f);
			psq->g = frand(0.1f, 0.9f);
			psq->b = frand(0.1f, 0.9f);
			psq->a = 0.6f;
			psq->size = 0.15f;
			psq->hvel = frand(-0.03f, 0.03f);
			psq->vvel = frand(-0.03f, 0.03f);
			psq->rotvel = ((rand() & 0x1f)-0x10) << 14;
		} 
		else 
		{
			psq->r = 1.0f;
			psq->g = 1.0f;
			psq->b = 1.0f;
			psq->a = 0.0f;
			psq->size = 2.5f;
			psq->hvel = frand(-0.02f, 0.02f);
			psq->vvel = frand(-0.02f, 0.02f);
			psq->rotvel = 0;
			psq->texnum = 0;
		}
	}
}

void SampleApp::makeVertexTable()
{
	static uint32_t angleoff[6] = {64, 0, 192, 64, 192, 128};
	static float ucoord[6] = {1.0, 0.0, 0.0, 1.0, 0.0, 1.0};
	static float vcoord[6] = {0.0, 0.0, 1.0, 0.0, 1.0, 1.0};

	for (uint32_t i = 0; i < SQUARE_COUNT; ++i) 
	{
		float *pvtx = &mVertexTable[30 * i];
		square *psq = &mSquareTable[i];

		for(uint32_t j = 0; j < 6; ++j) 
		{
			uint32_t ang = (psq->angle >> 16) + angleoff[j];
			float x = psq->h + (mCosTable[ang & 0xff] * psq->size);
			float y = psq->v + (mCosTable[(ang + 64) & 0xff] * psq->size);
			*pvtx++ = x * (1.0f/8.0f);
			*pvtx++ = y * (1.0f/4.5f);
			*pvtx++ = -1.0f;
			*pvtx++ = ucoord[j];
			*pvtx++ = vcoord[j];
		}
		psq->h += psq->hvel;
		if(psq->h > 9.0f)
			psq->h -= 18.0f;
		if(psq->h < -9.0f)
			psq->h += 18.0f;

		psq->v += psq->vvel;
		if(psq->v > 5.5f)
			psq->v -= 11.0f;
		if(psq->v < -5.5f)
			psq->v += 11.0f;

		psq->angle += psq->rotvel;
	}
}

uint32_t *gHeapEnd;
uint32_t *gHeapCurrent;
static int32_t GcmReserveFailed( CellGcmContextData *context, uint32_t /*count*/ )
{
	if(gHeapCurrent == gHeapEnd)
		return CELL_GCM_ERROR_FAILURE;

	uint32_t numWords = 0x2000 / sizeof(uint32_t);
	uint32_t *nextbegin = gHeapCurrent;
	uint32_t *nextend = nextbegin + numWords - 1;
	uint32_t nextbeginoffset;
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(nextbegin, &nextbeginoffset));
	gHeapCurrent += numWords;

	//use unsafe version so as not to trigger another callback
	((UnsafeInline::CellGcmContext*)context)->SetJumpCommand(nextbeginoffset);

	//set up new context
	context->current = nextbegin;
	context->end = nextend;
	return CELL_OK;
}

//-----------------------------------------------------------------------------
// Description: Initialization callback
// Parameters: 
// Returns:
// Notes: 
//-----------------------------------------------------------------------------
bool SampleApp::onInit(int argc, char ** ppArgv)
{
	FWGCMCamControlApplication::onInit(argc, ppArgv);

	uint32_t offset;
	mVertexTable = (float *)memalign(1024 * 1024, 1024 * 1024);
	CELL_GCMUTIL_ASSERTS(mVertexTable != NULL,"memalign()");
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmMapMainMemory(mVertexTable, 1024 * 1024, &mVertexTableOffset));
	mHeapBegin[0] = (uint32_t *)memalign(1024 * 1024, HEAP_SIZE);
	CELL_GCMUTIL_ASSERTS(mHeapBegin[0] != NULL,"memalign()");
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmMapMainMemory(mHeapBegin[0], HEAP_SIZE, &offset));
	mHeapBegin[1] = (uint32_t *)memalign(1024 * 1024, HEAP_SIZE);
	CELL_GCMUTIL_ASSERTS(mHeapBegin[1] != NULL,"memalign()");
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmMapMainMemory(mHeapBegin[1], HEAP_SIZE, &offset));
	mHeapIdx = 0;

	uint32_t size = mTextureWidth * mTextureHeight * mTextureDepth;
	for(uint32_t i = 0; i < 6; ++i) 
	{
		mTextures[i] = (uint32_t *)cellGcmUtilAllocateLocalMemory(size, 128);
		CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(mTextures[i], &mTextureOffsets[i]));
	}

	// build textures
	static float dots1[2]  = {0.5f, 0.5f};
	static float dots2[4]  = {0.25f, 0.25f,  0.75f, 0.75f};
	static float dots3[6]  = {0.25f, 0.25f,  0.50f, 0.50f,  0.75f, 0.75f};
	static float dots4[8]  = {0.25f, 0.25f,  0.25f, 0.75f,  0.75f, 0.25f,  0.75f, 0.75f};
	static float dots5[10] = {0.25f, 0.25f,  0.25f, 0.75f,  0.50f, 0.50f,  0.75f, 0.25f,  0.75f, 0.75f};
	static float dots6[12] = {0.25f, 0.25f,  0.50f, 0.25f,  0.75f, 0.25f,  0.25f, 0.75f,  0.50f, 0.75f,  0.75f, 0.75f};
	makeDotTexture(mTextures[0], mTextureWidth, mTextureHeight,
		dots1, 1, 0.2f,  0xc0c0c0,  0xff2020);  
	makeDotTexture(mTextures[1], mTextureWidth, mTextureHeight,
		dots2, 2, 0.1f,  0xc0c0c0,  0x202020);  
	makeDotTexture(mTextures[2], mTextureWidth, mTextureHeight,
		dots3, 3, 0.1f,  0xc0c0c0,  0x202020);  
	makeDotTexture(mTextures[3], mTextureWidth, mTextureHeight,
		dots4, 4, 0.1f,  0xc0c0c0,  0x202020);  
	makeDotTexture(mTextures[4], mTextureWidth, mTextureHeight,
		dots5, 5, 0.1f,  0xc0c0c0,  0x202020);  
	makeDotTexture(mTextures[5], mTextureWidth, mTextureHeight,
		dots6, 6, 0.1f,  0xc0c0c0,  0x202020);  

	// cos generate table
	for (uint32_t i = 0; i < 256; i++)
		mCosTable[i] = cosf(i*2.0*PI/256);

	// shader setup
	// init shader
	initShader();

	CGparameter position = cellGcmCgGetNamedParameter(mCGVertexProgram, "position");
	CGparameter texcoord = cellGcmCgGetNamedParameter(mCGVertexProgram, "texcoord");
	CGparameter color = cellGcmCgGetNamedParameter(mCGVertexProgram, "tint");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(position);
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(texcoord);
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(color);

	// get vertex attribute index
	mPosIndex = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, position) - CG_ATTR0);
	mTexIndex = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, texcoord) - CG_ATTR0);
	mColIndex = (CGresource)(cellGcmCgGetParameterResource(mCGVertexProgram, color) - CG_ATTR0);

	// set texture parameters
	CGparameter texture = cellGcmCgGetNamedParameter(mCGFragmentProgram, "texture");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(texture);
	mTexUnit = (CGresource)(cellGcmCgGetParameterResource(mCGFragmentProgram, texture) - CG_TEXUNIT0);

	initSquares();

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
	if (frame == 1) {
		cellGcmFinish(0x13131313);
		exit(0);
	}
	// base implementation clears screen and sets up camera
	FWGCMCamControlApplication::onRender();

	//Update objects, create their vertex table
	makeVertexTable();
	cellGcmSetInvalidateVertexCache();

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

	cellGcmSetTextureControl(mTexUnit, CELL_GCM_TRUE, 0<<8, 12<<8, CELL_GCM_TEXTURE_MAX_ANISO_1); // MIN:0,MAX:12
	cellGcmSetTextureAddress(mTexUnit,
		CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
		CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
		CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
		CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL,
		CELL_GCM_TEXTURE_ZFUNC_LESS, 0);
	cellGcmSetTextureFilter(mTexUnit, 0,
		CELL_GCM_TEXTURE_NEAREST_LINEAR,
		CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);

	cellGcmSetVertexProgram(mCGVertexProgram, mVertexProgramUCode);
	cellGcmSetFragmentProgram(mCGFragmentProgram, mFragmentProgramOffset);
	cellGcmSetVertexDataArray(mColIndex, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, 0);

	//Set up context structures
	CellGcmContext solidCon, transCon, minimCon;

	cellGcmSetupContextData(&solidCon, mHeapBegin[mHeapIdx], 0x2000, &GcmReserveFailed);
	cellGcmSetupContextData(&transCon, mHeapBegin[mHeapIdx] + 0x2000/sizeof(uint32_t), 0x2000, &GcmReserveFailed);
	cellGcmSetupContextData(&minimCon, mHeapBegin[mHeapIdx] + 0x4000/sizeof(uint32_t), 0x2000, &GcmReserveFailed);
	gHeapCurrent = mHeapBegin[mHeapIdx] + 0x6000/sizeof(uint32_t);
	gHeapEnd = mHeapBegin[mHeapIdx] + HEAP_SIZE/sizeof(uint32_t);
	mHeapIdx ^= 1;

#ifdef CELL_GCM_DEBUG // {
	gCellGcmDebugCallback = NULL;
#endif // }

	//Set up state for each context
	solidCon.SetDepthTestEnable(CELL_GCM_TRUE);
	solidCon.SetBlendEnable(CELL_GCM_FALSE);
	solidCon.SetLogicOpEnable(CELL_GCM_FALSE);

	transCon.SetDepthTestEnable(CELL_GCM_FALSE);
	transCon.SetBlendEnable(CELL_GCM_TRUE);
	transCon.SetBlendEquation(CELL_GCM_FUNC_ADD, CELL_GCM_FUNC_ADD);
	transCon.SetBlendFunc(CELL_GCM_SRC_ALPHA, CELL_GCM_ONE_MINUS_SRC_ALPHA, CELL_GCM_ZERO, CELL_GCM_ZERO);
	transCon.SetLogicOpEnable(CELL_GCM_FALSE);

	minimCon.SetDepthTestEnable(CELL_GCM_FALSE);
	minimCon.SetBlendEnable(CELL_GCM_TRUE);
	minimCon.SetBlendEquation(CELL_GCM_MIN, CELL_GCM_MIN);
	minimCon.SetLogicOpEnable(CELL_GCM_FALSE);

	//Bucket the objects in the appropriate context
	for(uint32_t i = 0; i < SQUARE_COUNT; i++) 
	{
		CellGcmContext *con;
		if(mSquareTable[i].bucket == 0)
			con = &solidCon;
		else if(mSquareTable[i].bucket == 1)
			con = &transCon;
		else
			con = &minimCon;

		tex.offset = mTextureOffsets[mSquareTable[i].texnum];
		con->SetTexture(mTexUnit, &tex);
		con->SetVertexDataArray(mPosIndex, 0, sizeof(float)*5, 3,
			CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN,
			mVertexTableOffset + sizeof(float)*30 * i);
		con->SetVertexDataArray(mTexIndex, 0, sizeof(float)*5, 2,
			CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, 
			mVertexTableOffset + sizeof(float)*30 * i + sizeof(float)*3);
		con->SetVertexData4f(mColIndex, &mSquareTable[i].r);
		con->SetDrawArrays(CELL_GCM_PRIMITIVE_TRIANGLES, 0, 6);
	}

	//Terminate each context with a return
	solidCon.SetReturnCommand();
	transCon.SetReturnCommand();
	minimCon.SetReturnCommand();

#ifdef CELL_GCM_DEBUG // [
	gCellGcmDebugCallback = cellGcmDebugFinish;
#endif // ]

	//Go back to default command buffer, call each of the three
	//contexts as a subroutine
	uint32_t offset;
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(solidCon.begin, &offset));
	cellGcmSetCallCommand(offset);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(transCon.begin, &offset));
	cellGcmSetCallCommand(offset);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(minimCon.begin, &offset));
	cellGcmSetCallCommand(offset);

	{
		// calc fps
		FWTimeVal	time = FWTime::getCurrentTime();
		float fFPS = 1.f / (float)(time - mLastTime);
		mLastTime = time;

		// print some messages
		FWDebugFont::setPosition(0, 0);
		FWDebugFont::setColor(1.f, 1.f, 1.f, 1.0f);

		FWDebugFont::print("Dice Sample Application\n\n");
		FWDebugFont::printf("FPS: %.2f\n", frame == 0 ? 0 : fFPS);
	}
	frame++;
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
