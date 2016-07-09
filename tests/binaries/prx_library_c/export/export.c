/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001                                              */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#include <cellstatus.h>
#include <sdk_version.h>
#include <sys/prx.h>

#include "../../include/library-c-export.h"

SYS_MODULE_INFO( cellPrxLibraryExport, 0, 1, 0 );
SYS_MODULE_START( _start );
SYS_MODULE_STOP( _stop );

SYS_LIB_DECLARE( cellPrxLibraryExportForUser, SYS_LIB_AUTO_EXPORT );
SYS_LIB_EXPORT( export_function, cellPrxLibraryExportForUser );
SYS_LIB_EXPORT_VAR( cell_prx_export_variable, cellPrxLibraryExportForUser );
SYS_LIB_EXPORT_VAR( cell_prx_export_variable2, cellPrxLibraryExportForUser );

int	cell_prx_export_variable = CELL_SDK_VERSION;
int	cell_prx_export_variable2 = 0xf00d;

int _start(void);
int _stop(void);

int export_function(void)
{
    return 7;
}

int _start(void)
{
	cell_prx_export_variable = 77;
	cell_prx_export_variable2 = 0x11223344;
    return SYS_PRX_RESIDENT;
}

int _stop(void)
{
    return SYS_PRX_STOP_OK;
}
