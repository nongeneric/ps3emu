/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2008 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#ifndef __LAUNCHER_H__
#define __LAUNCHER_H__

bool launchInit(void);
void launchEnd(void);
void launchUpdate(void);
void launchIncrease(uint32_t num);
void launchDecrease(uint32_t num);
uint32_t launchGetNumber(int32_t area);
float* launchGetBuffer(int32_t area);
void launchChangeMethod(uint32_t method);

#endif /* __LAUNCHER_H__ */
