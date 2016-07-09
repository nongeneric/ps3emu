/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001                                              */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#include <sys/prx.h>

#include "../../include/library-c-export.h"

SYS_MODULE_INFO( cellPrxLibraryImport, 0, 1, 0 );
SYS_MODULE_START( _start );
SYS_MODULE_STOP( _stop );

int _start(unsigned int args, unsigned int* argp)
{
	argp[0] = export_function() + 5;
    return SYS_PRX_RESIDENT;
}

int _stop(unsigned int args, unsigned int* argp)
{
	argp[0] = export_function() * 3 + cell_prx_export_variable + cell_prx_export_variable2;
    return SYS_PRX_STOP_OK;
}


