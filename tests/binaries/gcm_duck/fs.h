/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#ifndef __FS_H_
#define __FS_H_
#include <stdio.h>
int initFs(void);
int loadFile(uint32_t* gtf_addr, char* filename, uint32_t location) ;
#endif // __FS_H_
