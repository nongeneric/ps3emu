/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2008 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#ifndef __FRAGMENT_PATCH_H__
#define __FRAGMENT_PATCH_H__

typedef struct {
	uint32_t value[4];

	uint8_t isEnable;
	uint8_t nArray;
	uint16_t length;
	uint32_t constCount;
	uint32_t constStart;
	uint32_t *offsets;
} __attribute__((aligned(16))) PatchParam_t;

#endif /* __FRAGMENT_PATCH_H__ */
