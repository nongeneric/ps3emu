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
#include "../../include/call-prx-export.h"

SYS_PROCESS_PARAM(1001, 0x10000);

static int call_import(void)
{
    printf("call_import start\n");
    int i = export_function(10);
	printf("%d\n", i);
    return CELL_OK;
}

static sys_prx_id_t load_start(const char *path)
{
    int			res;
    int			modres;
    sys_prx_id_t	id;

    id = sys_prx_load_module(path, 0, NULL);
    if (id < CELL_OK) {
	printf("sys_prx_load_module failed: 0x%08x\n", id);
	return id;
    }
    res = sys_prx_start_module(id, 0, NULL, &modres, 0, NULL);
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

    res = sys_prx_stop_module(id, 0, NULL, &modres, 0, NULL);
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
    sys_prx_id_t	id;

    printf("call-prx-main:start\n");
    id = load_start(SYS_APP_HOME SYS_PRX_PATH "/call-prx-export.sprx");
    if (id < CELL_OK) {
	printf("load_start failed: 0x%08x\n", id);
	return id;
    }

    call_import();

    res = stop_unload(id);
    if (res < CELL_OK) {
	printf("stop_unload failed: 0x%08x\n", res);
	return res;
    }
    printf("call-prx-main:done\n");
    return CELL_OK;
}
