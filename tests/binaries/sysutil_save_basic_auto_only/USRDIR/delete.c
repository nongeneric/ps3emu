/*  SCE CONFIDENTIAL                                       */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*  Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*  All Rights Reserved.                                   */
/*  File: delete.c
 *  Description:
 *  simple sample to show how to use savedata system utility
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <cell/sysmodule.h>

#include "common.h"
#include "delete.h"
#include "saveload.h"

/**********************************************************************************/
/* function */
static void thr_delete_savedata(uint64_t arg);

/**********************************************************************************/
/* global variable */
extern int is_running;
extern int receiveExitGameRequest;
extern int last_result;

/**********************************************************************************/
/* main */

#define SAVEDATA_DELETE_CMD_PRIO		(1001)
#define SAVEDATA_DELETE_CMD_STACKSIZE	(4*1024)
/* Name of the PPU thread can be specified with upto 27 characters (excluding the null-terminator) */
#define SAVEDATA_DELETE_THREAD_NAME		"sdu_sample_list_delete"

int delete_savedata(void)
{
	int ret;

	sys_ppu_thread_t tid;

	ret = sys_ppu_thread_create(&tid,
		thr_delete_savedata, 0,
		SAVEDATA_DELETE_CMD_PRIO, SAVEDATA_DELETE_CMD_STACKSIZE,
		0, SAVEDATA_DELETE_THREAD_NAME);

	if (ret != 0) {
		ERR_PRINTF("list delete thread create failed %d\n", ret);
		last_result = ret;
		return -1;
	}

	return 0;
}

void thr_delete_savedata(uint64_t arg)
{
	PRINTF( "thr_delete_savedata() start\n");

	int ret = 0;
	(void)arg;

	DUMP_SYSMEMORY;

	ret = cellSaveDataDelete2( SYS_MEMORY_CONTAINER_ID_INVALID );

	last_result = ret;

	PRINTF("cellSaveDataDelete2() : 0x%x\n", ret);

	DUMP_SYSMEMORY;

	PRINTF( "thr_delete_savedata() end\n");

	is_running = FALSE;

	sys_ppu_thread_exit(0);
}
