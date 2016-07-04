/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001                                              */
/*   Copyright (C) 2007 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#include <cell/error.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/process.h>
#include <sys/paths.h>
#include <sys/prx.h>

#include "../../include/prx_path.h"

SYS_PROCESS_PARAM(1001, 0x10000);

int main(void)
{
    int			res;
    int			modres;
    sys_prx_id_t	id;

    printf("simple-main:start\n");
	unsigned int args[3];
    id = sys_prx_load_module(SYS_APP_HOME SYS_PRX_PATH  "/simple-c-prx.sprx", 0, NULL);
    if (id < CELL_OK) {
	printf("sys_prx_load_module failed\n");
		exit(-1);
    }
    res = sys_prx_start_module(id, 3, args, &modres, 0, NULL);
    if (res < CELL_OK) {
		printf("start failed\n");
		exit(-1);
    }
	printf("arg0 = %d, arg1 = %d, arg2 = %d, modres = %d\n", args[0], args[1], args[2], modres);
    printf("simple-main:done\n");
    return CELL_OK;
}
