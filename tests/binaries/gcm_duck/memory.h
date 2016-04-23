/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#ifndef __MEMORY_H_
#define __MEMORY_H_
#include <stdio.h>
void *localMemoryAlloc(const uint32_t size);
void *localMemoryAlign(const uint32_t alignment, const uint32_t size);
void *localMemoryAlloc2(const uint32_t size);
void *localMemoryAlign2(const uint32_t alignment, const uint32_t size);
void *mainMemoryAlloc(const uint32_t size) ;
void *mainMemoryAlign(const uint32_t alignment, const uint32_t size);
int initMemory(void);
#endif //__MEMORY_H_
