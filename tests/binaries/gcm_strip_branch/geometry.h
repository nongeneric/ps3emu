/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2008 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

bool cubeInit(void);
void cubeEnd(void);
void cubeDraw(void);

bool floorInit(void);
void floorEnd(void);
void floorDraw(void);

bool multiCubeInit(void);
void multiCubeEnd(void);
void multiCubeDrawBegin(void);
void multiCubeDraw(uint32_t number);
void multiCubeDrawEnd(void);

#endif /* __GEOMETRY_H__ */
