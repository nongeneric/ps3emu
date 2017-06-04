/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <sys/paths.h>
#include "debug.h"
#include "file.h"
#include "memory.h"
#include "mesh_manager.h"

namespace Render{

namespace Smesh{

enum{
	INDEX_ALIGN = 16,
	VERTEX_ALIGN = 16,
};

struct Float2{
	float x,y;
};

struct Float3{
	float x,y,z;
};

struct Vertex{
	float vx,vy,vz,vw;
	float nx,ny,nz,nw;
	float u,v,r,q;
};

typedef struct {
	uint16_t v0, v1, v2;
	uint16_t n0, n1, n2;
	uint16_t uv0, uv1, uv2;
} Triangle;

static int num_vert, num_norm, num_uv, num_tri;

int searchStr(char* ptr)
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


int getInt(char* ptr, char* str, int* num)
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

int getFloat3(char* ptr, char* str, Float3* num)
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

uint32_t getSmeshConfig(char* addr)
{
	char* ptr = addr;
	ptr += getInt(ptr, (char*)"Name", NULL);
	ptr += getInt(ptr, (char*)"NumVertices", &num_vert);
	ptr += getInt(ptr, (char*)"NumNormals", &num_norm);
	ptr += getInt(ptr, (char*)"NumUVs", &num_uv);
	ptr += getInt(ptr, (char*)"NumTriangles", &num_tri);
	return (ptr - addr);
}

int getIndex(char* ptr, uint16_t* tri)
{
	char* end;
	int cnt = searchStr(ptr);
	*tri = (uint16_t)strtol(ptr, &end, 10);
	return cnt;
}

uint32_t setVertex(char* addr, char* str, Float3* dest, int num)
{
	char* ptr = addr;
	Float3* vert = dest;
	for (int i = 0; i < num; i++) {
		ptr += getFloat3(ptr, str, vert);
		vert++;
	}
	return (uint32_t)(ptr - addr);
}


void setIndex(uint16_t* index)
{
	for (int i = 0; i < num_tri*3; i++) {
		index[i] = i;
	}
}


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

uint32_t setTriangle(char* addr, Triangle* tri)
{
	char* ptr = addr;
	return getTriangle(ptr, tri);
}

int load(const char* filename,int32_t location, Mesh& mesh)
{
	Sys::Memory::HeapBase& tmpHeap = Sys::Memory::getTemporaryHeap();
	void* buffer = Sys::File::loadFile(filename, tmpHeap);
	char* pt = reinterpret_cast<char*>(buffer);

	uint32_t offset = getSmeshConfig(pt);
	MY_DPRINTF("vert:%d, norm:%d, uv:%d, tri:%d\n", num_vert, num_norm, num_uv, num_tri);

	Sys::Memory::VramHeap& vramHeap = (location == CELL_GCM_LOCATION_LOCAL)
		? Sys::Memory::getLocalMemoryHeap()
		: Sys::Memory::getMappedMainMemoryHeap();

	Vertex* attr = vramHeap.calloc<Vertex>(num_tri*3, VERTEX_ALIGN);
	uint16_t* index = vramHeap.calloc<uint16_t>(num_tri*3, INDEX_ALIGN);
	int num_index = num_tri*3;

	Float3* vert = tmpHeap.calloc<Float3>(num_vert);
	offset = setVertex(pt+offset, (char*)"v", vert, num_vert);
	Float3* norm = tmpHeap.calloc<Float3>(num_norm);
	offset += setVertex(pt+offset, (char*)"n", norm, num_norm);
	Float3* uv = tmpHeap.calloc<Float3>(num_uv);
	offset += setVertex(pt+offset, (char*)"uv", uv, num_uv);
	Triangle* tri = tmpHeap.calloc<Triangle>(num_tri);
	offset += setTriangle(pt+offset, tri);
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

	mesh.position.location	= location;
	mesh.position.offset	= vramHeap.AddressToOffset(&attr->vx);
	mesh.position.type		= CELL_GCM_VERTEX_F;
	mesh.position.stride	= sizeof(Vertex);
	mesh.position.size		= 3;

	mesh.normal.location	= location;
	mesh.normal.offset		= vramHeap.AddressToOffset(&attr->nx);
	mesh.normal.type		= CELL_GCM_VERTEX_F;
	mesh.normal.stride		= sizeof(Vertex);
	mesh.normal.size		= 3;

	mesh.texture.location	= location;
	mesh.texture.offset		= vramHeap.AddressToOffset(&attr->u);
	mesh.texture.type		= CELL_GCM_VERTEX_F;
	mesh.texture.stride		= sizeof(Vertex);
	mesh.texture.size		= 2;

	mesh.index_offset		= vramHeap.AddressToOffset(index);
	mesh.index_location		= location;
	mesh.index_type			= CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16;
	mesh.draw_mode			= CELL_GCM_PRIMITIVE_TRIANGLES;
	mesh.num_vertex			= num_index;

	tmpHeap.reset();

	return CELL_OK;
}

} // namespace SMesh

const char* MeshManager::MeshName[MESH_ID_LAST] = {
	SYS_APP_HOME"/data/duck.smesh",
	NULL, // ALL_QUAD
};
Mesh MeshManager::mesh_data[MESH_ID_LAST];

void MeshManager::init(){
	Smesh::load(MeshName[MESH_ID_DUCK],CELL_GCM_LOCATION_LOCAL,mesh_data[MESH_ID_DUCK]);

	{
		Smesh::Float2* vertex = Sys::Memory::getLocalMemoryHeap().calloc<Smesh::Float2>(4, 16);
		uint32_t offset = Sys::Memory::getLocalMemoryHeap().AddressToOffset(vertex);

		vertex[0].x = -1.0f;
		vertex[0].y = -3.0f;
		vertex[1].x = -1.0f;
		vertex[1].y = 1.0f;
		vertex[2].x = 3.0f;
		vertex[2].y = 1.0f;
		mesh_data[MESH_ID_ALL_QUAD].position.location = CELL_GCM_LOCATION_LOCAL;
		mesh_data[MESH_ID_ALL_QUAD].position.offset	= offset;
		mesh_data[MESH_ID_ALL_QUAD].position.type		= CELL_GCM_VERTEX_F;
		mesh_data[MESH_ID_ALL_QUAD].position.stride	= sizeof(Smesh::Float2);
		mesh_data[MESH_ID_ALL_QUAD].position.size		= 2;
		mesh_data[MESH_ID_ALL_QUAD].index_offset = 0;
		mesh_data[MESH_ID_ALL_QUAD].num_vertex = 3;
	}
}

};// namespace 
