/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#ifndef PPU_REMAP_H_
#define PPU_REMAP_H_

#define TEXTURE_REMAP(RemapOrder, OutB, OutG, OutR, OutA, InB, InG, InR, InA) \
	(((RemapOrder) << 16) | \
	((OutB) << 14) | ((OutG) << 12) |  ((OutR) << 10) |  ((OutA) << 8) | \
	((InB) << 6) | ((InG) << 4) |  ((InR) << 2) | (InA))

#define TEXTURE_REMAP_ARGB_ARGB \
	TEXTURE_REMAP(CELL_GCM_TEXTURE_REMAP_ORDER_XYXY,\
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_FROM_B, \
	CELL_GCM_TEXTURE_REMAP_FROM_G, \
	CELL_GCM_TEXTURE_REMAP_FROM_R, \
	CELL_GCM_TEXTURE_REMAP_FROM_A)

#define TEXTURE_REMAP_XRGB_XRGB(x) \
	TEXTURE_REMAP(CELL_GCM_TEXTURE_REMAP_ORDER_XYXY,\
	x, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_FROM_B, \
	CELL_GCM_TEXTURE_REMAP_FROM_G, \
	CELL_GCM_TEXTURE_REMAP_FROM_R, \
	CELL_GCM_TEXTURE_REMAP_FROM_A)

#define TEXTURE_REMAP_ABGR_ARGB \
	TEXTURE_REMAP(CELL_GCM_TEXTURE_REMAP_ORDER_XYXY,\
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_FROM_R, \
	CELL_GCM_TEXTURE_REMAP_FROM_G, \
	CELL_GCM_TEXTURE_REMAP_FROM_B, \
	CELL_GCM_TEXTURE_REMAP_FROM_A)

#define TEXTURE_REMAP_XBGR_XRGB(x) \
	TEXTURE_REMAP(CELL_GCM_TEXTURE_REMAP_ORDER_XYXY,\
	x, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_FROM_R, \
	CELL_GCM_TEXTURE_REMAP_FROM_G, \
	CELL_GCM_TEXTURE_REMAP_FROM_B, \
	CELL_GCM_TEXTURE_REMAP_FROM_A)


#define TEXTURE_REMAP_X32_XXXX \
	TEXTURE_REMAP(CELL_GCM_TEXTURE_REMAP_ORDER_XYXY,\
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_FROM_R, \
	CELL_GCM_TEXTURE_REMAP_FROM_R, \
	CELL_GCM_TEXTURE_REMAP_FROM_R, \
	CELL_GCM_TEXTURE_REMAP_FROM_R)

#define TEXTURE_REMAP_DEPTH_READ \
	TEXTURE_REMAP(CELL_GCM_TEXTURE_REMAP_ORDER_XYXY,\
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_REMAP, \
	CELL_GCM_TEXTURE_REMAP_FROM_G, \
	CELL_GCM_TEXTURE_REMAP_FROM_R, \
	CELL_GCM_TEXTURE_REMAP_FROM_A, \
	CELL_GCM_TEXTURE_REMAP_FROM_B)

#endif // PPU_REMAP_H_