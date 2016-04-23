/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#include "disp.h"
#include "memory.h"
#include <cell/gcm.h>
#include <sysutil/sysutil_sysparam.h>
#include <string.h>

#include "gcmutil_error.h"

using namespace cell::Gcm;

static uint32_t color_pitch;
static uint32_t depth_pitch;
static void     *color_addr[4];
static uint32_t color_offset[4];
static uint32_t depth_offset;

/* double buffering */
#define COLOR_BUFFER_NUM 2

static uint32_t display_width;
static uint32_t display_height; 
static uint32_t frame_index = 0;
#if 1
void setRenderTarget(const uint32_t Index)
{
	CellGcmSurface sf;
	sf.colorFormat 	= CELL_GCM_SURFACE_A8R8G8B8;
	sf.colorTarget	= CELL_GCM_SURFACE_TARGET_0;
	sf.colorLocation[0]	= CELL_GCM_LOCATION_LOCAL;
	sf.colorOffset[0] 	= color_offset[Index];
	sf.colorPitch[0] 	= color_pitch;

	sf.colorLocation[1]	= CELL_GCM_LOCATION_LOCAL;
	sf.colorLocation[2]	= CELL_GCM_LOCATION_LOCAL;
	sf.colorLocation[3]	= CELL_GCM_LOCATION_LOCAL;
	sf.colorOffset[1] 	= 0;
	sf.colorOffset[2] 	= 0;
	sf.colorOffset[3] 	= 0;
	sf.colorPitch[1]	= 64;
	sf.colorPitch[2]	= 64;
	sf.colorPitch[3]	= 64;

	sf.depthFormat 	= CELL_GCM_SURFACE_Z24S8;
	sf.depthLocation	= CELL_GCM_LOCATION_LOCAL;
	sf.depthOffset	= depth_offset;
	sf.depthPitch 	= depth_pitch;

	sf.type		= CELL_GCM_SURFACE_PITCH;
	sf.antialias	= CELL_GCM_SURFACE_CENTER_1;

	sf.width 		= display_width;
	sf.height 		= display_height;
	sf.x 		= 0;
	sf.y 		= 0;
	cellGcmSetSurface(&sf);
}
#endif
/* wait until flip */
static void waitFlip(void)
{
	while (cellGcmGetFlipStatus()!=0){
		sys_timer_usleep(300);
	}
	cellGcmResetFlipStatus();
}

void flip(void)
{
	static int first=1;

	// wait until the previous flip executed
	if (first!=1) waitFlip();
	else cellGcmResetFlipStatus();

	if(cellGcmSetFlip(frame_index) != CELL_OK) return;
	cellGcmFlush();

	cellGcmSetWaitFlip();
	cellGcmFlush();

	// New render target
	frame_index = (frame_index+1)%COLOR_BUFFER_NUM;
	setRenderTarget(frame_index);

	first=0;
}

int initDisplay(void)
{
	uint32_t color_depth=4; // ARGB8
	uint32_t z_depth=4;     // COMPONENT24
	void *depth_addr;
	CellVideoOutResolution resolution;

	// display initialize

	// read the current video status
	// INITIAL DISPLAY MODE HAS TO BE SET BY RUNNING SETMONITOR.SELF
	CellVideoOutState videoState;
	CELL_GCMUTIL_CHECK_ASSERT(cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState));
	CELL_GCMUTIL_CHECK_ASSERT(cellVideoOutGetResolution(videoState.displayMode.resolutionId, &resolution));

	display_width = resolution.width;
	display_height = resolution.height;
	color_pitch = cellGcmGetTiledPitchSize((((display_width+(64-1))/64)*64)*color_depth);
	depth_pitch = cellGcmGetTiledPitchSize((((display_width+(64-1))/64)*64)*z_depth);

	uint32_t color_size = (((color_pitch*(((display_height+(32-1))/32)*32))+(64*1024-1))/(64*1024)) * (64*1024);
	uint32_t depth_size = (((depth_pitch*(((display_height+(32-1))/32)*32))+(64*1024-1))/(64*1024)) * (64*1024);

	CellVideoOutConfiguration videocfg;
	memset(&videocfg, 0, sizeof(CellVideoOutConfiguration));
	videocfg.resolutionId = videoState.displayMode.resolutionId;
	videocfg.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
	videocfg.pitch = color_pitch;

	CELL_GCMUTIL_CHECK_ASSERT(cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, &videocfg, NULL, 0));

	cellGcmSetFlipMode(CELL_GCM_DISPLAY_VSYNC);

	uint32_t comp_tag = 0;
	for (int i = 0; i < COLOR_BUFFER_NUM; i++) {
		color_addr[i] = localMemoryAlign(64*1024, color_size);
		CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(color_addr[i], &color_offset[i]));
		CELL_GCMUTIL_CHECK(cellGcmSetTileInfo(i, CELL_GCM_LOCATION_LOCAL, color_offset[i], color_size, color_pitch, CELL_GCM_COMPMODE_C32_2X1, comp_tag, i));
		CELL_GCMUTIL_CHECK(cellGcmBindTile(i));
		comp_tag += color_size/0x10000;
	}

	// regist surface
	for (int i = 0; i < COLOR_BUFFER_NUM; i++) {
		CELL_GCMUTIL_CHECK_ASSERT(cellGcmSetDisplayBuffer(i, color_offset[i], color_pitch, display_width, display_height));
	}

	depth_addr = localMemoryAlign(64*1024, depth_size);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(depth_addr, &depth_offset));
	CELL_GCMUTIL_CHECK(cellGcmSetTileInfo(COLOR_BUFFER_NUM, CELL_GCM_LOCATION_LOCAL, depth_offset, depth_size, depth_pitch, CELL_GCM_COMPMODE_Z32_SEPSTENCIL, comp_tag, COLOR_BUFFER_NUM));
	CELL_GCMUTIL_CHECK(cellGcmBindTile(COLOR_BUFFER_NUM));
	CELL_GCMUTIL_CHECK(cellGcmBindZcull(0, depth_offset, (((display_width+(64-1))/64)*64), (((display_height+(64-1))/64)*64), 0, CELL_GCM_ZCULL_Z24S8, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_LONES, CELL_GCM_SCULL_SFUNC_LESS, 0x80, 0xff));

	return CELL_OK;
}

uint32_t getFrameIndex(void)
{
	return frame_index;
}

void setViewport(void)
{
	uint16_t x,y,w,h;
	float min, max;
	float scale[4],offset[4];

	x = 0;
	y = 0;
	w = display_width;
	h = display_height;
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

}
