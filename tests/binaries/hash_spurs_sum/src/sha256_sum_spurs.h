/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 475.001
* Copyright (C) 2006 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/


//DOM-IGNORE-BEGIN
#ifndef _SUM_SPU_SHA256_H_
#define _SUM_SPU_SHA256_H_


#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif	/* defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus) */

int sha256_sum_spu(const char *mount, const char *file, CellSpursTaskset2 *taskset);
int sha256_sum_spu_multi(const char *mount, const char **file, int num_files,CellSpursTaskset2 *taskset);

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif	/* defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus) */

#endif	/* _SUM_SPU_SHA256_H_ */
