/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#ifndef __GEOMETRY_H_
#define __GEOMETRY_H_
typedef struct {
	float vx, vy, vz;
	float nx, ny, nz;
	float u, v;
} Vertex;

void increaseObj(void);
void decreaseObj(void);
void switchPolyMode(void);
bool isLineMode(void);
int setupVertexData();
void getMatrix(int i, float xpos, float* MVP, float* M);
uint32_t getVertexOffset();
uint32_t getNormalOffset();
uint32_t getTexcoordOffset();
int getObjNum();
uint32_t getIndexOffset();
uint32_t getIndexNumber();


#endif //__GEOMETRY_H_
