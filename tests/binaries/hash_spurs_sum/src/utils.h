/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 475.001
* Copyright (C) 2006 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/


//DOM-IGNORE-BEGIN
#ifndef _SPU_SUM_UTILS_H_
#define _SPU_SUM_UTILS_H_


#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif	/* defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus) */
#define DMA_BUFFER_SIZE			(16*1024)
const char *mkpath(char *buf, const char *path, const char *mp);
int isMounted(const char *path);

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif	/* defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus) */

#endif	/* _SPU_SUM_UTILS_H_ */
