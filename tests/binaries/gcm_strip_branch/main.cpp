/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#define __CELL_ASSERT__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// system
#include <sys/process.h>
#include <sysutil/sysutil_sysparam.h>
#include <cell/sysmodule.h>

// libgcm
#include <cell/gcm.h>
using namespace cell::Gcm;

// gcm method data for CELL_GCM_METHOD_NOP
#include <cell/gcm/gcm_method_data.h>

// vectormath
#include <vectormath/cpp/vectormath_aos.h>
using namespace Vectormath::Aos;

// file
#include <cell/cell_fs.h>
#include <sys/paths.h>

// using gcmutil
#include "gcmutil.h"
using namespace CellGcmUtil;

// using basic sample template
#include "template/sampleBasic.h"
using namespace CellGcmUtil::SampleBasic;

// sample navigator
#include "snaviutil.h"

// this project
#include "geometry.h"
#include "launcher.h"
#include "fspatch.h"

namespace{
	// sample display settings
	const int32_t COLOR_BUF_NUM = 2;
	CellGcmSurface sSurface[COLOR_BUF_NUM];
	Memory_t sFrameBuffer;
	Memory_t sDepthBuffer;
	CellGcmTexture sFrameTexture[COLOR_BUF_NUM];
	CellGcmTexture sDepthTexture;

	float sDisplayAspectRatio = 0.0f;
	const float MY_PI = 3.14159265f;

	// shader files
	Shader_t sVShader;
	Shader_t sFShader;
	Memory_t sFShaderUCode;
	#define VERTEX_SHADER	SYS_APP_HOME "/vs_multi.vpo"
	#define FRAGMENT_SHADER	SYS_APP_HOME "/fs_multi.fpo"

	// vertex shader constants
	const uint32_t VS_UF_VIEW_MATRIX	= 4;	// uniform float4x4 ViewMatrix  : C4,
	const uint32_t VS_UF_PROJ_MATRIX	= 8;	// uniform float4x4 ProjMatrix  : C8,
	const uint32_t VS_UF_LIGHT_DIR_AMB	= 12;	// uniform float4 lightDir_and_wAmbient : C12,
	const uint32_t VS_UF_MODEL_COLOR	= 13;	// uniform float4 modelColor            : C13,
	const uint32_t VS_UF_MULTI_MODEL_MATRIX	= 16;	// uniform float4 MatrixList[4] : C16,

	// fragment shader constants
	struct MyShaderParam_t{
		uint32_t branchLightMode;		// uniform float4 branchLightMode, 
		uint32_t branchAreaLight;		// uniform float4 branchAreaLight, 
		uint32_t colorAreaLight[4];		// uniform float4 colorAreaLight[4], 
		uint32_t atAreaLight[4];		// uniform float3 atAreaLight[4], 
		uint32_t posAreaLight[4];		// uniform float3 posAreaLight[4], 
	} sFsParam;
	

	const uint32_t AREA_NUM = 4;
	const uint32_t AREA_FLOOR = AREA_NUM;
	const uint32_t VP_CONST_NUM_LIMIT = 128;

	Vector4 sAreaColor[AREA_NUM];
	Vector3 sDefaultLightPos[AREA_NUM];
	Vector3 sLightPos[AREA_NUM];
	Vector3 sLightOrigin[AREA_NUM];
	float sLightFactor = 0.0f;

	bool sShowDbgFont = true;
	bool sMoveLight = true;
	uint32_t sShowMode = 2;
	uint32_t sPatchMode = 1;
	bool sInstansing = true;
	const uint32_t PATCH_MODE_NUM = 2;
	const uint32_t SHOW_MODE_NUM = 5;

	const Vector4 COLOR_CIRCLE	 = Vector4(1.0f, 0.3125f, 0.3125f, 1.0f);
	const Vector4 COLOR_CROSS	 = Vector4(0.59765625f, 0.59765625f, 1.0f, 1.0f);
	const Vector4 COLOR_TRIANGLE = Vector4(0.0f, 0.796875f, 0.59765625f, 1.0f);
	const Vector4 COLOR_SQUARE	 = Vector4(1.0f, 0.59765625f, 0.796875f, 1.0f);

	uint32_t sParamCount = 0;
	PatchParam_t *sParams;
	Memory_t sParamsBuffer, sParamOffsets;

	const uint32_t MY_LABEL_BASE_INDEX = 128;

	const uint32_t SHADER_RING_NUM = AREA_NUM * 2;
	const uint32_t LABEL_USE_SHADER = MY_LABEL_BASE_INDEX;
	uint32_t sShaderRingWritePtr = 0;
	uint32_t sShaderRingPosition[AREA_NUM + 1];
	Memory_t sShaderRing[SHADER_RING_NUM];
	void *sShaderJtsPtr[SHADER_RING_NUM];
	
	uint32_t sStripedUCodeSize[AREA_NUM + 1];

	Memory_t sDrawCommandBuffer[COLOR_BUF_NUM];

	enum _LABEL_STATUS{
		LABEL_STATUS_ACKNOWLEDGE = 0,
		LABEL_STATUS_LOCKED = 1,
		LABEL_STATUS_PREPARED = 2,
		LABEL_STATUS_USING = 3,
	};

} // namespace

uint8_t cellPadUtilRoundPressValue(uint8_t value)
{
	int32_t ret = value;
	if(ret < 16 ) ret = 0;
	if(240 < ret) ret = 255;
	
	return ret;
}

void frameMoveLight(float light_factor)
{
	float factor = (-cos(light_factor) * 0.5f + 0.5f) * 2.0f;
	for(uint32_t area = 0; area < AREA_NUM; ++area){
		sLightPos[area] = sDefaultLightPos[area] * factor;
	}
}

void waitForLabel(uint32_t index, uint32_t value)
{
	volatile uint32_t *ptr = cellGcmGetLabelAddress(index);
	while(*ptr != value){
		sys_timer_usleep(100);
	}
}

void writeLabel(uint32_t index, uint32_t value)
{
	uint32_t *ptr = cellGcmGetLabelAddress(index);
	if(ptr) *ptr = value;
}

void* setJumpToSelf(void)
{
	uint32_t *ptr = cellGcmGetCurrentBuffer();
	uint32_t offset = 0;
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(ptr, &offset));
	cellGcmSetJumpCommandUnsafeInline(gCellGcmCurrentContext, offset);
	//fprintf(stderr, "    set JTS: %08x\n", (uint32_t)ptr);
	return ptr;
}

void releaseJumpToSelf(void* addr)
{
	uint32_t *ptr = reinterpret_cast<uint32_t*>(addr);
	if(ptr){
		__asm__ __volatile__ ("lwsync");
		*ptr = CELL_GCM_METHOD_NOP;
		//fprintf(stderr, "release JTS: %08x\n", (uint32_t)ptr);
	}
}

int32_t userCallback(struct CellGcmContextData* con, uint32_t size)
{
	(void)size;
	(void)con;

	fprintf(stderr, "#error: sDrawCommandBuffer[%d] over flow!!\n", gSampleApp.nFrameIndex);
	
	assert(0);
	exit(-1);

	return CELL_OK;
}

void setDrawBuffer(uint32_t index)
{
	// Jump to DrawCommandBuffer
	cellGcmSetJumpCommand(sDrawCommandBuffer[index].offset);

	// Set CurrentBuffer
	cellGcmSetCurrentBuffer(reinterpret_cast<uint32_t*>(sDrawCommandBuffer[index].addr), sDrawCommandBuffer[index].size);
	cellGcmSetUserCallback(userCallback);
}

bool onInit(void)
{
	// init libgcm and gcmutil
	const int32_t CB_SIZE   = 0x00800000; //   8MB
	const int32_t MAIN_SIZE = 0x08000000; // 128MB
	if(!cellGcmUtilInit(CB_SIZE, MAIN_SIZE)) return false;

	// init display
	CellVideoOutResolution reso = cellGcmUtilGetResolution();
	const uint8_t color_format = CELL_GCM_TEXTURE_A8R8G8B8;
	const uint8_t depth_format = CELL_GCM_TEXTURE_DEPTH24_D8;
	bool bRet = false;
	//bRet = cellGcmUtilCreateSimpleTexture(reso.width, reso.height, CELL_GCM_LOCATION_LOCAL, sFrameTexture, &sFrameBuffer);
	bRet = cellGcmUtilCreateTiledTexture(reso.width, reso.height, color_format, CELL_GCM_LOCATION_LOCAL, CELL_GCM_COMPMODE_C32_2X1, COLOR_BUF_NUM, sFrameTexture, &sFrameBuffer);
	if(bRet == false)
	{
		printf("cellGcmUtilCreateTiledTexture() failed.\n");
		return false;
	}

	bRet = cellGcmUtilCreateDepthTexture(reso.width, reso.height, depth_format, CELL_GCM_LOCATION_LOCAL, CELL_GCM_SURFACE_CENTER_1, true, true, &sDepthTexture, &sDepthBuffer);
	if(bRet == false)
	{
		printf("cellGcmUtilCreateDepthTexture() failed.\n");
		return false;
	}

	// frame texture tu surface
	for(int32_t i = 0; i < COLOR_BUF_NUM; ++i){
		sSurface[i] = cellGcmUtilTextureToSurface(&sFrameTexture[i], &sDepthTexture);
	}

	// regist output buffer
	bRet = cellGcmUtilSetDisplayBuffer(COLOR_BUF_NUM, sFrameTexture);
	if(bRet == false)
	{
		printf("cellGcmUtilSetDisplayBuffer() failed.\n");
		return false;
	}

	// setup sample template
	gSampleApp.nFrameNumber = COLOR_BUF_NUM;
	gSampleApp.nFrameIndex = 0;
	gSampleApp.p_vSurface = sSurface;

	// init shader
	if(!cellGcmUtilLoadShader(CELL_SNAVI_ADJUST_PATH(VERTEX_SHADER), &sVShader)) return false;
	if(!cellGcmUtilLoadShader(CELL_SNAVI_ADJUST_PATH(FRAGMENT_SHADER), &sFShader)) return false;
	if(!cellGcmUtilGetFragmentUCode(&sFShader, CELL_GCM_LOCATION_MAIN, &sFShaderUCode)) return false;

	// prepare fragment patch
	sParamCount = fspatchSetupFragmentPatch(sFShader.program, &sParamsBuffer, &sParamOffsets);
	if(sParamCount == 0) return false;
	sParams = reinterpret_cast<PatchParam_t *>(sParamsBuffer.addr);

	sFsParam.branchLightMode	 =	fspatchGetNamedParamIndex(sFShader.program, "branchLightMode");
	sFsParam.branchAreaLight	 =	fspatchGetNamedParamIndex(sFShader.program, "branchAreaLight");
	sFsParam.colorAreaLight[0]	 =	fspatchGetNamedParamIndex(sFShader.program, "colorAreaLight[0]");
	sFsParam.colorAreaLight[1]	 =	fspatchGetNamedParamIndex(sFShader.program, "colorAreaLight[1]");
	sFsParam.colorAreaLight[2]	 =	fspatchGetNamedParamIndex(sFShader.program, "colorAreaLight[2]");
	sFsParam.colorAreaLight[3]	 =	fspatchGetNamedParamIndex(sFShader.program, "colorAreaLight[3]");
	sFsParam.atAreaLight[0]	 =	fspatchGetNamedParamIndex(sFShader.program, "atAreaLight[0]");
	sFsParam.atAreaLight[1]	 =	fspatchGetNamedParamIndex(sFShader.program, "atAreaLight[1]");
	sFsParam.atAreaLight[2]	 =	fspatchGetNamedParamIndex(sFShader.program, "atAreaLight[2]");
	sFsParam.atAreaLight[3]	 =	fspatchGetNamedParamIndex(sFShader.program, "atAreaLight[3]");
	sFsParam.posAreaLight[0]	 =	fspatchGetNamedParamIndex(sFShader.program, "posAreaLight[0]");
	sFsParam.posAreaLight[1]	 =	fspatchGetNamedParamIndex(sFShader.program, "posAreaLight[1]");
	sFsParam.posAreaLight[2]	 =	fspatchGetNamedParamIndex(sFShader.program, "posAreaLight[2]");
	sFsParam.posAreaLight[3]	 =	fspatchGetNamedParamIndex(sFShader.program, "posAreaLight[3]");

	// init shader ring buffer
	for(uint32_t sring = 0; sring < SHADER_RING_NUM; ++sring)
	{
		if(cellGcmUtilGetFragmentUCode(&sFShader, CELL_GCM_LOCATION_MAIN, &sShaderRing[sring]) == false)
		{
			printf("cellGcmUtilGetFragmentUCode() failed.\n");
			return false;
		}

		sShaderJtsPtr[sring] = 0;
		writeLabel(LABEL_USE_SHADER + sring, LABEL_STATUS_ACKNOWLEDGE);
	}

	// init ring pos
	sShaderRingWritePtr = 0;
	memset(sShaderRingPosition, 0, sizeof(sShaderRingPosition));
	for(uint32_t t = 0; t < AREA_NUM + 1; ++t){
		uint32_t shaderIndex = (++sShaderRingWritePtr) % SHADER_RING_NUM;
		sShaderRingPosition[t] = shaderIndex;
	}
	sShaderRingWritePtr %= SHADER_RING_NUM;

	// init geometries
	if(multiCubeInit() == false) return false;
	if(floorInit() == false) return false;

	// init camera
	cellGcmUtilSimpleCameraInit(Vector3(0.0f, 1.0f, 10.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 2.0f, 0.0f));
	cellGcmUtilSimpleCameraSetParam(Vector3(-75.0f, 40.0f, 75.0f), - MY_PI / 4.0f, - MY_PI / 10.0f);
	sDisplayAspectRatio = cellGcmUtilGetAspectRatio();

	// init other setting
	sAreaColor[0] = COLOR_CIRCLE;
	sAreaColor[1] = COLOR_CROSS;
	sAreaColor[2] = COLOR_TRIANGLE;
	sAreaColor[3] = COLOR_SQUARE;

	sDefaultLightPos[0] = Vector3(30.0f, 0.0f, 30.0f);
	sDefaultLightPos[1] = Vector3(-30.0f, 0.0f, 30.0f);
	sDefaultLightPos[2] = Vector3(30.0f, 0.0f, -30.0f);
	sDefaultLightPos[3] = Vector3(-30.0f, 0.0f, -30.0f);

	sLightOrigin[0] = Vector3(30.0f, 80.0f, 30.0f);
	sLightOrigin[1] = Vector3(-30.0f, 80.0f, 30.0f);
	sLightOrigin[2] = Vector3(30.0f, 80.0f, -30.0f);
	sLightOrigin[3] = Vector3(-30.0f, 80.0f, -30.0f);
	
	sLightFactor = 0.0f;
	frameMoveLight(sLightFactor);

	// init cube launcher
	{
		if(launchInit() == false)
		{
			printf("launchInit() failed.\n");
			return false;
		}
		launchIncrease(256);
	}

	// init DrawCommandBuffer
	{
		const int32_t DRAW_CB_ALIGN = 0x00000080; // 256
		const int32_t DRAW_CB_SIZE  = 0x00400000; // 4MB
		for(int32_t i = 0; i < COLOR_BUF_NUM; ++i){
			if(cellGcmUtilAllocateMain(DRAW_CB_SIZE, DRAW_CB_ALIGN, &sDrawCommandBuffer[i]) == false) return false;
		}
	}

	return true;
}

void onFinish(void)
{
	launchEnd();

	multiCubeEnd();
	floorEnd();

	cellGcmUtilDestroyShader(&sVShader);
	cellGcmUtilDestroyShader(&sFShader);
	cellGcmUtilFree(&sFShaderUCode);

	for(int32_t i = 0; i < COLOR_BUF_NUM; ++i){
		cellGcmUtilFree(&sDrawCommandBuffer[i]);
	}
	cellGcmUtilFree(&sFrameBuffer);
	cellGcmUtilFree(&sDepthBuffer);

	for(uint32_t sring = 0; sring < SHADER_RING_NUM; ++sring){
		cellGcmUtilFree(&sShaderRing[sring]);
	}

	cellGcmUtilFree(&sParamsBuffer);
	cellGcmUtilFree(&sParamOffsets);
}

int f = 0;

void iterate() {
	int z = 0;
	for (int b = 0; b < 2; ++b) {
		for (int c = 0; c < 2; ++c) {
			for (int d = 0; d < 5; ++d) {
				sInstansing = b;
				sPatchMode = c;
				sShowMode = d;
				if (z++ == f) {
					f++;
					return;
				}
			}
		}
	}
}

void onUpdate(void)
{
	iterate();

	if(cellPadUtilButtonPressedOnce(0, CELL_UTIL_BUTTON_SELECT)){
		sShowDbgFont = !sShowDbgFont;
	}

	uint8_t incv = cellPadUtilRoundPressValue(cellPadUtilGetButtonPressValue(0, CELL_UTIL_BUTTON_R2));
	uint8_t decv = cellPadUtilRoundPressValue(cellPadUtilGetButtonPressValue(0, CELL_UTIL_BUTTON_L2));
	uint32_t i_num = 0;
	uint32_t d_num = 0;

	uint32_t high_rate = (gSampleApp.isPause? 16: 1);
	if(incv == 255){
		i_num = 8 * high_rate;
	}else if(incv > 96){
		i_num = 4 * high_rate;
	}else if(incv > 0){
		i_num = 2 * high_rate;
	}
	if(decv == 255){
		d_num = 8 * high_rate;
	}else if(decv > 96){
		d_num = 4 * high_rate;
	}else if(decv > 0){
		d_num = 2 * high_rate;
	}
	launchIncrease(i_num);
	launchDecrease(d_num);


	if(cellPadUtilButtonPressedOnce(0, CELL_UTIL_BUTTON_CIRCLE)){
		sMoveLight = !sMoveLight;
	}
	if(cellPadUtilButtonPressedOnce(0, CELL_UTIL_BUTTON_CROSS)){
		sInstansing = !sInstansing;
	}
	if(cellPadUtilButtonPressedOnce(0, CELL_UTIL_BUTTON_SQUARE)){
		++sPatchMode;
		sPatchMode %= PATCH_MODE_NUM;
	}
	if(cellPadUtilButtonPressedOnce(0, CELL_UTIL_BUTTON_TRIANGLE)){
		++sShowMode;
		sShowMode %= SHOW_MODE_NUM;
	}

	cellGcmUtilSimpleCameraUpdate();
}

void prepareFragmentShader()
{
	void* srcUCode = sFShaderUCode.addr;
	uint32_t srcSize = sFShaderUCode.size;

	// setup source shader
	float branchLightMode[4] = {(sShowMode > 1? 1.0f: 0.0f), (sShowMode == 3? 1.0f: 0.0f), (sShowMode == 4? 1.0f: 0.0f), 0.0f};
	{
		float branchAreaLight[4] = {0.0f, 0.0f, 0.0f, 0.0f};

		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.branchLightMode], branchLightMode);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.branchAreaLight], branchAreaLight);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.colorAreaLight[0]], (float*)&sAreaColor[0]);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.colorAreaLight[1]], (float*)&sAreaColor[1]);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.colorAreaLight[2]], (float*)&sAreaColor[2]);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.colorAreaLight[3]], (float*)&sAreaColor[3]);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.atAreaLight[0]], (float*)&sLightPos[0]);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.atAreaLight[1]], (float*)&sLightPos[1]);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.atAreaLight[2]], (float*)&sLightPos[2]);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.atAreaLight[3]], (float*)&sLightPos[3]);

		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.posAreaLight[0]], (float*)&sLightOrigin[0]);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.posAreaLight[1]], (float*)&sLightOrigin[1]);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.posAreaLight[2]], (float*)&sLightOrigin[2]);
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.posAreaLight[3]], (float*)&sLightOrigin[3]);
	}

	for(uint32_t a = 0; a < AREA_NUM + 1; ++a)
	{
		// check order in [floor, area0, area1, area2, area3]
		uint32_t area = (a == 0? AREA_FLOOR: a - 1);
		uint32_t shaderIndex = sShaderRingPosition[area];

		// wait for shader 
		waitForLabel(LABEL_USE_SHADER + shaderIndex, LABEL_STATUS_ACKNOWLEDGE);
		writeLabel(LABEL_USE_SHADER + shaderIndex, LABEL_STATUS_LOCKED);

		// constant patch
		void* ucode = sShaderRing[shaderIndex].addr;
		uint32_t size = sShaderRing[shaderIndex].size;
					
		float branchAreaLight[AREA_NUM + 1] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
		branchAreaLight[area] = 1.0f;
		fspatchSetPatchParamDirect(srcUCode, &sParams[sFsParam.branchAreaLight], branchAreaLight);

		// strip branches
		if(sPatchMode != 0){
			uint32_t stripSize = size;
			if(cellGcmCgStripBranchesFromFragmentUCode(srcUCode, size, ucode, &stripSize) != CELL_OK)
			{
				// failed...
				memcpy(ucode, srcUCode, srcSize);
			}
			sStripedUCodeSize[area] = stripSize;
		}else{
			memcpy(ucode, srcUCode, srcSize);
		}

		if(sShaderJtsPtr[shaderIndex] != 0){
			releaseJumpToSelf(sShaderJtsPtr[shaderIndex]);
			sShaderJtsPtr[shaderIndex] = 0;
		}

		writeLabel(LABEL_USE_SHADER + shaderIndex, LABEL_STATUS_PREPARED);
	}

	// refresh ring pos
	for(uint32_t t = 0; t < AREA_NUM + 1; ++t){
		uint32_t shaderIndex = (++sShaderRingWritePtr) % SHADER_RING_NUM;
		sShaderRingPosition[t] = shaderIndex;
	}
	sShaderRingWritePtr %= SHADER_RING_NUM;
}

int x = 0;

void onFrame(void)
{
	if (x == 2)
		return;
	x++;

	if(!gSampleApp.isPause && !gSampleApp.isSysMenu){
		launchUpdate();
	}

	if(sMoveLight && !gSampleApp.isSysMenu){
		frameMoveLight(sLightFactor);
		sLightFactor += 1.5f;
	}
}

void setDrawEnv(void)
{
	// depth test
	cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
	cellGcmSetDepthFunc(CELL_GCM_LESS);
	
	// blend op
	cellGcmSetBlendEnable(CELL_GCM_TRUE);
	cellGcmSetBlendEquation(CELL_GCM_FUNC_ADD, CELL_GCM_FUNC_ADD);
	cellGcmSetBlendFunc(CELL_GCM_SRC_ALPHA, CELL_GCM_ONE_MINUS_SRC_ALPHA, CELL_GCM_ONE, CELL_GCM_ONE_MINUS_SRC_ALPHA);

	// set clear color
	cellGcmSetClearColor(0xFF000000);

	// set cull mode
	cellGcmSetFrontFace(CELL_GCM_CCW);
	cellGcmSetCullFace(CELL_GCM_BACK);
	cellGcmSetCullFaceEnable(CELL_GCM_TRUE);
	cellGcmSetZMinMaxControl(CELL_GCM_TRUE, CELL_GCM_TRUE, CELL_GCM_FALSE);
		
	// set view port
	Viewport_t vp = cellGcmUtilGetViewportGL(sSurface[0].height, 0, 0, sSurface[0].width, sSurface[0].height, 0.0f, 1.0f);
	cellGcmSetViewport(vp.x, vp.y, vp.width, vp.height, vp.min, vp.max, vp.scale, vp.offset);
	cellGcmSetScissor(0, 0, sSurface[0].width, sSurface[0].height);
	
	// clear surface
	cellGcmSetClearSurface(	CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_A |
							CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B);
}

void drawMultiCube(uint32_t num, float* buffer)
{
	if(num == 0) return;
	if(buffer == 0) return;

	float* ptr = buffer;
	uint32_t rest = num;
	const uint32_t STRIDE = VP_CONST_NUM_LIMIT * 12;

	while(rest > VP_CONST_NUM_LIMIT){
		cellGcmSetVertexProgramConstants(VS_UF_MULTI_MODEL_MATRIX, STRIDE, ptr);
		multiCubeDraw(VP_CONST_NUM_LIMIT);
		ptr += STRIDE;
		rest -= VP_CONST_NUM_LIMIT;
	}
	if(rest > 0){
		cellGcmSetVertexProgramConstants(VS_UF_MULTI_MODEL_MATRIX, rest * 12, ptr);
		multiCubeDraw(rest);
	}
}

void onDraw()
{
	setDrawBuffer(gSampleApp.nFrameIndex);

	// set state
	setDrawEnv();

	// view settings
	Matrix4 proj = transpose(Matrix4::perspective(cellGcmUtilToRadian(45.0f), sDisplayAspectRatio, 1.0f, 1000.0f));
	Matrix4 view = transpose(cellGcmUtilSimpleCameraGetMatrix());
	Vector3 light_dir = normalize(Vector3(0.5f, 0.4f, 1.0f));
	float light_ambient = 0.2f;

	// vertex shader
	{
		cellGcmSetVertexProgram(sVShader.program, sVShader.ucode);

		Vector4 lightDir_and_wAmbient(light_dir[0], light_dir[1], light_dir[2], light_ambient);
		cellGcmSetVertexProgramConstants(VS_UF_LIGHT_DIR_AMB, 4, reinterpret_cast<float*>(&lightDir_and_wAmbient));
		cellGcmSetVertexProgramConstants(VS_UF_VIEW_MATRIX, 16, reinterpret_cast<const float*>(&view));
		cellGcmSetVertexProgramConstants(VS_UF_PROJ_MATRIX, 16, reinterpret_cast<const float*>(&proj));
	}

	// fagment shader
	{
		cellGcmSetFragmentProgramOffset(sFShader.program, sFShaderUCode.offset, sFShaderUCode.location);
		cellGcmSetFragmentProgramControl(sFShader.program, CELL_GCM_TRUE, 1, 0);
	}

	// draw floor
	{
		uint32_t shaderIndex = sShaderRingPosition[AREA_FLOOR];
		sShaderJtsPtr[shaderIndex] = setJumpToSelf();
		cellGcmSetWriteTextureLabel(LABEL_USE_SHADER + shaderIndex, LABEL_STATUS_USING);		
		cellGcmSetUpdateFragmentProgramParameterLocation(sShaderRing[shaderIndex].offset, sShaderRing[shaderIndex].location);
		//cellGcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_TEXTURE);

		Vector4 modelColor = Vector4(0.7f, 0.7f, 0.7f, 1.0f);
		Matrix4 modelMatrix = Matrix4::scale(Vector3(100.0f, 1.0f, 100.0f));
		cellGcmSetVertexProgramConstants(VS_UF_MODEL_COLOR, 4, reinterpret_cast<float*>(&modelColor));
		cellGcmSetVertexProgramConstants(VS_UF_MULTI_MODEL_MATRIX, 3 * 4, reinterpret_cast<float*>(&modelMatrix));

		floorDraw();

		cellGcmSetWriteTextureLabel(LABEL_USE_SHADER + shaderIndex, LABEL_STATUS_ACKNOWLEDGE);
	}

	// draw dices
	{
		multiCubeDrawBegin();

		for(uint32_t area = 0; area < AREA_NUM; ++area)
		{
			uint32_t shaderIndex = sShaderRingPosition[area];
			sShaderJtsPtr[shaderIndex] = setJumpToSelf();
			cellGcmSetWriteTextureLabel(LABEL_USE_SHADER + shaderIndex, LABEL_STATUS_USING);		
			cellGcmSetUpdateFragmentProgramParameterLocation(sShaderRing[shaderIndex].offset, sShaderRing[shaderIndex].location);
			cellGcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_TEXTURE);

			uint32_t num = launchGetNumber(area);
			float *buffer = launchGetBuffer(area);

			Vector4 modelColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
			if(sShowMode == 1){
				modelColor = sAreaColor[area];
			}
			cellGcmSetVertexProgramConstants(VS_UF_MODEL_COLOR, 4, reinterpret_cast<float*>(&modelColor));

			if(sInstansing){
				drawMultiCube(num, buffer);
			}else{
				for(uint32_t dice = 0; dice < num; ++dice){
					cellGcmSetVertexProgramConstants(VS_UF_MULTI_MODEL_MATRIX, 12, &buffer[12 * dice]);
					multiCubeDraw(1);
				}
			}

			cellGcmSetWriteTextureLabel(LABEL_USE_SHADER + shaderIndex, LABEL_STATUS_ACKNOWLEDGE);
		}

		multiCubeDrawEnd();
	}

	cellGcmFlush();

	prepareFragmentShader();
}

void onDbgfont(void)
{
	if(!sShowDbgFont) return;

	cellGcmUtilSetPrintSize(0.75f);
	cellGcmUtilSetPrintPos(0.04f, 0.04f);
	cellGcmUtilSetPrintColor(0xffffffff);
	cellGcmUtilPrintf("GCM Graphics Sample\n");

	cellGcmUtilSetPrintSize(0.5f);
	cellGcmUtilSetPrintPos(0.05f, CELL_GCMUTIL_POSFIX);

	const char* const SAMPLE_NAME = "Fragment Strip Branch";
	if(!gSampleApp.isPause && !gSampleApp.isSysMenu){
		cellGcmUtilPrintf("%s\n", SAMPLE_NAME);
	}else if(gSampleApp.isSysMenu){
		cellGcmUtilPrintf("%s: In Game XMB\n", SAMPLE_NAME);
	}else if(gSampleApp.isPause){
		cellGcmUtilPrintf("%s: PAUSE\n", SAMPLE_NAME);
	}

	const char* patch_mode[] = {
		"BRANCH", 
		"STRIP ON PPU", 
	};
	const char* show_mode[] = {
		"DEFAULT VIEW", 
		"AREA VIEW", 
		"SPOT LIGHT", 
		"PAINT LIGHT", 
		"FLUOR LIGHT", 
	};
	cellGcmUtilPrintf(" [Mode]\n");
	cellGcmUtilPrintf("   MoveLight: %s\n", (sMoveLight? "ON": "OFF"));
	cellGcmUtilPrintf("  Instansing: %s\n", (sInstansing? "ON": "OFF"));
	cellGcmUtilPrintf("  Patch Mode: %s\n", patch_mode[sPatchMode]);
	cellGcmUtilPrintf("  Light Mode: %s\n", show_mode[sShowMode]);

	Vector3 t;
	float y, p;
	cellGcmUtilSimpleCameraGetParam(&t, &y, &p);
	cellGcmUtilPrintf("  Camera: (%.02f %.02f %.02f)-(%.02f %.02f)\n", (float)t[0], (float)t[1], (float)t[2], y, p);

	{
		cellGcmUtilPrintf(" [Objects Info]\n");
		uint32_t dice_num = 0;	
		for(uint32_t area = 0; area < AREA_NUM; ++area){
			dice_num += launchGetNumber(area);
		}
		cellGcmUtilPrintf("  dice = %d\n", dice_num);
		for(int32_t area = 0; area < (int32_t)AREA_NUM; ++area){
			cellGcmUtilPrintf("   [%d] %d\n", area, launchGetNumber(area));
		}
	}

	{
		cellGcmUtilPrintf(" [Fragment Info]\n");
		if(sPatchMode != 0){
			cellGcmUtilPrintf("  Mode = Strip Branches\n");
			
			cellGcmUtilPrintf("  original = %d inst\n", sFShaderUCode.size / 16);
			cellGcmUtilPrintf("   [F] %d\n", sStripedUCodeSize[AREA_NUM] / 16);
			for(uint32_t area = 0; area < AREA_NUM; ++area){
				cellGcmUtilPrintf("   [%d] %d\n", area, sStripedUCodeSize[area] / 16);
			}
		}else{
			cellGcmUtilPrintf("  Mode = Branch\n");
			cellGcmUtilPrintf("  original = %d inst\n", sFShaderUCode.size / 16);
		}
	}

	float dx(0.0f), dy(0.0f);
	cellGcmUtilGetPrintPos(&dx, &dy);

	cellGcmUtilSetPrintPos(CELL_GCMUTIL_POSFIX, 0.85f);
	cellGcmUtilPrintf(" [How to Operate]\n");
	cellGcmUtilPrintf("     L2/R2: Dec/Inc dices\n");
	cellGcmUtilPrintf("    Circle: %s Lights\n", (sMoveLight? "Stop": "Move"));
	cellGcmUtilPrintf("     Cross: Switch Instancing Mode\n");
	cellGcmUtilPrintf("  Triangle: Change Lighting Mode\n");
	cellGcmUtilPrintf("    Square: Change Strip Branch Mode\n");

	cellGcmUtilSetPrintPos(dx, dy);

	// draw perf
	if(!gSampleApp.isSysMenu){
		sampleDrawSimplePerf();
	}
}


SYS_PROCESS_PARAM(1001, 0x10000);

int main(int argc, char *argv[])
{
	return sampleMain(argc, argv);
}
