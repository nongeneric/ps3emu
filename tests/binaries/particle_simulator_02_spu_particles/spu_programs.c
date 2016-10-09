/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#include <sys/prx.h>
#include "spu_programs.h"

extern char _binary_particle_spu_elf_start[];

// E This function is invoked when the PRX is loaded, to do any initialization
// E necessary.
int SpuProgramsStart(size_t args, void *argp);
// E This function is stopping when the PRX is loaded, to do any shutdown
// E necessary.
int SpuProgramsStop(size_t args, void *argp);

// E Define a library called 'spu_programs', version 1.1
SYS_MODULE_INFO(spu_programs, 0, 1, 1);
// E This is our startup function
SYS_MODULE_START(SpuProgramsStart);
// E This is our shutdown function
SYS_MODULE_STOP(SpuProgramsStop);

SYS_LIB_DECLARE(spu_programs, SYS_LIB_AUTO_EXPORT | SYS_LIB_WEAK_IMPORT);
// E This is the function we're exporting
SYS_LIB_EXPORT(getParticleSpuElf, spu_programs);

/**
 * E PRX initialization function.  For our sample, it just succeeds
 */
int SpuProgramsStart(size_t args, void *argp) {
	(void) args;
	(void) argp;

	return SYS_PRX_RESIDENT;
}

/**
 * E PRX shutdown function.  For our sample, it just returns.
 */
int SpuProgramsStop(size_t args, void *argp) {
	(void) args;
	(void) argp;

	return 0;
}

/**
 * E Returns a pointer to the SPU .elf program.
 * This symbol is being linked in.
 */
char *getParticleSpuElf() {
	return _binary_particle_spu_elf_start;
}
