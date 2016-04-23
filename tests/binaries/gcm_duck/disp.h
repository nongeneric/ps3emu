/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#ifndef __DISP_H_
#define __DISP_H_
#include <stdio.h>

void setRenderTarget(const uint32_t Index);
void flip(void);
int initDisplay(void);
uint32_t getFrameIndex(void);
void setViewport(void);
#endif // __DISP_H_
