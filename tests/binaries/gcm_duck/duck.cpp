/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timer.h>
#include <sys/paths.h>
#include <sys/return_code.h>
#include <cell/gcm.h>
#include <cell/sysmodule.h>
#include <stddef.h>
#include <math.h>
#include <sysutil/sysutil_sysparam.h>
#include "gtf.h"
#include "spu.h"
#include "disp.h"
#include "fs.h"
#include "geometry.h"
#include "shader.h"
#include "texture.h"
#include "memory.h"

#include "vpshader_params.h"
#include "fpshader_params.h"

#include "padutil.h"
#include "snaviutil.h"

// For exit routine
static void sysutil_exit_callback(uint64_t status, uint64_t param, void* userdata);
static bool sKeepRunning = true;

using namespace cell::Gcm;

/* prototypes */
extern "C" int32_t userMain(void);

static void setDrawEnv(void)
{
	cellGcmSetColorMask(CELL_GCM_COLOR_MASK_B|
			CELL_GCM_COLOR_MASK_G|
			CELL_GCM_COLOR_MASK_R|
			CELL_GCM_COLOR_MASK_A);

	cellGcmSetColorMaskMrt(0);
	setViewport();
	cellGcmSetClearColor((64<<0)|(64<<8)|(64<<16)|(64<<24));

	if (isLineMode()) {
		cellGcmSetFrontPolygonMode(CELL_GCM_POLYGON_MODE_LINE);
		cellGcmSetLineSmoothEnable(CELL_GCM_TRUE);
	}
	else {
		cellGcmSetFrontPolygonMode(CELL_GCM_POLYGON_MODE_FILL);
	}
	cellGcmSetBlendEnable(CELL_GCM_TRUE);
	cellGcmSetBlendEquation(CELL_GCM_FUNC_ADD, CELL_GCM_FUNC_ADD);
	cellGcmSetBlendFunc(CELL_GCM_SRC_ALPHA, CELL_GCM_ONE_MINUS_SRC_ALPHA, CELL_GCM_ZERO, CELL_GCM_ZERO);
	cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
	cellGcmSetDepthFunc(CELL_GCM_LESS);
	cellGcmSetShadeMode(CELL_GCM_SMOOTH);
	cellGcmSetCullFace(CELL_GCM_BACK);
	cellGcmSetCullFaceEnable(CELL_GCM_TRUE);
}


static int setRenderObject(void)
{
	// transform
	float M[16];
	float MVP[16];
	static float xpos = 3.5f;

	// wait green light from spu
	waitSignalFromSpu();
	setShaderProgram();
	// setup default value
	for (uint32_t i = 0; i < sizeof(vpshader_params)/sizeof(CellGcm_vpshader_params_Table); i++) {
		if (vpshader_params[i].dvindex != -1) {
			cellGcmSetVertexProgramConstants(
					vpshader_params[i].resindex,
					4,
					vpshader_params_default_value[vpshader_params[i].dvindex].defaultvalue);
		}
	}
	setGcmTexture(fpshader_params[CELL_GCM_fpshader_params_texture].res-CG_TEXUNIT0);
	cellGcmSetVertexDataArray(vpshader_params[CELL_GCM_vpshader_params_position].res-CG_ATTR0,
			0, 
			sizeof(Vertex), 
			3, 
			CELL_GCM_VERTEX_F, 
			CELL_GCM_LOCATION_MAIN, 
			getVertexOffset());

	cellGcmSetVertexDataArray(vpshader_params[CELL_GCM_vpshader_params_normal].res-CG_ATTR0,
			0, 
			sizeof(Vertex), 
			3, 
			CELL_GCM_VERTEX_F, 
			CELL_GCM_LOCATION_MAIN, 
			getNormalOffset());

	cellGcmSetVertexDataArray(vpshader_params[CELL_GCM_vpshader_params_texcoord].res-CG_ATTR0,
			0, 
			sizeof(Vertex), 
			2, 
			CELL_GCM_VERTEX_F, 
			CELL_GCM_LOCATION_MAIN, 
			getTexcoordOffset());

	int obj_num = getObjNum();
	uint32_t index_offset = getIndexOffset();
	uint32_t num_index = getIndexNumber();
	for (int i=0; i < obj_num; i++) {
		getMatrix(i, xpos, MVP, M);
		cellGcmSetVertexProgramConstants(vpshader_params[CELL_GCM_vpshader_params_modelViewProj_0].resindex, 16, MVP);
		cellGcmSetVertexProgramConstants(vpshader_params[CELL_GCM_vpshader_params_modelView_0].resindex, 16, M);
		cellGcmSetDrawIndexArray(CELL_GCM_PRIMITIVE_TRIANGLES, num_index, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, CELL_GCM_LOCATION_MAIN, index_offset);

	}
	xpos += 1.0f/60.0f;

	// clear it for the next sync
	clearSignalFromSpu();
	signalSpuFromRsx();

	return CELL_OK;
}

int initialize(void)
{
	if (initSpu() != CELL_OK) return -1;
	if (initFs() != CELL_OK) return -1;
	if (initMemory() != CELL_OK) return -1;
	if (initDisplay()!= CELL_OK) return -1;
	if (initShader() != CELL_OK) return -1;

	if(!cellPadUtilPadInit()) return -1;

	return CELL_OK;
}

int userMain()
{
	int32_t ret;
	if (initialize() != CELL_OK) return -1;

	setupTex();
	setupVertexData();
	// 1st time
	setRenderTarget(getFrameIndex());
	cellGcmFinish(0);

	uint32_t const_addr =
		getFpUcode() + fpshader_params_const_offset[fpshader_params[CELL_GCM_fpshader_params_light].ecindex].offset;
	setupSpuSharedBuffer(
			const_addr,
			(uint32_t)mainMemoryAlign(128, 128),
			(uint32_t)cellGcmGetLabelAddress(64)
			);

	// signal to spu
	signalSpuFromPpu();

	signalSpuFromRsx();

	// Exit routines
	{
		// register sysutil exit callback
		ret = cellSysutilRegisterCallback(0, sysutil_exit_callback, NULL);
		if( ret != CELL_OK ) {
			printf( "Registering sysutil callback failed...: error=0x%x\n", ret );
			return -1;
		}
	}

	// rendering loop
	while (sKeepRunning) {
		// check system event
		ret = cellSysutilCheckCallback();
		if( ret != CELL_OK ) {
			printf( "cellSysutilCheckCallback() failed...: error=0x%x\n", ret );
		}

		if(cellPadUtilUpdate()){
			if(cellPadUtilButtonPressedOnce(0, CELL_UTIL_BUTTON_TRIANGLE)){
				switchPolyMode();
			}
			if(cellPadUtilButtonPressed(0, CELL_UTIL_BUTTON_R1)){
				increaseObj();
			}
			if(cellPadUtilButtonPressed(0, CELL_UTIL_BUTTON_R2)){
				decreaseObj();
			}
		}

		setDrawEnv();
		// clear frame buffer
		cellGcmSetClearSurface(CELL_GCM_CLEAR_Z|
				CELL_GCM_CLEAR_R|
				CELL_GCM_CLEAR_G|
				CELL_GCM_CLEAR_B|
				CELL_GCM_CLEAR_A);

		setRenderObject();

		// start reading the command buffer
		flip();
		
		// for sample navigator
		if(cellSnaviUtilIsExitRequested(cellPadUtilGetPadData(0))){
			break;
		}

		break;
	}

	// Let RSX wait for final flip
	cellGcmSetWaitFlip();

	// Let PPU wait for all commands done (include waitFlip)
	cellGcmFinish(1);
	
	
	cellPadUtilPadEnd();

	return 0;
}

void sysutil_exit_callback(uint64_t status, uint64_t param, void* userdata)
{
	(void) param;
	(void) userdata;

	switch(status) {
	case CELL_SYSUTIL_REQUEST_EXITGAME:
		sKeepRunning = false;
		break;
	case CELL_SYSUTIL_DRAWING_BEGIN:
	case CELL_SYSUTIL_DRAWING_END:
	case CELL_SYSUTIL_SYSTEM_MENU_OPEN:
	case CELL_SYSUTIL_SYSTEM_MENU_CLOSE:
	case CELL_SYSUTIL_BGMPLAYBACK_PLAY:
	case CELL_SYSUTIL_BGMPLAYBACK_STOP:
	default:
		break;
	}
}
