/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001                                              */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#include <sys/prx.h>

SYS_MODULE_INFO( SysPrxSimple, 0, 1, 1);
SYS_MODULE_START(_start);
SYS_MODULE_STOP(_stop);

int _stop(void);

int _start(unsigned int args, unsigned int* argp)
{
	for (int i = 0; i < args; ++i) {
		argp[i] = 2 * i;
	}
    return SYS_PRX_NO_RESIDENT;
}

int _stop(void)
{
    return 0x13;
}
