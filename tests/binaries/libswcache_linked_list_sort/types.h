/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2008 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef __LINKED_LIST_TYPES_H__
#define __LINKED_LIST_TYPES_H__

#include <stdint.h>

typedef struct Node {
	uint32_t mVal;
	struct Node *mpPrev, *mpNext;
} Node;

#include <cell/swcache/pointer.h>
using namespace cell::swcache;

typedef struct NodeP {
	uint32_t mVal;
	Pointer<struct NodeP> mpPrev, mpNext;
} NodeP;

#endif /* __LINKED_LIST_TYPES_H__ */
