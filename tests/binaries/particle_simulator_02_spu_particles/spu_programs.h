/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#ifndef __SPU_PROGRAMS_H
#define __SPU_PROGRAMS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * E Returns a pointer to the SPU .elf program.
 * Be sure to load this PRX before calling the function!
 */
char *getParticleSpuElf(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __SPU_PROGRAMS_H

