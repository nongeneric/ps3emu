/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2009 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#define __CELL_ASSERT__

#include <stdio.h>
#include <assert.h>
#include <sys/sys_time.h>
#include <cell/sysmodule.h>
#include <cell/gcm.h>
#include <vectormath/cpp/vectormath_aos.h>
#include <sys/paths.h>

#include "cellutil.h"
#include "gcmutil.h"
#include "Test_VertexData.h"

#include "gtfloader.h"

#include "snaviutil.h"

using namespace cell::Gcm;
using namespace Vectormath::Aos;
using namespace Test_Scene;

// Export user's main function.
// userMain() will be executed after PRX modules are loaded.
//
extern "C" int32_t userMain(void);

// clean up function called at exit
static void clean_up(void);

#define PI 3.14159265358979
#define PI2 ((PI)+(PI))

#define STATE_BUFSIZE (0x100000)  // 1MB 

static const char* GTF_FILE = GCM_SAMPLE_DATA_PATH "/human/normalMap.gtf";

struct AttributeContainer {
	void*    address;
	uint32_t offset;
	uint32_t count;
	uint32_t size;
};

struct ShaderContainer {
	CGprogram vertexProgram;
	CGprogram fragmentProgram;

	void*     vertexProgramUcode;
	void*     fragmentProgramUcode;

	uint32_t  fragmentProgramOffset;

	CGparameter ModelMatrixParameter; 
	CGparameter ViewProjMatrixParameter;

	CGparameter LightPosParameter;   
	CGparameter LightColParameter;
	CGparameter AmbientParameter;
	CGparameter EyePosLocalParameter;

	uint32_t    PositionIndex;
	uint32_t    NormalIndex;
	uint32_t    TexcoordIndex;
};

// shader x 3
extern struct _CGprogram _binary_shaders_vp_vertexlighting_vpo_start;
extern struct _CGprogram _binary_shaders_fp_vertexlighting_fpo_start;
extern struct _CGprogram _binary_shaders_vp_fragmentlighting_vpo_start;
extern struct _CGprogram _binary_shaders_fp_fragmentlighting_fpo_start;
extern struct _CGprogram _binary_shaders_vp_normalmap_vpo_start;
extern struct _CGprogram _binary_shaders_fp_normalmap_fpo_start;

ShaderContainer   sVertexLightingContainer;
ShaderContainer   sFragmentLightingContainer;
ShaderContainer   sNormalMapContainer;

static Matrix4 sViewProjMatrix;

// for animation
static Matrix4 sRotateMatrix;
static Matrix4 sInvRotateMatrix;

static Vector4 sLightPos;
static Vector4 sLightCol;
static Vector4 sAmbient;
static Vector4 sEyePos;

static AttributeContainer sVertexBuffer;
static AttributeContainer sNormalBuffer;
static AttributeContainer sColorBuffer;
static AttributeContainer sTexcoordBuffer0;
static AttributeContainer sTexcoordBuffer1;

static void*      sStateBuffer;           // static command buffer to 
static uint32_t   sStateOffset;           // save default state setting


// textures
CGresource        sTexUnit;
CellGtfFileHeader sGtfHeader;
CellGcmTexture    sNormalMap;
void*             sNormalMapAddress;
uint32_t          sNormalMapSize;
//uint32_t sNormalMapOffset;

static bool setupShaderContainer(ShaderContainer * container)
{

	// allocate video memory for fragment program
	unsigned int ucodeSize;
	void *ucode;

	// init
	cellGcmCgInitProgram(container->vertexProgram);
	cellGcmCgInitProgram(container->fragmentProgram);

	cellGcmCgGetUCode(container->fragmentProgram, &ucode, &ucodeSize);

	container->fragmentProgramUcode
		= (void*)cellGcmUtilAllocateLocalMemory(ucodeSize, CELL_GCM_FRAGMENT_UCODE_LOCAL_ALIGN_OFFSET);
	if( container->fragmentProgramUcode == NULL ) return false;

	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(container->fragmentProgramUcode, 
	                             &container->fragmentProgramOffset));

	// copy micro code to local memory
	memcpy(container->fragmentProgramUcode, ucode, ucodeSize); 

	// get and copy 
	cellGcmCgGetUCode(container->vertexProgram, &ucode, &ucodeSize);
	container->vertexProgramUcode = ucode;

	return true;
}

static bool initShader(void)
{
	sVertexLightingContainer.vertexProgram   = 
	           &_binary_shaders_vp_vertexlighting_vpo_start;
	sVertexLightingContainer.fragmentProgram = 
	           &_binary_shaders_fp_vertexlighting_fpo_start;
	sFragmentLightingContainer.vertexProgram   = 
	           &_binary_shaders_vp_fragmentlighting_vpo_start;
	sFragmentLightingContainer.fragmentProgram = 
	           &_binary_shaders_fp_fragmentlighting_fpo_start;
	sNormalMapContainer.vertexProgram   = 
	           &_binary_shaders_vp_normalmap_vpo_start;
	sNormalMapContainer.fragmentProgram = 
	           &_binary_shaders_fp_normalmap_fpo_start;

	// initialize all 3 shaders
	if(setupShaderContainer(&sVertexLightingContainer) != true)
		return false;
	if(setupShaderContainer(&sFragmentLightingContainer) != true)
		return false;
	if(setupShaderContainer(&sNormalMapContainer) != true)
		return false;


	return true;
}

static bool setVertexAttributes(void)
{
	// vertices
	sVertexBuffer.count   = gNumElements_gData_Mesh_Human_Normal01_obj_PolygonGroup00_VERTEX_00;
	sVertexBuffer.size    = sVertexBuffer.count * (sizeof(float)*3);
	sVertexBuffer.address = cellGcmUtilAllocateLocalMemory(sVertexBuffer.size, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(sVertexBuffer.address, &sVertexBuffer.offset));
	memcpy(sVertexBuffer.address, gData_Mesh_Human_Normal01_obj_PolygonGroup00_VERTEX_00, sVertexBuffer.size);

	// normals
	sNormalBuffer.count   = gNumElements_gData_Mesh_Human_Normal01_obj_PolygonGroup00_NORMAL_01;
	sNormalBuffer.size    = sNormalBuffer.count * (sizeof(float)*3);
	sNormalBuffer.address = cellGcmUtilAllocateLocalMemory(sNormalBuffer.size, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(sNormalBuffer.address, &sNormalBuffer.offset));
	memcpy(sNormalBuffer.address, gData_Mesh_Human_Normal01_obj_PolygonGroup00_NORMAL_01, sNormalBuffer.size);

	// colors
	sColorBuffer.count   = gNumElements_gData_Mesh_Human_Normal01_obj_PolygonGroup00_COLOR_02;
	sColorBuffer.size    = sColorBuffer.count * (sizeof(float)*3);
	sColorBuffer.address = cellGcmUtilAllocateLocalMemory(sColorBuffer.size, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(sColorBuffer.address, &sColorBuffer.offset));
	memcpy(sColorBuffer.address, gData_Mesh_Human_Normal01_obj_PolygonGroup00_COLOR_02, sColorBuffer.size);

	// texcoords 0
	sTexcoordBuffer0.count   = gNumElements_gData_Mesh_Human_Normal01_obj_PolygonGroup00_TEXCOORD_04;
	sTexcoordBuffer0.size    = sTexcoordBuffer0.count * (sizeof(float)*2);
	sTexcoordBuffer0.address = cellGcmUtilAllocateLocalMemory(sTexcoordBuffer0.size, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(sTexcoordBuffer0.address, &sTexcoordBuffer0.offset));
	memcpy(sTexcoordBuffer0.address, gData_Mesh_Human_Normal01_obj_PolygonGroup00_TEXCOORD_04, sTexcoordBuffer0.size);

	// texcoords 1
	sTexcoordBuffer1.count   = gNumElements_gData_Mesh_Human_Normal01_obj_PolygonGroup00_TEXCOORD_05;
	sTexcoordBuffer1.size    = sTexcoordBuffer1.count * (sizeof(float)*2);
	sTexcoordBuffer1.address = cellGcmUtilAllocateLocalMemory(sTexcoordBuffer1.size, 128);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(sTexcoordBuffer1.address, &sTexcoordBuffer1.offset));
	memcpy(sTexcoordBuffer1.address, gData_Mesh_Human_Normal01_obj_PolygonGroup00_TEXCOORD_05, sTexcoordBuffer1.size);

	return true;
}

static bool setRenderObject(void)
{
	// set up vertex attributes
	if(setVertexAttributes() != true) return false;
	
	// set up texture
	if( cellGtfReadFileHeader( GTF_FILE, &sGtfHeader ) < 0 ) {
		fprintf( stderr, "Error getting file header: %s.\n", GTF_FILE );
		return false;
	}

	// Get texture size 
	sNormalMapAddress = (void*)cellGcmUtilAllocateLocalMemory(sGtfHeader.size, 64);

	// load default texture
	if(cellGtfLoad(GTF_FILE, 0, &sNormalMap, CELL_GCM_LOCATION_LOCAL, sNormalMapAddress) < 0 ) {
		fprintf( stderr, "Error loading texture: %s.\n", GTF_FILE );
		return false;
	}

	return true;
}

static bool initStateBuffer(void)
{
	// allocate buffer on main memory
	sStateBuffer = memalign( 0x100000, STATE_BUFSIZE );
	CELL_GCMUTIL_ASSERTS(sStateBuffer != NULL,"memalign()");

	// map allocated buffer
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmMapMainMemory(sStateBuffer, STATE_BUFSIZE, &sStateOffset));
	
	return true;
}

//
// Render a human with vertex lighting
//
static void renderHumanVertexLighting(void)
{
	Vector3 translate = Vector3(-1.0f, -1.0f, -3.0f);
	Matrix4 model_matrix;
	Matrix4 inv_model_matrix;
	Vector4 eye_pos_local;
	Vector4 light_pos_local;

	// Matrix calculation
	{
		model_matrix = Matrix4::translation(translate) 
		             * sRotateMatrix;

		// inv(RT) = inv(T) * inv(R)
		inv_model_matrix = sInvRotateMatrix 
		                 * Matrix4::translation(-translate);

		// Local light position
		light_pos_local = inv_model_matrix * sLightPos;

		// Local eye position
		eye_pos_local = inv_model_matrix * sEyePos;
	}

	// set vertex pointer and draw
	cellGcmSetVertexDataArray(sVertexLightingContainer.PositionIndex, 0, sizeof(float)*3, 3,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  sVertexBuffer.offset);
	cellGcmSetVertexDataArray(sVertexLightingContainer.NormalIndex, 0, sizeof(float)*3, 3,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  sNormalBuffer.offset);

	// bind Cg programs
	// NOTE: vertex program constants are copied here
	cellGcmSetVertexProgram(sVertexLightingContainer.vertexProgram, 
	                        sVertexLightingContainer.vertexProgramUcode);
	cellGcmSetFragmentProgram(sVertexLightingContainer.fragmentProgram, 
	                          sVertexLightingContainer.fragmentProgramOffset);

	// Set matrices
	cellGcmSetVertexProgramParameter(sVertexLightingContainer.ModelMatrixParameter, (float*)&model_matrix);
	cellGcmSetVertexProgramParameter(sVertexLightingContainer.ViewProjMatrixParameter,  (float*)&sViewProjMatrix);

	// Set lighting parameters
	cellGcmSetVertexProgramParameter(sVertexLightingContainer.LightPosParameter,    (float*)&light_pos_local);
	cellGcmSetVertexProgramParameter(sVertexLightingContainer.LightColParameter,    (float*)&sLightCol);
	cellGcmSetVertexProgramParameter(sVertexLightingContainer.AmbientParameter,     (float*)&sAmbient);
	cellGcmSetVertexProgramParameter(sVertexLightingContainer.EyePosLocalParameter, (float*)&eye_pos_local);

	// Draw Human!
	cellGcmSetInvalidateVertexCache();
	cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_TRIANGLES, 0, sVertexBuffer.count);
}

// 
// Render a human with Fragment Lighting
//
static void renderHumanFragmentLighting(void)
{
	Vector3 translate = Vector3( 0.0f, -1.0f, -3.0f);
	Matrix4 model_matrix;
	Matrix4 inv_model_matrix;
	Vector4 eye_pos_local;
	Vector4 light_pos_local;

	// Matrix calculation
	{
		model_matrix = Matrix4::translation(translate) 
		             * sRotateMatrix;

		// inv(RT) = inv(T) * inv(R)
		inv_model_matrix = sInvRotateMatrix 
		                 * Matrix4::translation(-translate);

		// Local light position
		light_pos_local = inv_model_matrix * sLightPos;

		// Local eye position
		eye_pos_local = inv_model_matrix * sEyePos;
	}

	// set vertex pointer and draw
	cellGcmSetVertexDataArray(sFragmentLightingContainer.PositionIndex, 0, sizeof(float)*3, 3,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  sVertexBuffer.offset);
	cellGcmSetVertexDataArray(sFragmentLightingContainer.NormalIndex, 0, sizeof(float)*3, 3,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  sNormalBuffer.offset);

	// bind Cg programs
	// NOTE: vertex program constants are copied here
	cellGcmSetVertexProgram(sFragmentLightingContainer.vertexProgram, 
	                        sFragmentLightingContainer.vertexProgramUcode);
	cellGcmSetFragmentProgram(sFragmentLightingContainer.fragmentProgram, 
	                          sFragmentLightingContainer.fragmentProgramOffset);


	// Set matrices
	cellGcmSetVertexProgramParameter(sFragmentLightingContainer.ModelMatrixParameter, (float*)&model_matrix);
	cellGcmSetVertexProgramParameter(sFragmentLightingContainer.ViewProjMatrixParameter,  (float*)&sViewProjMatrix);

	// Set lighting parameters
	cellGcmSetFragmentProgramParameter(sFragmentLightingContainer.fragmentProgram, 
	                                   sFragmentLightingContainer.LightPosParameter, 
	                                   (float*)&light_pos_local, 
									   sFragmentLightingContainer.fragmentProgramOffset);
	cellGcmSetFragmentProgramParameter(sFragmentLightingContainer.fragmentProgram, 
	                                   sFragmentLightingContainer.LightColParameter,
	                                   (float*)&sLightCol, 
									   sFragmentLightingContainer.fragmentProgramOffset);
	cellGcmSetFragmentProgramParameter(sFragmentLightingContainer.fragmentProgram, 
	                                   sFragmentLightingContainer.AmbientParameter,
	                                   (float*)&sAmbient, 
									   sFragmentLightingContainer.fragmentProgramOffset);
	cellGcmSetFragmentProgramParameter(sFragmentLightingContainer.fragmentProgram, 
	                                   sFragmentLightingContainer.EyePosLocalParameter,
	                                   (float*)&eye_pos_local, 
									   sFragmentLightingContainer.fragmentProgramOffset);

	// update fragment program parameter
	cellGcmSetUpdateFragmentProgramParameter( sFragmentLightingContainer.fragmentProgramOffset );

	// Draw Human!
	cellGcmSetInvalidateVertexCache();
	cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_TRIANGLES, 0, sVertexBuffer.count);
}

//
// Render a human with normal map
//
static void renderHumanNormalMap(void)
{
	Vector3 translate = Vector3(+1.0f, -1.0f, -3.0f);
	Matrix4 model_matrix;
	Matrix4 inv_model_matrix;
	Vector4 eye_pos_local;
	Vector4 light_pos_local;

	// Matrix calculation
	{
		model_matrix = Matrix4::translation(translate) 
		             * sRotateMatrix;

		// inv(RT) = inv(T) * inv(R)
		inv_model_matrix = sInvRotateMatrix 
		                 * Matrix4::translation(-translate);

		// Local light position
		light_pos_local = inv_model_matrix * sLightPos;

		// Local eye position
		eye_pos_local = inv_model_matrix * sEyePos;
	}

	// set vertex pointer and draw
	cellGcmSetVertexDataArray(sNormalMapContainer.PositionIndex, 0, sizeof(float)*3, 3,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  sVertexBuffer.offset);
	cellGcmSetVertexDataArray(sNormalMapContainer.TexcoordIndex, 0, sizeof(float)*2, 2,
							  CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
							  sTexcoordBuffer1.offset);

	// bind Cg programs
	// NOTE: vertex program constants are copied here
	cellGcmSetVertexProgram(sNormalMapContainer.vertexProgram, 
	                        sNormalMapContainer.vertexProgramUcode);
	cellGcmSetFragmentProgram(sNormalMapContainer.fragmentProgram, 
	                          sNormalMapContainer.fragmentProgramOffset);

	// Set matrices
	cellGcmSetVertexProgramParameter(sNormalMapContainer.ModelMatrixParameter, (float*)&model_matrix);
	cellGcmSetVertexProgramParameter(sNormalMapContainer.ViewProjMatrixParameter,  (float*)&sViewProjMatrix);

	// Set parameters for fragment program
	cellGcmSetFragmentProgramParameter(sNormalMapContainer.fragmentProgram, 
	                                   sNormalMapContainer.LightPosParameter, 
	                                   (float*)&light_pos_local, 
									   sNormalMapContainer.fragmentProgramOffset);
	cellGcmSetFragmentProgramParameter(sNormalMapContainer.fragmentProgram, 
	                                   sNormalMapContainer.LightColParameter,
	                                   (float*)&sLightCol, 
									   sNormalMapContainer.fragmentProgramOffset);
	cellGcmSetFragmentProgramParameter(sNormalMapContainer.fragmentProgram, 
	                                   sNormalMapContainer.AmbientParameter,
	                                   (float*)&sAmbient, 
									   sNormalMapContainer.fragmentProgramOffset);
	cellGcmSetFragmentProgramParameter(sNormalMapContainer.fragmentProgram, 
	                                   sNormalMapContainer.EyePosLocalParameter,
	                                   (float*)&eye_pos_local, 
									   sNormalMapContainer.fragmentProgramOffset);

	// set texture
	cellGcmSetTexture(sTexUnit, &sNormalMap);

	// bind texture and set filter
	cellGcmSetTextureControl(sTexUnit, CELL_GCM_TRUE, 0<<8, 0<<8, CELL_GCM_TEXTURE_MAX_ANISO_1);
	cellGcmSetTextureAddress(sTexUnit,
							 CELL_GCM_TEXTURE_WRAP,
							 CELL_GCM_TEXTURE_WRAP,
							 CELL_GCM_TEXTURE_WRAP,
							 CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL,
							 CELL_GCM_TEXTURE_ZFUNC_LESS, 0);
	cellGcmSetTextureFilter(sTexUnit, 0,
							CELL_GCM_TEXTURE_NEAREST, 
							CELL_GCM_TEXTURE_NEAREST, 
							CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);

	// update fragment program parameter
	cellGcmSetUpdateFragmentProgramParameter( sNormalMapContainer.fragmentProgramOffset );

	// Draw Human!
	cellGcmSetInvalidateVertexCache();
	cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_TRIANGLES, 0, sVertexBuffer.count);
}


// This will create graphics command sequence that will be
// executed at every flip (light-weight switch).
//
static void setRenderState(void) 
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
	w = cellGcmUtilGetDisplayWidth();
	h = cellGcmUtilGetDisplayHeight();
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

	cellGcmSetBlendEnable(CELL_GCM_FALSE);
	cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
	cellGcmSetDepthFunc(CELL_GCM_LESS);
	cellGcmSetShadeMode(CELL_GCM_SMOOTH);
}

static bool getShaderParametersVertexLighting(void)
{
	// uniforma variables
	sVertexLightingContainer.ModelMatrixParameter
		= cellGcmCgGetNamedParameter(sVertexLightingContainer.vertexProgram, "ModelMatrix");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sVertexLightingContainer.ModelMatrixParameter);

	sVertexLightingContainer.ViewProjMatrixParameter
		= cellGcmCgGetNamedParameter(sVertexLightingContainer.vertexProgram, "ViewProjMatrix");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sVertexLightingContainer.ViewProjMatrixParameter);

	sVertexLightingContainer.LightPosParameter
		= cellGcmCgGetNamedParameter(sVertexLightingContainer.vertexProgram, "lightPos");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sVertexLightingContainer.LightPosParameter);

	sVertexLightingContainer.LightColParameter
		= cellGcmCgGetNamedParameter(sVertexLightingContainer.vertexProgram, "lightCol");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sVertexLightingContainer.LightColParameter);

	sVertexLightingContainer.AmbientParameter
		= cellGcmCgGetNamedParameter(sVertexLightingContainer.vertexProgram, "ambient");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sVertexLightingContainer.AmbientParameter);

	sVertexLightingContainer.EyePosLocalParameter
		= cellGcmCgGetNamedParameter(sVertexLightingContainer.vertexProgram, "eyePosLocal");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sVertexLightingContainer.EyePosLocalParameter);

	// vertex attributes
	CGparameter position
		= cellGcmCgGetNamedParameter(sVertexLightingContainer.vertexProgram, "position");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(position);

	CGparameter normal
		= cellGcmCgGetNamedParameter(sVertexLightingContainer.vertexProgram, "normal");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(normal);

	// get vertex attribute index
	sVertexLightingContainer.PositionIndex = 
	  (CGresource)(cellGcmCgGetParameterResource(sVertexLightingContainer.vertexProgram, position) - CG_ATTR0);
	sVertexLightingContainer.NormalIndex = 
	  (CGresource)(cellGcmCgGetParameterResource(sVertexLightingContainer.vertexProgram, normal) - CG_ATTR0);

	return true;
}

static bool getShaderParametersFragmentLighting(void)
{
	// uniforma variables
	sFragmentLightingContainer.ModelMatrixParameter
		= cellGcmCgGetNamedParameter(sFragmentLightingContainer.vertexProgram, "ModelMatrix");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sFragmentLightingContainer.ModelMatrixParameter);

	sFragmentLightingContainer.ViewProjMatrixParameter
		= cellGcmCgGetNamedParameter(sFragmentLightingContainer.vertexProgram, "ViewProjMatrix");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sFragmentLightingContainer.ViewProjMatrixParameter);

	sFragmentLightingContainer.LightPosParameter
		= cellGcmCgGetNamedParameter(sFragmentLightingContainer.fragmentProgram, "lightPos");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sFragmentLightingContainer.LightPosParameter);

	sFragmentLightingContainer.LightColParameter
		= cellGcmCgGetNamedParameter(sFragmentLightingContainer.fragmentProgram, "lightCol");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sFragmentLightingContainer.LightColParameter);

	sFragmentLightingContainer.AmbientParameter
		= cellGcmCgGetNamedParameter(sFragmentLightingContainer.fragmentProgram, "ambient");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sFragmentLightingContainer.AmbientParameter);

	sFragmentLightingContainer.EyePosLocalParameter
		= cellGcmCgGetNamedParameter(sFragmentLightingContainer.fragmentProgram, "eyePosLocal");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sFragmentLightingContainer.EyePosLocalParameter);

	// vertex attributes
	CGparameter position
		= cellGcmCgGetNamedParameter(sFragmentLightingContainer.vertexProgram, "position");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(position);

	CGparameter normal
		= cellGcmCgGetNamedParameter(sFragmentLightingContainer.vertexProgram, "normal");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(normal);

	// get vertex attribute index
	sFragmentLightingContainer.PositionIndex = 
	   (CGresource)(cellGcmCgGetParameterResource(sFragmentLightingContainer.vertexProgram, position) - CG_ATTR0);
	sFragmentLightingContainer.NormalIndex = 
	   (CGresource)(cellGcmCgGetParameterResource(sFragmentLightingContainer.vertexProgram, normal) - CG_ATTR0);

	return true;
}

static bool getShaderParametersNormalMap(void)
{
	// uniform variables
	sNormalMapContainer.ModelMatrixParameter
		= cellGcmCgGetNamedParameter(sNormalMapContainer.vertexProgram, "ModelMatrix");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sNormalMapContainer.ModelMatrixParameter);

	sNormalMapContainer.ViewProjMatrixParameter
		= cellGcmCgGetNamedParameter(sNormalMapContainer.vertexProgram, "ViewProjMatrix");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sNormalMapContainer.ViewProjMatrixParameter);

	sNormalMapContainer.LightPosParameter
		= cellGcmCgGetNamedParameter(sNormalMapContainer.fragmentProgram, "lightPos");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sNormalMapContainer.LightPosParameter);

	sNormalMapContainer.LightColParameter
		= cellGcmCgGetNamedParameter(sNormalMapContainer.fragmentProgram, "lightCol");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sNormalMapContainer.LightColParameter);

	sNormalMapContainer.AmbientParameter
		= cellGcmCgGetNamedParameter(sNormalMapContainer.fragmentProgram, "ambient");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sNormalMapContainer.AmbientParameter);

	sNormalMapContainer.EyePosLocalParameter
		= cellGcmCgGetNamedParameter(sNormalMapContainer.fragmentProgram, "eyePosLocal");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(sNormalMapContainer.EyePosLocalParameter);

	// vertex attributes
	CGparameter position
		= cellGcmCgGetNamedParameter(sNormalMapContainer.vertexProgram, "position");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(position);

	CGparameter texcoord
		= cellGcmCgGetNamedParameter(sNormalMapContainer.vertexProgram, "texcoord");
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT(texcoord);

	// get vertex attribute index
	sNormalMapContainer.PositionIndex = 
	  (CGresource)(cellGcmCgGetParameterResource(sNormalMapContainer.vertexProgram, position) - CG_ATTR0);
	sNormalMapContainer.TexcoordIndex = 
	  (CGresource)(cellGcmCgGetParameterResource(sNormalMapContainer.vertexProgram, texcoord) - CG_ATTR0);

	// get texture sampler
	CGparameter texture
		= cellGcmCgGetNamedParameter(sNormalMapContainer.fragmentProgram, "normalMap");
	sTexUnit = (CGresource)(cellGcmCgGetParameterResource(sNormalMapContainer.fragmentProgram, texture) - CG_TEXUNIT0);

	return true;
}

static bool setRenderEnv(void)
{
	//
	sEyePos = Vector4( Point3( 0.f, 0.f, 0.f));

	// view projection matrix
	float rad = 45.f * (PI/180.f);
	float aspect = cellGcmUtilGetDisplayAspectRatio();
	sViewProjMatrix = Matrix4::perspective(rad, aspect, 1.f, 10000.f) 
	                * Matrix4::identity();

	// assign lighting parameters
	sLightCol = Vector4( 0.6f, 0.7f, 0.7f, 1.f );
	sAmbient  = Vector4( 0.2f, 0.1f, 0.1f, 1.f );
	sLightPos = Vector4( 0.0f, 1.0f, 5.0f, 1.f );

	// get Shader's uniform parameters
	if(getShaderParametersVertexLighting() != true) return false;
	if(getShaderParametersFragmentLighting() != true) return false;
	if(getShaderParametersNormalMap() != true) return false;


	return true;
}

static void updateMatrix(void)
{
	// model rotate
	float AngleX = 0.0f; 
	float AngleY = 0.3f; 
	float AngleZ = 0.0f;
	AngleX += 0.000f;
	AngleY += 0.01f;
	AngleZ += 0.000f;
	if( AngleX > PI2 ) AngleX -= PI2;
	if( AngleY > PI2 ) AngleY -= PI2;
	if( AngleZ > PI2 ) AngleZ -= PI2;

	// Transform matrix
	//
	sRotateMatrix = Matrix4::rotationZYX(Vector3(AngleX, AngleY, AngleZ));
	sInvRotateMatrix = transpose(sRotateMatrix);
	
}

static bool loadPrx(void) 
{
	int ret;

	// Load Fs PRX
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
	if (ret < 0) {
		printf("cellSysmoduleLoadModule failed (0x%x)\n", ret);
		return false;
	}
	return true;
}

static void unloadPrx(void) 
{
	// Unload Fs PRX
	cellSysmoduleUnloadModule(CELL_SYSMODULE_FS);
}

#define HOST_SIZE 0x100000 // 1MB
#define CB_SIZE   0x10000  // 64KB

int32_t userMain(void)
{
	void *host_addr = memalign(0x100000, HOST_SIZE);
	CELL_GCMUTIL_ASSERTS(host_addr != NULL,"memalign()");
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmInit(CB_SIZE, HOST_SIZE, host_addr));

	if (loadPrx() != true)
		return -1;

	// set callback for exit routine
	if( cellGcmUtilInitCallback(clean_up) != CELL_OK ) return -1;
	
	// get configration
	CellGcmConfig config;
	cellGcmGetConfiguration(&config);

	cellGcmUtilInitializeLocalMemory((size_t)config.localAddress, (size_t)config.localSize);

	if (cellGcmUtilInitDisplay()!=true)	
		return -1;

	if (initShader() != true)
		return -1;

	if (setRenderObject() != true)
		return -1;
	
	if (setRenderEnv() != true)
		return -1;

	// initialize state buffer
	if (initStateBuffer() != true)
		return -1;

#ifdef CELL_GCM_DEBUG // {
	// need to disable debug finish bedcause we don't want to 
	// execute commands created here.
	//
	gCellGcmDebugCallback = NULL;
#endif // }

	// set up render state and state buffer.
	cellGcmSetCurrentBuffer((uint32_t*)sStateBuffer, STATE_BUFSIZE); 
	{
		// create graphic commands for render states
		setRenderState();

		// need to put 'return' command at end
		cellGcmSetReturnCommand();

	}
	// get back to default command buffer
	cellGcmSetDefaultCommandBuffer();

#ifdef CELL_GCM_DEBUG // {
		// restore debug finish callback.
		// cellGcmDebugFinish() is default debug finish function.
		// 
		gCellGcmDebugCallback = cellGcmDebugFinish;
#endif // }

	cellGcmUtilResetRenderTarget();

	// rendering loop
	while (cellGcmUtilCheckCallback()) {
		
		// reset render state by calling state buffer
		cellGcmSetCallCommand( sStateOffset );

		// clear frame buffer
		cellGcmSetClearSurface(CELL_GCM_CLEAR_Z|
				CELL_GCM_CLEAR_R|
				CELL_GCM_CLEAR_G|
				CELL_GCM_CLEAR_B|
				CELL_GCM_CLEAR_A);

		// update matrix
		updateMatrix();

		// render scene
		renderHumanVertexLighting();
		renderHumanFragmentLighting();
		renderHumanNormalMap();

		// start reading the command buffer
		cellGcmUtilFlipDisplay();
		
		// for sample navigator
		if(cellSnaviUtilIsExitRequested(NULL)){
			break;
		}

		return 0;
	}

	return 0;
}

void clean_up(void)
{
	// Unload Prx
	unloadPrx();
}


