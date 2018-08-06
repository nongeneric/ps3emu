/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 475.001
 * Copyright (C) 2006-2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

/*E standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*E Lv2 OS headers */
#include <sys/spu_initialize.h>
#include <sys/spu_thread.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>

#include <cell/sysmodule.h>
/*E SPURS */
#include <cell/spurs.h>
#include <cell/cell_fs.h>

#include <sys/process.h>
#include "utils.h"

const char *mkpath(char *buf, const char *path, const char *mp)
{
	strcpy(buf, mp);
	strcat(buf,"/");
	strcat(buf, path);
	return (const char *)buf;
}   


int isMounted(const char *path)
{
	int i, err;
	CellFsStat status;

	printf("Waiting for mounting on %s\n", path);
	for (i = 0; i < 15; i++) {
		err = cellFsStat(path, &status);
		if (err == CELL_FS_SUCCEEDED) {
			printf("Waiting for mounting done\n");
			return 1;
		}
		sys_timer_sleep(1);
		printf(".\n");
	}
	printf("Waiting for mounting failed\n");
	return 0;
}
