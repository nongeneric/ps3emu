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


#include <sys/process.h>
#include <cell/sysmodule.h>

// libgcm
#include <cell/gcm.h>
#include <sysutil/sysutil_sysparam.h>

// libdbgfont
#include <cell/dbgfont.h>

// using vectormath
#include <vectormath/cpp/vectormath_aos.h>
using namespace Vectormath::Aos;

// for file
#include <cell/cell_fs.h>
#include <sys/paths.h>

// using gcmutil
#include "gcmutil.h"
using namespace CellGcmUtil;

// sample navigator
#include "snaviutil.h"

// this project
#include "geometry.h"

using namespace cell::Gcm;

namespace{
	Memory_t sCubeVertex;
	CellGcmTexture sCubeTex;
	Memory_t sCubeTexImage;
	
	Memory_t sFloorVertex;
	CellGcmTexture sFloorTex;
	Memory_t sFloorTexImage;

	Memory_t sMultiCubeVertex;
	Memory_t sMultiCubeIndex;
	CellGcmTexture sMultiCubeTex;
	Memory_t sMultiCubeTexImage;

	// cube geometry data
	struct MyVertex_t
	{
		float x, y, z;
		float nx, ny, nz;		
		float u, v;

		MyVertex_t(): x(0.0f), y(0.0f), z(0.0f), nx(0.0f), ny(0.0f), nz(0.0f), u(0.0f), v(0.0f){}
	};
	const uint32_t VTX_NUMBER = 24;
	const int32_t VTX_POSITION_LIST[VTX_NUMBER][3] = {
		{ 1, 1, 1}, {-1, 1, 1}, {-1,-1, 1}, { 1,-1, 1},
		{ 1,-1, 1}, {-1,-1, 1}, {-1,-1,-1}, { 1,-1,-1},
		{ 1, 1, 1}, { 1,-1, 1}, { 1,-1,-1}, { 1, 1,-1},
		{-1,-1, 1}, {-1, 1, 1}, {-1, 1,-1}, {-1,-1,-1},
		{-1, 1, 1}, { 1, 1, 1}, { 1, 1,-1}, {-1, 1,-1},
		{-1, 1,-1}, { 1, 1,-1}, { 1,-1,-1}, {-1,-1,-1}
	};
	const float VTX_NORMAL_LIST[6][3] = {
		{ 0, 0, 1}, { 0,-1, 0}, { 1, 0, 0},
		{-1, 0, 0}, { 0, 1, 0}, { 0, 0,-1}
	};
	const float VTX_TEX_U_LIST[4] = {1.0, 0.0, 0.0, 1.0};
	const float VTX_TEX_V_LIST[4] = {0.0, 0.0, 1.0, 1.0};

} // namespace

bool cubeInit(void)
{
	if(cellGcmUtilAllocateLocal(VTX_NUMBER * sizeof(MyVertex_t), 128, &sCubeVertex) == false) return false;
	memset(sCubeVertex.addr, 0, VTX_NUMBER * sizeof(MyVertex_t));
	
	MyVertex_t *vertices = (MyVertex_t*)sCubeVertex.addr;
	
	for(uint32_t dice = 0; dice < 6; ++dice){
		for(uint32_t i = 0; i < 4; ++i){
			uint32_t index = dice * 4 + i;
			MyVertex_t &v = vertices[index];

			v.x = VTX_POSITION_LIST[index][0];
			v.y = VTX_POSITION_LIST[index][1];
			v.z = VTX_POSITION_LIST[index][2] ;
			
			v.nx = VTX_NORMAL_LIST[dice][0];
			v.ny = VTX_NORMAL_LIST[dice][1];
			v.nz = VTX_NORMAL_LIST[dice][2];
			
			v.u = (VTX_TEX_U_LIST[i] + (dice % 3)) / 3.0f;
			v.v = (VTX_TEX_V_LIST[i] + (dice / 3)) / 3.0f;
		}
	}

	cellGcmUtilLoadTexture(CELL_SNAVI_ADJUST_PATH(SYS_APP_HOME "/dice.gtf"), CELL_GCM_LOCATION_MAIN, &sCubeTex, &sCubeTexImage);

	return true;
}
void cubeEnd(void)
{
	cellGcmUtilFree(&sCubeVertex);
	cellGcmUtilFree(&sCubeTexImage);
}
void cubeDraw(void)
{
	// begine
	{
		// setup vertex attribute
		cellGcmSetInvalidateVertexCache();
		cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_POSITION,  0, sizeof(MyVertex_t), 3, CELL_GCM_VERTEX_F, sCubeVertex.location, sCubeVertex.offset);
		cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_NORMAL,    0, sizeof(MyVertex_t), 3, CELL_GCM_VERTEX_F, sCubeVertex.location, sCubeVertex.offset + sizeof(float) * 3);
		cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_TEXCOORD0, 0, sizeof(MyVertex_t), 2, CELL_GCM_VERTEX_F, sCubeVertex.location, sCubeVertex.offset + sizeof(float) * 6);

		// set texture
		cellGcmUtilSetTextureUnit(0, &sCubeTex);
	}

	// draw
	{
		cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_QUADS, 0, VTX_NUMBER);
	}

	// end
	{
		// invalidate texture
		cellGcmUtilInvalidateTextureUnit(0);

		// invalidate vertex attribute
		cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_POSITION,  0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
		cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_NORMAL,    0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
		cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_TEXCOORD0, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	}
}
bool floorInit(void)
{
	return cellGcmUtilLoadTexture(CELL_SNAVI_ADJUST_PATH(SYS_APP_HOME "/floor.gtf"), CELL_GCM_LOCATION_MAIN, &sFloorTex, &sFloorTexImage);
}
void floorEnd(void)
{
	cellGcmUtilFree(&sFloorTexImage);
}
void floorDraw(void)
{
	const uint32_t MY_FLOOR_VERTEX_NUMBER = 4;

	struct MyFloorVertex_t
	{
		float x, y, z;
		float nx, ny, nz;		
		float u, v;
		float index;
	} floor[MY_FLOOR_VERTEX_NUMBER] = {
		/*  x ,   y ,    z ,  nx ,  ny ,  nz ,    u ,    v , index */
		{ 1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 10.0f,  0.0f, 0.0f}, 
		{ 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,  0.0f,  0.0f, 0.0f}, 
		{-1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,  0.0f, 10.0f, 0.0f}, 
		{-1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 10.0f, 10.0f, 0.0f}, 
	};
	
	cellGcmSetInvalidateVertexCache();

	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_POSITION,	0, sizeof(MyFloorVertex_t), 3, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_NORMAL,		0, sizeof(MyFloorVertex_t), 3, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_TEXCOORD0,	0, sizeof(MyFloorVertex_t), 2, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_TEXCOORD1,	0, sizeof(MyFloorVertex_t), 1, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

	// set texture
	cellGcmUtilSetTextureUnit(0, &sFloorTex);

	const int MY_FLOOR_WORD_SIZE = sizeof(MyFloorVertex_t) / sizeof(float);
	cellGcmSetDrawInlineArray(CELL_GCM_PRIMITIVE_QUADS, MY_FLOOR_VERTEX_NUMBER * MY_FLOOR_WORD_SIZE, floor);

	// invalidate texture
	cellGcmUtilInvalidateTextureUnit(0);

	// invalidate vertex attribute
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_POSITION,  0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_NORMAL,    0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_TEXCOORD0, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_TEXCOORD1, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
}

bool multiCubeInit(void)
{
	if(cellGcmUtilAllocateLocal(VTX_NUMBER * sizeof(MyVertex_t), 128, &sMultiCubeVertex) == false) return false;
	memset(sMultiCubeVertex.addr, 0, VTX_NUMBER * sizeof(MyVertex_t));
	
	MyVertex_t *vertices = (MyVertex_t*)sMultiCubeVertex.addr;
	
	for(uint32_t dice = 0; dice < 6; ++dice){
		for(uint32_t i = 0; i < 4; ++i){
			uint32_t index = dice * 4 + i;
			MyVertex_t &v = vertices[index];

			v.x = VTX_POSITION_LIST[index][0];
			v.y = VTX_POSITION_LIST[index][1];
			v.z = VTX_POSITION_LIST[index][2] ;
			
			v.nx = VTX_NORMAL_LIST[dice][0];
			v.ny = VTX_NORMAL_LIST[dice][1];
			v.nz = VTX_NORMAL_LIST[dice][2];
			
			v.u = (VTX_TEX_U_LIST[i] + (dice % 3)) / 3.0f;
			v.v = (VTX_TEX_V_LIST[i] + (dice / 3)) / 3.0f;
		}
	}

	if(cellGcmUtilLoadTexture(CELL_SNAVI_ADJUST_PATH(SYS_APP_HOME "/dice.gtf"), CELL_GCM_LOCATION_MAIN, &sMultiCubeTex, &sMultiCubeTexImage) == false) return false;

	if(cellGcmUtilAllocateLocal(128 * sizeof(float), 128, &sMultiCubeIndex) == false) return false;

	float value = 0.0f;
	float *indecs = (float*)sMultiCubeIndex.addr;
	for(uint32_t i = 0; i < 128; ++i){
		indecs[i] = value;
		value += 1.0f;
	}

	return true;

}
void multiCubeEnd(void)
{
	cellGcmUtilFree(&sMultiCubeVertex);
	cellGcmUtilFree(&sMultiCubeIndex);
	cellGcmUtilFree(&sMultiCubeTexImage);
}
void multiCubeDrawBegin(void)
{
	uint32_t location = sMultiCubeVertex.location;
	uint32_t offset = sMultiCubeVertex.offset;

	// setup vertex attribute
	cellGcmSetInvalidateVertexCache();
	
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_POSITION,	24, sizeof(MyVertex_t), 3, CELL_GCM_VERTEX_F, location, offset);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_NORMAL,		24, sizeof(MyVertex_t), 3, CELL_GCM_VERTEX_F, location, offset + sizeof(float) * 3);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_TEXCOORD0,	24, sizeof(MyVertex_t), 2, CELL_GCM_VERTEX_F, location, offset + sizeof(float) * 6);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_TEXCOORD1,	24, sizeof(float), 1, CELL_GCM_VERTEX_F, sMultiCubeIndex.location, sMultiCubeIndex.offset);

	cellGcmSetFrequencyDividerOperation(
		CELL_GCM_FREQUENCY_MODULO << CELL_GCMUTIL_ATTR_POSITION |
		CELL_GCM_FREQUENCY_MODULO << CELL_GCMUTIL_ATTR_NORMAL |
		CELL_GCM_FREQUENCY_MODULO << CELL_GCMUTIL_ATTR_TEXCOORD0 |
		CELL_GCM_FREQUENCY_DIVIDE << CELL_GCMUTIL_ATTR_TEXCOORD1
	);

	// set texture
	cellGcmUtilSetTextureUnit(0, &sMultiCubeTex);
}
void multiCubeDraw(uint32_t number)
{
	cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_QUADS, 0, VTX_NUMBER * number);
}
void multiCubeDrawEnd(void)
{
	// invalidate texture
	cellGcmUtilInvalidateTextureUnit(0);

	// invalidate vertex attribute
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_POSITION,  0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_NORMAL,    0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_TEXCOORD0, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	cellGcmSetVertexDataArray(CELL_GCMUTIL_ATTR_TEXCOORD1, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
}
