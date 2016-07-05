/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001                                              */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#include <sdk_version.h>
#include <cellstatus.h>
#include <sys/prx.h>

#include "../../include/call-prx-export.h"

SYS_MODULE_INFO( cellPrxCallPrxExport, 0, 1, 0 );
SYS_MODULE_START( _start );
SYS_MODULE_STOP( _stop );

SYS_LIB_DECLARE( cellPrxCallPrxExportForUser, SYS_LIB_AUTO_EXPORT | SYS_LIB_WEAK_IMPORT );
SYS_LIB_EXPORT( export_function, cellPrxCallPrxExportForUser );

int _start(void);
int _stop(void);

int export_function(int i)
{
    return i * 2;
}

int _start(void)
{
    return SYS_PRX_RESIDENT;
}

int _stop(void)
{
    return SYS_PRX_STOP_OK;
}
