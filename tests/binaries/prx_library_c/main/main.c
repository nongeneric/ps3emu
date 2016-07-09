/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001                                              */
/*   Copyright (C) 2007 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#include <cell/error.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/paths.h>
#include <sys/prx.h>

#include "../../include/prx_path.h"

SYS_PROCESS_PARAM(1001, 0x10000);

static sys_prx_id_t load_start(const char *path, int *modres)
{
    int			res;
    sys_prx_id_t	id;

    id = sys_prx_load_module(path, 0, NULL);
    if (id < CELL_OK) {
	printf("sys_prx_load_module failed: 0x%08x\n", id);
	return id;
    }
	uint32_t val = 3;
    res = sys_prx_start_module(id, 1, &val, modres, 0, NULL);
	printf("load val=%d\n", val);
    if (res < CELL_OK) {
	printf("start failed: 0x%08x\n", res);
	return res;
    }
    return id;
}

static sys_prx_id_t stop_unload(sys_prx_id_t id)
{
    int			modres;
    int			res;

	uint32_t val = 9;
    res = sys_prx_stop_module(id, 1, &val, &modres, 0, NULL);
	printf("unload val=%d\n", val);
    if (res < CELL_OK) {
	printf("sys_prx_stop_module failed: id=0x%08x, 0x%08x\n", id, res);
	return res;
    }
    res = sys_prx_unload_module(id, 0, NULL);
    if (res < CELL_OK) {
	printf("sys_prx_unload_mdoule failed: 0x%08x\n", res);
	return res;
    }
    return id;
}

int main(void)
{
    int			res;
    int			modres;
    sys_prx_id_t	export_module;
    sys_prx_id_t	import_module;

    printf("library-c-main:start\n");
    export_module = load_start(SYS_APP_HOME SYS_PRX_PATH "/library-c-export.sprx", &modres);
    if (export_module < CELL_OK) {
	printf("load_module failed\n");
	return (-1);
    }

    import_module = load_start(SYS_APP_HOME SYS_PRX_PATH "/library-c-import.sprx", &modres);
    if (import_module < CELL_OK) {
	printf("load_module failed\n");
	return (-1);
    }

    res = stop_unload(import_module);
    if (res < CELL_OK) {
	printf("stop_unload failed\n");
	return (-1);
    }
    res = stop_unload(export_module);
    if (res < CELL_OK) {
	printf("stop_unload failed\n");
	return (-1);
    }
    printf("library-c-main:done\n");
    return CELL_OK;
}
