/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2008 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

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
#include <vectormath/c/vectormath_aos.h>
using namespace Vectormath::Aos;

// for file
#include <cell/cell_fs.h>
#include <sys/paths.h>

// using gcmutil
#include "gcmutil.h"
using namespace CellGcmUtil;

// this project
#include "launcher.h"

using namespace cell::Gcm;

namespace{
	const float MY_PI = 3.14159265f;
	const uint32_t AREA_ELEMENT_MAX = 8192;

	Memory_t sLauncherBuffer;

	typedef struct{
		Vector3 v, a, p, c;
		uint32_t t, life, fead;
		float r, th, alpha, r0;
		Vector3 vrem;
	}Element_t;

	Element_t *sElementArea[4];
	uint32_t sElementAreaNumber[4];
	float *sAreaBuffer[4];

	uint32_t my_rand(uint32_t a, uint32_t b){
		uint32_t s = b - a + 1;
		return rand() % s + a;
	}

	float my_rand_f(){
		return (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
	}

	float my_rand_fu(){
		return (rand() / (float)RAND_MAX);
	}

	Vector3 my_rand_vec(){
		Vector3 v(my_rand_f(), my_rand_fu(), my_rand_f());
		return normalize(v);
	}

	void calcMatrixEx(Element_t &e, float *ptr);
	void updateArea(int32_t area)
	{
		uint32_t num = sElementAreaNumber[area];
		for(uint32_t i = 0; i < num; ++i){
			Element_t &e = sElementArea[area][i];
			e.t += 70;
			if(e.life == 1)
			{
				e.v = e.vrem;
				e.life = my_rand(300, 800);
				e.a = Vector3(0.0f, -0.0098f, 0.0f);
				e.r = length(e.vrem) * (my_rand_fu() * 0.4f + 0.1f);
				e.p = Vector3(0.0f, 40.0f, 0.0f);
				e.t = 0;
			}
			if(e.life != 0){
				--e.life;
			}else{
				continue;
			}

			calcMatrixEx(e, sAreaBuffer[area] + 12 * i);
		}
	}
	uint32_t getNumber(int32_t area)
	{
		return sElementAreaNumber[area];
	}
	float* getBuffer(int32_t area)
	{
		return sAreaBuffer[area];
	}

	void calcMatrixEx(Element_t &e, float *ptr)
	{
		Vector3 pos = e.p + e.t * e.v + (0.5f * e.t * e.t) * e.a;
		if(e.life == 2 && pos[1] < 0.0f){
			e.life = my_rand(300, 800);
		}
		if(pos[1] < 1.1f && (abs(pos[0]) < 100.0f) && (abs(pos[2]) < 100.0f)){
			e.p = pos;
			e.r0 = e.r * e.t + e.r0;
			e.v = (e.v + e.a * e.t) * 0.72f;
			e.v[1] *= -1.0f;
			e.r *= 0.64f;
			e.t = 1;
			pos = e.p + (e.v + 0.5f * e.a * e.t) * e.t;
		}else if(pos[1] < -250.0f){
			e.life = 1;
		}
		Matrix4 *p = reinterpret_cast<Matrix4*>(ptr);
		*p = transpose(Matrix4(Quat::rotation(e.r * e.t + e.r0, e.c), pos));
	}

	void generateElement()
	{
		Element_t new_element;
		new_element.t = 0;
		new_element.life = my_rand(300, 800);
		new_element.fead = new_element.life / 10 + 1;
		float v = my_rand_fu() * 0.5f;
		new_element.v = my_rand_vec() * v + Vector3(0.0f, 0.5f, 0.0f);
		new_element.a = Vector3(0.0f, -0.0098f, 0.0f);
		new_element.r = v * 1.0f;
		new_element.r0 = 0.0f;
		new_element.th = 0.0f;
		new_element.p = Vector3(0.0f, 40.0f, 0.0f);
		Vector3 ang = Vector3(new_element.v[0], 0.0f, new_element.v[2]);
		ang = normalize(ang);
		Matrix3 m = Matrix3::rotationZYX(Vector3(0.0f, MY_PI * 0.5f, 0.0f));
		new_element.c = m * ang;
		new_element.alpha = 1.0f;
		new_element.vrem = new_element.v;

		Vector3 p = new_element.v;
		uint32_t area = ((p[0] < 0.0f) & 0x01) | (((p[2] < 0.0f) << 1) & 0x02);
		
		if(sElementAreaNumber[area] == AREA_ELEMENT_MAX) return;

		uint32_t last = sElementAreaNumber[area];
		sElementArea[area][last] = new_element;
		calcMatrixEx(sElementArea[area][last], sAreaBuffer[area] + 12 * last);
		++sElementAreaNumber[area];
	}

	void deleteElement()
	{
		uint32_t area = my_rand(0, 3);
		if(sElementAreaNumber[area] == 0) return;
		--sElementAreaNumber[area];
	}

} // namespace


bool launchInit(void)
{
	srand(1);
	uint32_t alloc_number = AREA_ELEMENT_MAX + 1;
	uint32_t element_size = sizeof(Element_t) * alloc_number * 4;
	uint32_t area_buffer_size = sizeof(float) * alloc_number * 12 * 4;

	uint32_t rq_size = element_size + area_buffer_size;

	if(cellGcmUtilAllocateUnmappedMain(rq_size, 16, &sLauncherBuffer) == false) return false;
	
	memset(sLauncherBuffer.addr, 0, rq_size);
	uint8_t *base_ptr = reinterpret_cast<uint8_t *>(sLauncherBuffer.addr);

	for(int32_t i = 0; i < 4; ++i){
		sElementArea[i] = reinterpret_cast<Element_t *>(base_ptr + sizeof(Element_t) * alloc_number * i);
		sAreaBuffer[i] = reinterpret_cast<float *>(base_ptr + element_size + sizeof(float) * alloc_number * 12 * i);
	}

	return true;
}
void launchEnd(void)
{
	cellGcmUtilFree(&sLauncherBuffer);
}
void launchUpdate(void)
{
	for(int32_t area = 0; area < 4; ++area){
		updateArea(area);
	}
}
void launchIncrease(uint32_t num)
{
	if(num == 0) return;
	for(uint32_t gen_count = 0; gen_count < num; ++gen_count){
		generateElement();
	}
}
void launchDecrease(uint32_t num)
{
	if(num == 0) return;
	for(uint32_t gen_count = 0; gen_count < num; ++gen_count){
		deleteElement();
	}
}
uint32_t launchGetNumber(int32_t area)
{
	return getNumber(area);
}
float* launchGetBuffer(int32_t area)
{
	return getBuffer(area);
}
void launchChangeMethod(uint32_t method)
{
	(void)method;
}
