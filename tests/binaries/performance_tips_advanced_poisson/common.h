/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2007 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

#define POISSON_SPU_NUM 3
#define MAX_RESIDUAL 0.001f
#define NX 128
#define NY 128

typedef struct _DataAddress{
	uint32_t nx;
	uint32_t ny;
	uint64_t sync_ea;
	uint64_t scalar_ea;
	uint64_t x_ea;
	uint64_t r_ea;
	uint64_t p_ea[2];
	uint64_t ap_ea;
}__attribute__ ((aligned(16))) DataAddress;

typedef struct _ScalarData{
	float pap;
	float pr;
	float rap;
	float rr;
}__attribute__ ((aligned(16))) ScalarData;

#endif // _COMMON_H_

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
