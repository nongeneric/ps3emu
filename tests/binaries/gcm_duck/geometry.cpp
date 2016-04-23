/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cell/gcm.h>
#include <sys/paths.h>
#include "geometry.h"
#include "fs.h"
#include "memory.h"

#include "gcmutil_error.h"

static int obj_num=1;
static int num_vert, num_norm, num_uv, num_tri;
static const char* sFileName2 = GCM_SAMPLE_DATA_PATH "/duck/duck.smesh";

static uint32_t geom_addr;

static float U[16];
static float P[16];

static uint32_t vertex_offset[3];
static uint32_t index_offset;
static uint32_t num_index;
static Vertex* attr;

typedef struct {
	float x, y, z;
	float angle;
} ObjInfo;
static ObjInfo obj_info[2048];

static uint32_t line_mode_flg=0;
void increaseObj(void)
{
	if (obj_num < 2048) obj_num += 1;
	printf("obj %d\n", obj_num);
	obj_info[obj_num-1].x = ((float)rand()/(float)RAND_MAX)*2.0f - 1.0f;
	obj_info[obj_num-1].y = ((float)rand()/(float)RAND_MAX)*2.0f - 1.0f;
	obj_info[obj_num-1].z = -2.0f - ((float)(rand()%10));
	obj_info[obj_num-1].angle = (float)(rand()%60);
}

void decreaseObj(void)
{
	if(obj_num > 0) obj_num -= 1;
}

void switchPolyMode(void)
{
	line_mode_flg = (~line_mode_flg)&0x01;
}

bool isLineMode(void)
{
	return line_mode_flg;
}
static void buildProjection(float *M, 
		const float top, 
		const float bottom, 
		const float left, 
		const float right, 
		const float near, 
		const float far)
{
	memset(M, 0, 16*sizeof(float)); 

	M[0*4+0] = (2.0f*near) / (right - left);
	M[1*4+1] = (2.0f*near) / (bottom - top);

	float A = (right + left) / (right - left);
	float B = (top + bottom) / (top - bottom);
	float C = -(far + near) / (far - near);
	float D = -(2.0f*far*near) / (far - near);

	M[0*4 + 2] = A;
	M[1*4 + 2] = B;
	M[2*4 + 2] = C;
	M[3*4 + 2] = -1.0f; 
	M[2*4 + 3] = D;
}

static void matrixMul(float *Dest, float *A, float *B)
{
	for (int i=0; i < 4; i++) {
		for (int j=0; j < 4; j++) {
			Dest[i*4+j] 
				= A[i*4+0]*B[0*4+j] 
				+ A[i*4+1]*B[1*4+j] 
				+ A[i*4+2]*B[2*4+j] 
				+ A[i*4+3]*B[3*4+j];
		}
	}
}

void matrixTranslate(float *M, 
		const float x, 
		const float y, 
		const float z)
{
	memset(M, 0, sizeof(float)*16);
	M[0*4+3] = x;
	M[1*4+3] = y;
	M[2*4+3] = z;

	M[0*4+0] = 1.0f;
	M[1*4+1] = 1.0f;
	M[2*4+2] = 1.0f;
	M[3*4+3] = 1.0f;
}

void unitMatrix(float *M, float mul)
{
	M[0*4+0] = 1.0f * mul;
	M[0*4+1] = 0.0f;
	M[0*4+2] = 0.0f;
	M[0*4+3] = 0.0f;

	M[1*4+0] = 0.0f;
	M[1*4+1] = 1.0f * mul;
	M[1*4+2] = 0.0f;
	M[1*4+3] = 0.0f;

	M[2*4+0] = 0.0f;
	M[2*4+1] = 0.0f;
	M[2*4+2] = 1.0f * mul;
	M[2*4+3] = 0.0f;

	M[3*4+0] = 0.0f;
	M[3*4+1] = 0.0f;
	M[3*4+2] = 0.0f;
	M[3*4+3] = 1.0f;
}
static int searchStr(char* ptr)
{
	int i = 0;
	while (1) {
		if (ptr[i] == ':' || ptr[i] == '\n' || ptr[i] == ' ' || ptr[i] == '\r') {
			ptr[i] = '\0';
			i++;
			if (ptr[i] == '\n')
				i++;
			break;
		}
		i++;
	}
	return i;
}

static int getInt(char* ptr, char* str, int* num)
{
	int total = 0;
	int cnt;
	char* end;
	while (1) {
		cnt = searchStr(ptr);
		total += cnt;
		if (!strcmp(str, ptr)) {
			ptr += cnt;
			cnt = searchStr(ptr);
			total += cnt;
			if (str != "Name")
				*num = strtol(ptr, &end, 10);
			break;
		}
		ptr += cnt;
	}
	return total;
}

typedef struct {
	float x, y, z;
} Float3;

static int getFloat3(char* ptr, char* str, Float3* num)
{
	int total = 0;
	int cnt;
	char* end;
	while (1) {
		cnt = searchStr(ptr);
		total += cnt;
		if (!strcmp(str, ptr)) {
			ptr += cnt;
			cnt = searchStr(ptr);
			total += cnt;
			num->x = strtof(ptr, &end);
			ptr += cnt;
			cnt = searchStr(ptr);
			total += cnt;
			num->y = strtof(ptr, &end);
			ptr += cnt;
			cnt = searchStr(ptr);
			total += cnt;
			num->z = strtof(ptr, &end);
			break;
		}
		ptr += cnt;
	}
	return total;
}

static int getIndex(char* ptr, uint16_t* tri)
{
	char* end;
	int cnt = searchStr(ptr);
	*tri = (uint16_t)strtol(ptr, &end, 10);
	return cnt;
}

typedef struct {
	uint16_t v0, v1, v2;
	uint16_t n0, n1, n2;
	uint16_t uv0, uv1, uv2;
} Triangle;

uint32_t getTriangle(char* ptr, Triangle* tri)
{
	int offset = 0;
	int tmp=0;
	int num = num_tri;
	while (1) {
		tmp += searchStr(ptr+offset);
		if (!strcmp((char*)"triangles", ptr+offset)) {
			offset += tmp;
			while (num--) {
				offset += getIndex(ptr+offset, &tri->v0);
				offset += getIndex(ptr+offset, &tri->v1);
				offset += getIndex(ptr+offset, &tri->v2);
				offset += getIndex(ptr+offset, &tri->n0);
				offset += getIndex(ptr+offset, &tri->n1);
				offset += getIndex(ptr+offset, &tri->n2);
				offset += getIndex(ptr+offset, &tri->uv0);
				offset += getIndex(ptr+offset, &tri->uv1);
				offset += getIndex(ptr+offset, &tri->uv2);
				offset += 3;
				tri++;
			}
			break;
		}
		offset += tmp;
	}
	return offset;
}
static uint32_t getSmeshConfig(uint32_t addr)
{
	char* ptr = (char*)addr;
	ptr += getInt(ptr, (char*)"Name", NULL);
	ptr += getInt(ptr, (char*)"NumVertices", &num_vert);
	ptr += getInt(ptr, (char*)"NumNormals", &num_norm);
	ptr += getInt(ptr, (char*)"NumUVs", &num_uv);
	ptr += getInt(ptr, (char*)"NumTriangles", &num_tri);
	return ((uint32_t)ptr - (uint32_t)addr);
}

static uint32_t setVertex2(uint32_t addr, char* str, Float3* dest, int num)
{
	char* ptr = (char*)addr;
	Float3* vert = dest;
	for (int i = 0; i < num; i++) {
		ptr += getFloat3(ptr, str, vert);
		vert++;
	}
	return ((uint32_t)ptr - (uint32_t)addr);
}

static uint32_t setTriangle(uint32_t addr, Triangle* tri)
{
	char* ptr = (char*)addr;
	return getTriangle(ptr, tri);
}

static void setIndex(uint16_t* index)
{
	for (int i = 0; i < num_tri*3; i++) {
		index[i] = i;
	}
}



int setupVertexData()
{
	if (loadFile(&geom_addr, (char*)sFileName2, CELL_GCM_LOCATION_MAIN) != CELL_OK)
		return -1;
	uint32_t offset = getSmeshConfig(geom_addr);
	printf("vert:%d, norm:%d, uv:%d, tri:%d\n", num_vert, num_norm, num_uv, num_tri);

	Float3* vert = (Float3*)malloc(num_vert*sizeof(Float3));
	Float3* norm = (Float3*)malloc(num_norm*sizeof(Float3));
	Float3* uv = (Float3*)malloc(num_uv*sizeof(Float3));
	Triangle* tri = (Triangle*)malloc(num_tri*sizeof(Triangle));
	attr = (Vertex*)mainMemoryAlign(128*1024, sizeof(Vertex)*num_tri*3);
	uint16_t* index = (uint16_t*)mainMemoryAlign(128*1024, sizeof(uint16_t)*num_tri*3);
	num_index = num_tri*3;
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(index, &index_offset));

	offset = setVertex2(geom_addr+offset, (char*)"v", vert, num_vert);
	offset += setVertex2(geom_addr+offset, (char*)"n", norm, num_norm);
	offset += setVertex2(geom_addr+offset, (char*)"uv", uv, num_uv);
	offset += setTriangle(geom_addr+offset, tri);
	setIndex(index);

	{
		uint32_t num = num_tri;
		Vertex* pattr = attr;
		Triangle* ptri = tri;
		while (num--) {
			pattr->vx = vert[ptri->v0].x;
			pattr->vy = vert[ptri->v0].y;
			pattr->vz = vert[ptri->v0].z;
			pattr->nx = norm[ptri->n0].x;
			pattr->ny = norm[ptri->n0].y;
			pattr->nz = norm[ptri->n0].z;
			pattr->u  = uv[ptri->uv0].x;
			pattr->v  = uv[ptri->uv0].y;
			pattr++;
			pattr->vx = vert[ptri->v1].x;
			pattr->vy = vert[ptri->v1].y;
			pattr->vz = vert[ptri->v1].z;
			pattr->nx = norm[ptri->n1].x;
			pattr->ny = norm[ptri->n1].y;
			pattr->nz = norm[ptri->n1].z;
			pattr->u  = uv[ptri->uv1].x;
			pattr->v  = uv[ptri->uv1].y;
			pattr++;
			pattr->vx = vert[ptri->v2].x;
			pattr->vy = vert[ptri->v2].y;
			pattr->vz = vert[ptri->v2].z;
			pattr->nx = norm[ptri->n2].x;
			pattr->ny = norm[ptri->n2].y;
			pattr->nz = norm[ptri->n2].z;
			pattr->u  = uv[ptri->uv2].x;
			pattr->v  = uv[ptri->uv2].y;
			pattr++;
			ptri++;
		}
	}

	free(tri);
	free(uv);
	free(norm);
	free(vert);
	obj_info[obj_num-1].x = ((float)rand()/(float)RAND_MAX)*2.0f - 1.0f;
	obj_info[obj_num-1].y = ((float)rand()/(float)RAND_MAX)*2.0f - 1.0f;
	obj_info[obj_num-1].z = -2.0f - ((float)(rand()%10));
	obj_info[obj_num-1].angle = (float)(rand()%60);

	// projection 
	buildProjection(P, -0.3f, 0.3f, -0.3f, 0.3f, 1.0, 10000.0f); 
	unitMatrix(U, 3.0f);
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(&attr->vx, &vertex_offset[0]));
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(&attr->nx, &vertex_offset[1]));
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(&attr->u, &vertex_offset[2]));

	return CELL_OK;
}

void getMatrix(int i, float xpos, float* MVP, float* M)
{
	float R[16];
	float V[16];
	float VP[16];
	// 16:9 scale
	matrixTranslate(V, obj_info[i].x + 0.05*sinf(xpos), obj_info[i].y + 0.05*sinf(1.3f*xpos), obj_info[i].z);
	V[0*4 + 0] = 9.0f / 16.0f; 
	V[1*4 + 1] = 1.0f; 

	// model view 
	matrixMul(VP, P, V);

	unitMatrix(R, 1.0f);
	R[0*4 + 0] = cosf(obj_info[i].angle + xpos);
	R[0*4 + 2] = sinf(obj_info[i].angle + xpos);
	R[2*4 + 0] = -sinf(obj_info[i].angle + xpos);
	R[2*4 + 2] = cosf(obj_info[i].angle + xpos);
	matrixMul(M, R, U);
	matrixMul(MVP, VP, M);
}

uint32_t getVertexOffset()
{
	return (uint32_t)vertex_offset[0];
}

uint32_t getNormalOffset()
{
	return (uint32_t)vertex_offset[1];
}

uint32_t getTexcoordOffset()
{
	return (uint32_t)vertex_offset[2];
}

int getObjNum()
{
	return obj_num;
}

uint32_t getIndexOffset()
{
	return index_offset;
}

uint32_t getIndexNumber()
{
	return num_index;
}

