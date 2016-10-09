/*  SCE CONFIDENTIAL                                      */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.   */
/*  All Rights Reserved.                                  */
#include <stdio.h>

#include <sys/process.h>
#include <cell/sysmodule.h>

extern int32_t userMain(void);

SYS_PROCESS_PARAM(1001, 0x10000)

int main(void)
{
    int	ret;

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_RESC);
	if (ret < 0) {
		printf("cellSysmoduleLoadModule RESC failed (0x%x)\n", ret);
		return 0;
    }

	/*E entry point of user program */
	userMain();

	/*E unload relocatable modules */
	ret = cellSysmoduleUnloadModule(CELL_SYSMODULE_RESC);
	if (ret < 0) {
		printf("cellSysmoduleUnloadModule RESC failed (0x%x)\n", ret);
		return 0;
    }

    return CELL_OK;
}
