/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 475.001
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
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
#include "spurs_hash_utils.h"
#include <libsha384SPURS.h>
#include "utils.h"
#include "sha384_sum_spurs.h"

#define FILEBUF_SIZE		16*1024
#define MAX_BUFF 4
static unsigned char *g_buf[MAX_BUFF];
unsigned char result[CELL_SHA384_DIGEST_SIZE] __attribute__((__aligned__(16)));
CellSha384SpuInstance inst;


int sha384_sum_spu(const char *mount, const char *file, CellSpursTaskset2 *taskset)
{
	#define SHA_QDEPTH 8 

	int ret;
	int fd;
	char path[256];
	uint64_t  nread;
	int res;

	int read_buf=0, i;
	int job_active[MAX_BUFF];


	for (i = 0; i < MAX_BUFF;i++){
		job_active[i] = 0;
		g_buf[i]=memalign(16, FILEBUF_SIZE);
	}


	ret = cellSha384SpuCreateTask2(&inst, taskset, NULL, NULL, SHA_QDEPTH,DMA_BUFFER_SIZE);

	//E Initialize CellFS and open file
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
	if (ret) {
		printf("cellSysmoduleLoadModule() error 0x%x !\n", ret);
		sys_process_exit(1);
	}
 
	if (!isMounted(mount)) {
		sys_process_exit(1);
	}

	res=cellFsOpen(mkpath(path, file, mount), CELL_FS_O_RDONLY, &fd, NULL, 0);

	if (fd < 0) {
		printf("cellFsOpen('%s') error: 0x%08X\n", path, res);
		return (1);
	}



	//E Process data in blocks of size FILEBUF_SIZE
	do {

		if (job_active[read_buf]){
			ret = cellSha384SpuCheckStatus(&inst, 1 << read_buf, 1);
		}
		res = cellFsRead(fd, g_buf[read_buf], FILEBUF_SIZE, &nread);

		if (res < 0) {
			printf("cellFsRead('%s') error: 0x%08X\n", path, res);
			cellFsClose(fd);	
			return (1);
		}

		ret = cellSha384SpuProcessData(&inst, (uint64_t)((int)g_buf[read_buf]), nread, 1 << read_buf, 1);
		job_active[ read_buf ] = 1;

		read_buf = (read_buf + 1) % MAX_BUFF;
	} while (nread == FILEBUF_SIZE);
	ret=cellSha384SpuEndTask2(&inst, (uint64_t)result);

	printf("Hash result is: ");
	for (i = 0;i < CELL_SHA384_DIGEST_SIZE;i++){
		printf("%02x", result[i]);
	}
	printf("\n");

	for (i = 0; i < MAX_BUFF;i++){
		if(g_buf[i] != NULL)
			free(g_buf[i]);
	}

	cellFsClose(fd);

	return 0;
}
